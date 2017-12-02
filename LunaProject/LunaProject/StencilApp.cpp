#include "stdafx.h"
#include "StencilApp.h"

StencilApp::StencilApp(HINSTANCE hInstance) : D3DApp(hInstance)
{
}

StencilApp::~StencilApp()
{
	if (md3dDevice != nullptr) {
		FlushCommandQueue();
	}
}

bool StencilApp::Initialize()
{
	if (!D3DApp::Initialize()) {
		return false;
	}

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	StoreConstants();
	CreateRootSignature();
	CreateShaderInputLayout();
	
	// textures
	LoadTextures();
	BuildMaterials();
	BuildRoomGeometry();
	BuildSkullGeometry();
	BuildRenderItems();
	BuildFrameResources();

	BuildDescriptorHeaps();
	BuildTextureDescriptors();
	BuildMaterialDescriptors();


	CreatePSOs();
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* commandLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	FlushCommandQueue();
	return true;
}

void StencilApp::StoreConstants()
{
	XMVECTOR mirrorPlane = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	XMMATRIX R = XMMatrixReflect(mirrorPlane);

	XMStoreFloat4x4(&mReflectionMatrix, R);
	
	XMMATRIX S = XMMatrixScaling(0.25f, 0.25f, 0.25f);
	XMMATRIX Rot = XMMatrixRotationY(mSkullRotation);
	XMStoreFloat4x4(&mSkullSMatrix, S);
	XMStoreFloat4x4(&mSkullRMatrix, Rot);


	XMMATRIX T = XMMatrixTranslation(1.0f, 0.0f, -5.0f);
	XMStoreFloat4x4(&mSkullTMatrix, T);

	mLights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	mLights[0].Strength = { 0.6f, 0.6f, 0.6f };
	mLights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	mLights[1].Strength = { 0.3f, 0.3f, 0.3f };
	mLights[2].Direction = { 0.0f, -0.707f, -0.707f };
	mLights[2].Strength = { 0.15f, 0.15f, 0.15f };

	XMVECTOR shadowPlane = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR lightVec = XMLoadFloat3(&mLights[0].Direction);
	XMMATRIX shadowMatrix = XMMatrixShadow(shadowPlane, -lightVec);
	XMStoreFloat4x4(&mShadowMatrix, shadowMatrix);
	XMMATRIX shadowOffset = XMMatrixTranslation(0.0f, 0.01f, 0.0f);
	XMStoreFloat4x4(&mShadowOffset, shadowOffset);
}

void StencilApp::LoadTextures()
{
	// STUB
	std::unique_ptr<Texture> tex;
	auto loadTexture = [&](std::string&& texName, const std::wstring& fileName) {
		tex = std::make_unique<Texture>();
		tex->Filename = fileName;
		tex->Name = texName;
		ThrowIfFailed(CreateDDSTextureFromFile12(md3dDevice.Get(), mCommandList.Get(), fileName.c_str(),
			tex->Resource, tex->UploadHeap));
		mTextureNames.emplace_back(texName);
		mTextures[texName] = std::move(tex);
	};

	loadTexture("ice", L"Textures//ice.dds");		// 0
	loadTexture("stone", L"Textures//stone.dds");	// 1
	loadTexture("tile", L"Textures//tile.dds");		// 2
	loadTexture("white", L"Textures//white1x1.dds"); // 3
	loadTexture("checkerboard", L"Textures//checkboard.dds");
	loadTexture("bricks", L"Textures//bricks3.dds");
}

void StencilApp::BuildFrameResources()
{
	for (int i = 0; i < mNumFrameResources; ++i) {
		mFrameResources.emplace_back(
			std::make_unique<FrameResource>(md3dDevice.Get(), 2, (UINT)mRenderItems.size(), (UINT) mMaterials.size())
		);
	}
}

void StencilApp::BuildSkullGeometry()
{
	// GeometryGenerator geoGen;
	GeometricObject geoObj("StencilApp");

	geoObj.AddObject(mSkull.GetVertices(), mSkull.GetIndices());
	
	auto convertVertex = [&](const GeometryGenerator::Vertex& vertex) {
		Vertex v;
		v.Pos = vertex.Position;
		v.Normal = vertex.Normal;
		v.TexC = vertex.TexC;
		return v;
	};

	auto vertices = getVertices<Vertex>(&geoObj, convertVertex);
	UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	auto indices = geoObj.GetIndices();
	UINT ibByteSize = (UINT)indices.size() * sizeof(uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));

	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->Name = "skullGeo";
	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);
	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->DrawArgs["skull"] = geoObj.GetSubmesh(0);
	mGeometries[geo->Name] = std::move(geo);
}

void StencilApp::BuildRoomGeometry()
{
		// Create and specify geometry.  For this sample we draw a floor
		// and a wall with a mirror on it.  We put the floor, wall, and
		// mirror geometry in one vertex buffer.
		//
		//   |--------------|
		//   |              |
		//   |----|----|----|
		//   |Wall|Mirr|Wall|
		//   |    | or |    |
		//   /--------------/
		//  /   Floor      /
		// /--------------/

		std::array<Vertex, 20> vertices =
		{
			// Floor: Observe we tile texture coordinates.
			Vertex(-3.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 0.0f, 4.0f), // 0 
			Vertex(-3.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f),
			Vertex(7.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 4.0f, 0.0f),
			Vertex(7.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 4.0f, 4.0f),

			// Wall: Observe we tile texture coordinates, and that we
			// leave a gap in the middle for the mirror.
			Vertex(-3.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f), // 4
			Vertex(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
			Vertex(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 0.0f),
			Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 2.0f),

			Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f), // 8 
			Vertex(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
			Vertex(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 0.0f),
			Vertex(7.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 2.0f),

			Vertex(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f), // 12
			Vertex(-3.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
			Vertex(7.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 0.0f),
			Vertex(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 1.0f),

			// Mirror
			Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f), // 16
			Vertex(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
			Vertex(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f),
			Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f)
		};

		std::array<std::int16_t, 30> indices =
		{
			// Floor
			0, 1, 2,
			0, 2, 3,

			// Walls
			4, 5, 6,
			4, 6, 7,

			8, 9, 10,
			8, 10, 11,

			12, 13, 14,
			12, 14, 15,

			// Mirror
			16, 17, 18,
			16, 18, 19
		};

		SubmeshGeometry floorSubmesh;
		floorSubmesh.IndexCount = 6;
		floorSubmesh.StartIndexLocation = 0;
		floorSubmesh.BaseVertexLocation = 0;

		SubmeshGeometry wallSubmesh;
		wallSubmesh.IndexCount = 18;
		wallSubmesh.StartIndexLocation = 6;
		wallSubmesh.BaseVertexLocation = 0;

		SubmeshGeometry mirrorSubmesh;
		mirrorSubmesh.IndexCount = 6;
		mirrorSubmesh.StartIndexLocation = 24;
		mirrorSubmesh.BaseVertexLocation = 0;

		const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
		const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

		auto geo = std::make_unique<MeshGeometry>();
		geo->Name = "roomGeo";

		ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
		CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

		ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
		CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

		geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
			mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

		geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
			mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

		geo->VertexByteStride = sizeof(Vertex);
		geo->VertexBufferByteSize = vbByteSize;
		geo->IndexFormat = DXGI_FORMAT_R16_UINT;
		geo->IndexBufferByteSize = ibByteSize;

		geo->DrawArgs["floor"] = floorSubmesh;
		geo->DrawArgs["wall"] = wallSubmesh;
		geo->DrawArgs["mirror"] = mirrorSubmesh;

		mGeometries[geo->Name] = std::move(geo);
}

void StencilApp::BuildTextureDescriptors()
{
	auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mSrvCbvHeap->GetCPUDescriptorHandleForHeapStart());
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	desc.Texture2D.MostDetailedMip = 0;
	desc.Texture2D.ResourceMinLODClamp = 0.0f;

	auto buildTextureDescriptor = [&](const std::unique_ptr<Texture>& tex) {
		auto resource = tex->Resource;
		desc.Texture2D.MipLevels = resource->GetDesc().MipLevels;
		desc.Format = resource->GetDesc().Format;
		md3dDevice->CreateShaderResourceView(resource.Get(), &desc, handle);
		handle.Offset(1, mCbvSrvDescriptorSize);
	};

	UINT numTextures = mTextureNames.size();
	for (UINT i = 0; i < numTextures;  ++i) {
		buildTextureDescriptor(mTextures[mTextureNames[i]]);
	}
}

void StencilApp::BuildMaterialDescriptors()
{
	mMaterialIndexStart = mTextures.size();
	auto matCbSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));
	D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
	desc.SizeInBytes = matCbSize;
	auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mSrvCbvHeap->GetCPUDescriptorHandleForHeapStart());
	handle.Offset(mMaterialIndexStart, mCbvSrvDescriptorSize);
	UINT numMaterials = (UINT)mMaterials.size();
	for (int i = 0; i < mNumFrameResources; ++i) {
		auto frameResource = mFrameResources[i]->MaterialCB->Resource();
		for (UINT j = 0; j < numMaterials; ++j) {
			auto cbAddress = frameResource->GetGPUVirtualAddress();
			cbAddress += j * matCbSize;
			desc.BufferLocation = cbAddress;
			md3dDevice->CreateConstantBufferView(&desc, handle);
			handle.Offset(1, mCbvSrvDescriptorSize);
		}
	}
}

void StencilApp::BuildMaterials()
{
	auto stone = std::make_unique<Material>(mNumFrameResources);
	stone->Name = "stone";
	stone->MatCBIndex = 0;
	stone->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	stone->FresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.1f);
	stone->Roughness = 0.3f;
	stone->DiffuseSrvHeapIndex = 1;

	auto ice = std::make_unique<Material>(mNumFrameResources);
	ice->Name = "ice";
	ice->MatCBIndex = 1;
	ice->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.3f);
	ice->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	ice->Roughness = 0.0f;
	ice->DiffuseSrvHeapIndex = 0;

	auto tile = std::make_unique<Material>(mNumFrameResources);
	tile->Name = "tile";
	tile->MatCBIndex = 2;
	tile->DiffuseSrvHeapIndex = 2;
	tile->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	tile->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	tile->Roughness = 0.40f;

	auto white = std::make_unique<Material>(mNumFrameResources);
	white->Name = "white";
	white->MatCBIndex = 3;
	white->DiffuseSrvHeapIndex = 3;
	white->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	white->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	white->Roughness = 0.78f;


	auto checkerboard = std::make_unique<Material>(mNumFrameResources);
	checkerboard->Name = "checkerboard";
	checkerboard->MatCBIndex = 4;
	checkerboard->DiffuseSrvHeapIndex = 4;
	checkerboard->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	checkerboard->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	checkerboard->Roughness = 0.10f;


	auto bricks = std::make_unique<Material>(mNumFrameResources);
	bricks->Name = "bricks";
	bricks->MatCBIndex = 5;
	bricks->DiffuseSrvHeapIndex = 5;
	bricks->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	bricks->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	bricks->Roughness = 0.78f;

	auto shadow = std::make_unique<Material>(mNumFrameResources);
	shadow->Name = "shadow";
	shadow->MatCBIndex = 6;
	shadow->DiffuseSrvHeapIndex = 3;
	shadow->DiffuseAlbedo = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f);
	shadow->FresnelR0 = XMFLOAT3(0.001f, 0.001f, 0.001f);
	shadow->Roughness = 0.01f;

	mMaterials["stone"] = std::move(stone);
	mMaterials["ice"] = std::move(ice);
	mMaterials["tile"] = std::move(tile);
	mMaterials["white"] = std::move(white);
	mMaterials["checkerboard"] = std::move(checkerboard);
	mMaterials["bricks"] = std::move(bricks);
	mMaterials["shadow"] = std::move(shadow);
}

void StencilApp::BuildRenderItems()
{
	std::unique_ptr<RenderItem> rItem;
	auto buildRenderItem = [&](std::string&& submeshName, const XMMATRIX& world, UINT&& objCBIndex,
		const std::unique_ptr<Material>& mat, std::string&& geo, LightingDemo::RenderLayer* rLayer, UINT&& layerCount) -> RenderItem* {
		rItem = std::make_unique<RenderItem>();
		XMStoreFloat4x4(&rItem->World, world);
		rItem->TexTransform = MathHelper::Identity4x4();
		rItem->ObjCBIndex = objCBIndex;
		rItem->Geo = mGeometries[geo].get();
		rItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rItem->IndexCount = rItem->Geo->DrawArgs[submeshName].IndexCount;
		rItem->StartIndexLocation = rItem->Geo->DrawArgs[submeshName].StartIndexLocation;
		rItem->BaseVertexLocation = rItem->Geo->DrawArgs[submeshName].BaseVertexLocation;
		rItem->Mat = mat.get();
		for (UINT i = 0; i < layerCount; ++i) {
			mRenderLayers[(UINT)rLayer[i]].emplace_back(rItem.get());
		}
		auto toReturn = rItem.get();
		mRenderItems.emplace_back(std::move(rItem));
		return toReturn;
	};

	XMMATRIX world = XMLoadFloat4x4(&MathHelper::Identity4x4());

	RenderLayer renderLayers[1] = { RenderLayer::Opaque };
	buildRenderItem("floor", world, 0, mMaterials["checkerboard"], "roomGeo", renderLayers, _countof(renderLayers));
	buildRenderItem("wall", world, 1, mMaterials["bricks"], "roomGeo", renderLayers, _countof(renderLayers));

	RenderLayer mirrorRenderLayers[2] = { RenderLayer::Mirrors, RenderLayer::Transparent };
	buildRenderItem("mirror", world, 2, mMaterials["ice"], "roomGeo", mirrorRenderLayers, _countof(mirrorRenderLayers));
	world = XMLoadFloat4x4(&mSkullSMatrix)*XMLoadFloat4x4(&mSkullRMatrix)*XMLoadFloat4x4(&mSkullTMatrix);
	mSkullRItem = buildRenderItem("skull", world, 3, mMaterials["white"], "skullGeo", renderLayers, _countof(renderLayers));

	// reflected skull
	RenderLayer reflectionLayers[1] = { RenderLayer::Reflected };
	XMMATRIX R = world*XMLoadFloat4x4(&mReflectionMatrix);
	mReflectedSkullRItem = buildRenderItem("skull", R, 4, mMaterials["white"], "skullGeo", reflectionLayers, _countof(reflectionLayers));


	// shadow
	RenderLayer shadowLayers[1] = { RenderLayer::Shadow };
	XMMATRIX S = world*XMLoadFloat4x4(&mShadowMatrix)*XMLoadFloat4x4(&mShadowOffset);
	mSkullShadowRItem = buildRenderItem("skull", S, 5, mMaterials["shadow"], "skullGeo", shadowLayers, _countof(shadowLayers));
}

void StencilApp::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NumDescriptors = (UINT)mTextures.size() + (UINT)mMaterials.size() * mNumFrameResources;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(mSrvCbvHeap.GetAddressOf())));
}

void StencilApp::DrawRenderItems(const std::vector<RenderItem*> rItems)
{
	auto cbByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	auto matByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));
	auto baseAddr = mCurrentFrameResource->ObjectCB->Resource()->GetGPUVirtualAddress();
	auto matIndexOffset = mMaterialIndexStart + mCurrentFrameResourceIndex * mMaterials.size();
	for (auto& rItem : rItems) {
		mCommandList->IASetVertexBuffers(0, 1, &rItem->Geo->VertexBufferView());
		mCommandList->IASetIndexBuffer(&rItem->Geo->IndexBufferView());
		mCommandList->IASetPrimitiveTopology(rItem->PrimitiveType);

		// object constants
		mCommandList->SetGraphicsRootConstantBufferView(0, baseAddr + rItem->ObjCBIndex * cbByteSize);

		// material constants
		mCommandList->SetGraphicsRootDescriptorTable(1, 
			GetDescriptorHandleWithOffset(matIndexOffset + rItem->Mat->MatCBIndex));

		// texture
		mCommandList->SetGraphicsRootDescriptorTable(3, GetDescriptorHandleWithOffset((UINT) rItem->Mat->DiffuseSrvHeapIndex));
	
		mCommandList->DrawIndexedInstanced(rItem->IndexCount, 1, rItem->StartIndexLocation, rItem->BaseVertexLocation, 0);
	}
}

void StencilApp::CreateRootSignature()
{
	CD3DX12_ROOT_PARAMETER params[4];

	params[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL); // object constants b0, param 0
	CD3DX12_DESCRIPTOR_RANGE cbvTable0(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1); // mat constants register b1, param 1
	params[1].InitAsDescriptorTable(1, &cbvTable0, D3D12_SHADER_VISIBILITY_ALL);

	params[2].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_ALL); // pass constants b2, param 2
	CD3DX12_DESCRIPTOR_RANGE srvTable0(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // diffuse tex in t0, param 3
	params[3].InitAsDescriptorTable(1, &srvTable0, D3D12_SHADER_VISIBILITY_PIXEL);

	auto samplers = GetStaticSamplers();

	auto rootSigDesc = CD3DX12_ROOT_SIGNATURE_DESC(4, params, (UINT)samplers.size(),
		samplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> rootSig;
	ComPtr<ID3DBlob> errorBlob;

	auto handle = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSig, &errorBlob);

	ThrowIfFailed(handle);

	if (errorBlob != nullptr) {
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	
	ThrowIfFailed(md3dDevice->CreateRootSignature(0, rootSig->GetBufferPointer(),
		rootSig->GetBufferSize(), IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void StencilApp::CreateShaderInputLayout()
{
	mShaders["standardVS"] = d3dUtil::CompileShader(L"Shaders//AnimatedWaves.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders//AnimatedWaves.hlsl", nullptr, "PS", "ps_5_1");

	mInputLayout = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void StencilApp::BuildOpaquePSO(D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
{
	desc.pRootSignature = mRootSignature.Get();
	desc.VS = {
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};
	desc.PS = {
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};

	desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	desc.SampleMask = UINT_MAX;
	D3D12_RASTERIZER_DESC rState = {};
	rState.FillMode = D3D12_FILL_MODE_SOLID;
	rState.CullMode = D3D12_CULL_MODE_BACK;
	
	desc.RasterizerState = rState;
	desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	desc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = mBackBufferFormat;
	desc.DSVFormat = mDepthStencilFormat;

	desc.SampleDesc.Quality = m4xMsaaState ? m4xMsaaQuality - 1 : 0;
	desc.SampleDesc.Count = m4xMsaaState ? 4 : 1;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&mPSOs["opaque"])));

	auto wireframeDesc = desc;
	wireframeDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&wireframeDesc, IID_PPV_ARGS(&mPSOs["wireframe"])));
}

void StencilApp::BuildMirrorPSO(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
{
	CD3DX12_BLEND_DESC mirrorBlendState(D3D12_DEFAULT);
	mirrorBlendState.RenderTarget[0].RenderTargetWriteMask = 0;

	D3D12_DEPTH_STENCIL_DESC mirrorDSS;
	mirrorDSS.DepthEnable = true;
	mirrorDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	mirrorDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	mirrorDSS.StencilEnable = true;
	mirrorDSS.StencilReadMask = 0xff;
	mirrorDSS.StencilWriteMask = 0xff;
	
	// this never happens with stencil func set to always pass
	mirrorDSS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	// if depth fails, keep the pixel
	mirrorDSS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	// if depth passes, mark the stencil buffer with 1
	mirrorDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
	// stencil test set to always pass
	mirrorDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	mirrorDSS.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	mirrorDSS.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	mirrorDSS.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
	mirrorDSS.BackFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC markMirrorsPSODesc = desc;
	markMirrorsPSODesc.BlendState = mirrorBlendState;
	markMirrorsPSODesc.DepthStencilState = mirrorDSS;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&markMirrorsPSODesc, IID_PPV_ARGS(&mPSOs["markStencilMirrors"])));
}

D3D12_GRAPHICS_PIPELINE_STATE_DESC StencilApp::BuildTransparentPSO(const D3D12_GRAPHICS_PIPELINE_STATE_DESC & desc)
{
	CD3DX12_BLEND_DESC transparentBlend(D3D12_DEFAULT);
	transparentBlend.RenderTarget[0].BlendEnable = true;
	transparentBlend.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	transparentBlend.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transparentBlend.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	transparentBlend.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	transparentBlend.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
	transparentBlend.RenderTarget[0].LogicOpEnable = false;
	transparentBlend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	transparentBlend.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	transparentBlend.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;

	auto transparentPSO = desc;
	transparentPSO.BlendState = transparentBlend;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&transparentPSO, IID_PPV_ARGS(&mPSOs["transparent"])));

	return transparentPSO;
}

void StencilApp::BuildReflectionPSO(const D3D12_GRAPHICS_PIPELINE_STATE_DESC & desc)
{
	D3D12_DEPTH_STENCIL_DESC reflectionsDSS;
	reflectionsDSS.DepthEnable = true;
	reflectionsDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	reflectionsDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	reflectionsDSS.StencilEnable = true;
	reflectionsDSS.StencilReadMask = 0xff;
	reflectionsDSS.StencilWriteMask = 0xff;

	
	reflectionsDSS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	reflectionsDSS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	reflectionsDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	reflectionsDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;

	reflectionsDSS.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	reflectionsDSS.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	reflectionsDSS.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
	reflectionsDSS.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;

	// correct for reversed normals
	auto rasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	rasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	rasterizerState.FrontCounterClockwise = true;
	
	auto reflectionsPSO = desc;
	reflectionsPSO.DepthStencilState = reflectionsDSS;
	reflectionsPSO.RasterizerState = rasterizerState;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&reflectionsPSO, IID_PPV_ARGS(&mPSOs["reflections"])));
}

void StencilApp::BuildShadowPSO(const D3D12_GRAPHICS_PIPELINE_STATE_DESC & desc)
{
	D3D12_DEPTH_STENCIL_DESC shadowDSS;
	shadowDSS.DepthEnable = true;
	shadowDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	shadowDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	
	shadowDSS.StencilEnable = true;
	shadowDSS.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	shadowDSS.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	shadowDSS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	shadowDSS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	shadowDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_INCR;
	shadowDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;

	shadowDSS.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	shadowDSS.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	shadowDSS.BackFace.StencilPassOp = D3D12_STENCIL_OP_INCR;
	shadowDSS.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;

	auto shadowPSO = desc;
	shadowPSO.DepthStencilState = shadowDSS;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&shadowPSO, IID_PPV_ARGS(&mPSOs["shadow"])));
}

void StencilApp::CreatePSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;
	ZeroMemory(&desc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	BuildOpaquePSO(desc);
	BuildMirrorPSO(desc);
	auto transparent = BuildTransparentPSO(desc);
	BuildReflectionPSO(desc);
	BuildShadowPSO(transparent);
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 1> StencilApp::GetStaticSamplers() const
{
	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(0,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		0,
		8);
	return { anisotropicWrap };
}

CD3DX12_GPU_DESCRIPTOR_HANDLE StencilApp::GetDescriptorHandleWithOffset(UINT && offset)
{
	return CD3DX12_GPU_DESCRIPTOR_HANDLE(mSrvCbvHeap->GetGPUDescriptorHandleForHeapStart(), offset, mCbvSrvDescriptorSize);
}

void StencilApp::OnResize()
{
	D3DApp::OnResize();
	XMMATRIX P = XMMatrixPerspectiveFovLH(XM_PIDIV4, AspectRatio(), mNearPlane, mFarPlane);
	XMStoreFloat4x4(&mProj, P);
}

void StencilApp::Update(const GameTimer & gt)
{
	OnKeyboardInput(gt);
	UpdateCamera(gt);

	mCurrentFrameResourceIndex = (mCurrentFrameResourceIndex + 1) % mNumFrameResources;
	mCurrentFrameResource = mFrameResources[mCurrentFrameResourceIndex].get();

	if (mCurrentFrameResource->Fence != 0 && mCurrentFrameResource->Fence < mFence->GetCompletedValue()) {
		HANDLE handle = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFrameResource->Fence, handle));
		WaitForSingleObject(handle, INFINITE);
		CloseHandle(handle);
	}

	UpdateObjectCBs(gt);
	UpdateMainPassCB(gt);
	UpdateReflectedPassCB(gt);
}

void StencilApp::UpdateCamera(const GameTimer & gt)
{
	mEyePos.x = mRadius*sinf(mPhi)*cosf(mTheta);
	mEyePos.y = mRadius*cosf(mPhi);
	mEyePos.z = mRadius*sinf(mPhi)*sinf(mTheta);

	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR eyePos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);

	XMMATRIX view = XMMatrixLookAtLH(eyePos, target, up);
	XMStoreFloat4x4(&mView, view);
}

void StencilApp::UpdateObjectCBs(const GameTimer & gt)
{
	auto currFrame = mCurrentFrameResource->ObjectCB.get();
	for (auto& rItem : mRenderItems) {
		if (rItem->NumFramesDirty > 0) {
			ObjectConstants oc;
			XMMATRIX w = XMLoadFloat4x4(&rItem->World);
			XMMATRIX tt = XMLoadFloat4x4(&rItem->TexTransform);
			XMStoreFloat4x4(&oc.World, XMMatrixTranspose(w));
			XMStoreFloat4x4(&oc.TexTransform, tt);
			currFrame->CopyData(rItem->ObjCBIndex, oc);
		}
	}

	auto currFrameMat = mCurrentFrameResource->MaterialCB.get();
	for (auto& materialTuple : mMaterials) {
		auto material = materialTuple.second.get();
		if (material->NumFramesDirty > 0) {
			MaterialConstants mc;
			mc.DiffuseAlbedo = material->DiffuseAlbedo;
			mc.FresnelR0 = material->FresnelR0;
			mc.Roughness = material->Roughness;
			XMMATRIX matTransform = XMLoadFloat4x4(&material->MatTransform);
			XMStoreFloat4x4(&mc.MatTransform, matTransform);
			currFrameMat->CopyData(material->MatCBIndex, mc);
			material->NumFramesDirty--;
		}
	}
}

void StencilApp::UpdateMainPassCB(const GameTimer & gt)
{
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX viewProj = XMMatrixMultiply(view, proj);

	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	mMainPassCB.EyePosW = mEyePos;
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();

	mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	auto numLights = _countof(mLights);
	for (UINT i = 0; i < numLights; ++i) {
		mMainPassCB.Lights[i].Direction = mLights[i].Direction;
		mMainPassCB.Lights[i].Strength = mLights[i].Strength;
	}


	mCurrentFrameResource->PassCB->CopyData(0, mMainPassCB);
}

void StencilApp::UpdateReflectedPassCB(const GameTimer& gt) {
	mReflectedPassCB = mMainPassCB;
	
	XMVECTOR mirrorPlane = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // xy plane
	XMMATRIX R = XMMatrixReflect(mirrorPlane);
	for (int i = 0; i < 3; ++i) {
		XMVECTOR lightDir = XMLoadFloat3(&mMainPassCB.Lights[i].Direction);
		XMVECTOR reflectedLightDir = XMVector3TransformNormal(lightDir, R);
		XMStoreFloat3(&mReflectedPassCB.Lights[i].Direction, reflectedLightDir);
	}

	// reflected pass stored in index 1
	auto currPassCB = mCurrentFrameResource->PassCB.get();
	currPassCB->CopyData(1, mReflectedPassCB);
}

void StencilApp::Draw(const GameTimer & gt)
{
	auto commandAlloc = mCurrentFrameResource->CmdListAlloc.Get();

	ThrowIfFailed(mCommandList->Reset(commandAlloc, mPSOs["opaque"].Get()));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);
	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::Black, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_STENCIL | D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* heaps[] = { mSrvCbvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(heaps), heaps);

	auto baseAddr = mCurrentFrameResource->PassCB->Resource()->GetGPUVirtualAddress();

	mCommandList->SetGraphicsRootConstantBufferView(2, baseAddr);
	DrawRenderItems(mRenderLayers[(UINT)RenderLayer::Opaque]);

	// draw into the stencil buffers
	mCommandList->OMSetStencilRef(1);
	mCommandList->SetPipelineState(mPSOs["markStencilMirrors"].Get());
	DrawRenderItems(mRenderLayers[(UINT)RenderLayer::Mirrors]);

	// update the pass cb
	mCommandList->SetGraphicsRootConstantBufferView(2, baseAddr + 1 * d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants)));
	// draw the reflection
	mCommandList->SetPipelineState(mPSOs["reflections"].Get());
	DrawRenderItems(mRenderLayers[(UINT)RenderLayer::Reflected]); 

	// restore stencil ref
	mCommandList->OMSetStencilRef(0);
	mCommandList->SetGraphicsRootConstantBufferView(2, baseAddr);
	// draw the mirrors
	mCommandList->SetPipelineState(mPSOs["transparent"].Get());
	DrawRenderItems(mRenderLayers[(UINT)RenderLayer::Transparent]);

	mCommandList->SetPipelineState(mPSOs["shadow"].Get());
	DrawRenderItems(mRenderLayers[(UINT)RenderLayer::Shadow]);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT
	));

	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// oresent and swap
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	mCurrentFrameResource->Fence = ++mCurrentFence;
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void StencilApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;
	SetCapture(mhMainWnd);
}

void StencilApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void StencilApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0) {
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

		mTheta += dx;
		mPhi += dy;
		mPhi = MathHelper::Clamp(mPhi, 0.1f, XM_PI - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0) {
		float dx = 0.5f*static_cast<float>(x - mLastMousePos.x);
		float dy = 0.5f*static_cast<float>(y - mLastMousePos.y);

		mRadius += dx - dy;

		mRadius = MathHelper::Clamp(mRadius, mNearPlane, mFarPlane);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void StencilApp::OnKeyboardInput(const GameTimer & gt)
{
	auto dt = gt.DeltaTime();
	bool dirty = false;
	bool rotDirty = false;
	bool scaleDirty = false;
	if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
		mSkullTMatrix._41 -= 1.0f*dt;
		dirty = true;
	}
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
		mSkullTMatrix._41 += 1.0f*dt;
		dirty = true;
	}
	if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
		mSkullTMatrix._43 -= 1.0f*dt;
		dirty = true;
	}
	if (GetAsyncKeyState(VK_UP) & 0x8000) {
		mSkullTMatrix._43 += 1.0f*dt;
		dirty = true;
	}
	if (GetAsyncKeyState('W') & 0x8000) {
		mSkullRotation += XMConvertToRadians(0.10f);
		rotDirty = true;
	}
	if (GetAsyncKeyState('A') & 0x8000) {
		mSkullRotation -= XMConvertToRadians(0.10f);
		rotDirty = true;
	}

	if (GetAsyncKeyState('G') & 0x8000) {
		mSkullScale += 0.25f*dt;
		scaleDirty = true;
	}

	if (GetAsyncKeyState('S') & 0x8000) {
		mSkullScale -= 0.25f*dt;
		scaleDirty = true;
	}

	if (scaleDirty) {
		mSkullSMatrix._11 = mSkullScale;
		mSkullSMatrix._22 = mSkullScale;
		mSkullSMatrix._33 = mSkullScale;
	}

	if (rotDirty) {
		XMStoreFloat4x4(&mSkullRMatrix, XMMatrixRotationY(mSkullRotation));
	}
	
	if (dirty || rotDirty || scaleDirty) {
		XMMATRIX world = XMLoadFloat4x4(&mSkullSMatrix)*XMLoadFloat4x4(&mSkullRMatrix)*
			XMLoadFloat4x4(&mSkullTMatrix);
		XMStoreFloat4x4(&mSkullRItem->World, world);
		XMStoreFloat4x4(&mReflectedSkullRItem->World, world*XMLoadFloat4x4(&mReflectionMatrix));
		XMStoreFloat4x4(&mSkullShadowRItem->World, world*XMLoadFloat4x4(&mShadowMatrix)*XMLoadFloat4x4(&mShadowOffset));
		mSkullRItem->NumFramesDirty = mNumFrameResources;
		mReflectedSkullRItem->NumFramesDirty = mNumFrameResources;
		mSkullShadowRItem->NumFramesDirty = mNumFrameResources;
	}
}

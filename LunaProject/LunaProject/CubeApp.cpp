#include "stdafx.h"
#include "CubeApp.h"

CubeApp::CubeApp(HINSTANCE hInstance) : D3DApp(hInstance)
{
	mCamera = std::make_unique<Camera>();
	mCamera->SetPosition(XMFLOAT3(0.0f, 20.0f, 0.0f));
	mCubeMapSize = 256;
}

bool CubeApp::Initialize()
{
	if (!D3DApp::Initialize()) {
		return false;
	}

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	BuildCubeMaps();
	BuildCubeMapDsv();
	BuildObjectTextures();
	BuildGeometry();
	BuildRenderItems();
	BuildFrameResources();
	BuildDescriptorHeaps();
	BuildInstanceDescriptors();
	BuildDescriptors();
	CompileShadersAndInputLayout();
	BuildRootSignature();
	BuildCommonRootSignature();
	BuildPSOs();

	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* commandLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	FlushCommandQueue();
	return true;
}

void CubeApp::BuildGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(1.0f, 20, 30);
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(160.0f, 160.0f, 40, 40);
	GeometryGenerator::MeshData column = geoGen.CreateCylinder(3.0f, 2.0f, 10.0f, 12, 12);

	std::vector<Vertex> vertices;
	UINT numSphereVertices = (UINT)sphere.Vertices.size();
	UINT numGridVertices = (UINT)grid.Vertices.size();
	UINT numColumnVertices = (UINT)column.Vertices.size();
	UINT numVertices = numSphereVertices + numGridVertices + numColumnVertices;
	vertices.resize(numVertices);
	auto copyVertices = [&](const std::vector<GeometryGenerator::Vertex> vin, UINT offset) {
		for (UINT i = 0; i < (UINT)vin.size(); ++i) {
			vertices[i + offset] = { vin[i].Position, vin[i].Normal, vin[i].TexC };
		}
	};
	copyVertices(sphere.Vertices, 0);
	copyVertices(grid.Vertices, numSphereVertices);
	copyVertices(column.Vertices, numSphereVertices + numGridVertices);

	std::vector<std::uint16_t> indices;
	UINT numSphereIndices = (UINT)sphere.GetIndices16().size();
	UINT numGridIndices = (UINT)grid.GetIndices16().size();
	UINT numColumnIndices = (UINT)column.GetIndices16().size();
	UINT numIndices = numSphereIndices + numGridIndices + numColumnIndices;
	indices.resize(numIndices);
	auto copyIndices = [&](const std::vector<std::uint16_t> iin, UINT offset) {
		for (UINT i = 0; i < (UINT)iin.size(); ++i) {
			indices[i + offset] = iin[i];
		}
	};
	copyIndices(sphere.GetIndices16(), 0);
	copyIndices(grid.GetIndices16(), numSphereIndices);
	copyIndices(column.GetIndices16(), numSphereIndices + numGridIndices);

	UINT vbByteSize = sizeof(Vertex) * numVertices;
	UINT ibByteSize = sizeof(std::uint16_t) * numIndices;

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "AppObjects";
	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));

	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), vertices.data(),
		vbByteSize, geo->VertexBufferUploader);
	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), indices.data(),
		ibByteSize, geo->IndexBufferUploader);


	SubmeshGeometry submesh = {};
	submesh.IndexCount = numSphereIndices;
	submesh.StartIndexLocation = 0u;
	submesh.BaseVertexLocation = 0u;
	geo->DrawArgs["skySphere"] = submesh;

	submesh.IndexCount = numGridIndices;
	submesh.StartIndexLocation = numSphereIndices;
	submesh.BaseVertexLocation = numSphereVertices;
	geo->DrawArgs["grid"] = submesh;

	submesh.IndexCount = numColumnIndices;
	submesh.StartIndexLocation = numSphereIndices + numGridIndices;
	submesh.BaseVertexLocation = numSphereVertices + numGridVertices;
	geo->DrawArgs["column"] = submesh;

	geo->VertexByteStride = sizeof(Vertex);
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexBufferByteSize = ibByteSize;

	mGeometries[geo->Name] = std::move(geo);
}

void CubeApp::BuildRenderItems()
{
	auto rItem = std::make_unique<RenderItem>();
	rItem->Geo = mGeometries["AppObjects"].get();
	rItem->Name = "skySphere";
	rItem->GeoName = "skySphere";
	rItem->PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	rItem->IndexCount = rItem->Geo->DrawArgs[rItem->GeoName].IndexCount;
	rItem->StartIndexLocation = rItem->Geo->DrawArgs[rItem->GeoName].StartIndexLocation;
	rItem->InstanceCount = 1;
	rItem->StartInstanceLocation = 0;
	rItem->BaseVertexLocation = rItem->Geo->DrawArgs[rItem->GeoName].BaseVertexLocation;
	InstanceData data;
	rItem->Instances.emplace_back(data);
	rItem->IgnoreBoundingBox = true;
	mRenderItemInstanceCounts[rItem->Name] = rItem->InstanceCount; 
	mRenderLayer[RenderLayers::Skies].emplace_back(rItem.get());
	mAllRenderItems.emplace_back(std::move(rItem));

	rItem = std::make_unique<RenderItem>();
	rItem->Name = "grid";
	rItem->GeoName = "grid";
	rItem->Geo = mGeometries["AppObjects"].get();
	rItem->PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	rItem->IndexCount = rItem->Geo->DrawArgs[rItem->GeoName].IndexCount;
	rItem->StartIndexLocation = rItem->Geo->DrawArgs[rItem->GeoName].StartIndexLocation;
	rItem->InstanceCount = 1;
	rItem->StartInstanceLocation = 0;
	rItem->IgnoreBoundingBox = true;
	rItem->BaseVertexLocation = rItem->Geo->DrawArgs[rItem->GeoName].BaseVertexLocation;
	data.DiffuseMapIndex = 0;
	rItem->DiffuseMapIndex = 0;
	XMStoreFloat4x4(&data.TexTransform, XMMatrixScaling(10.0f, 10.0f, 1.0f));
	rItem->Instances.emplace_back(std::move(data));
	mRenderItemInstanceCounts[rItem->Name] = rItem->InstanceCount;
	mRenderLayer[RenderLayers::Opaque].emplace_back(rItem.get());
	mAllRenderItems.emplace_back(std::move(rItem));

	rItem = std::make_unique<RenderItem>();
	rItem->Name = "column";
	rItem->GeoName = "column";
	rItem->Geo = mGeometries["AppObjects"].get();
	rItem->PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	rItem->IndexCount = rItem->Geo->DrawArgs[rItem->GeoName].IndexCount;
	rItem->StartIndexLocation = rItem->Geo->DrawArgs[rItem->GeoName].StartIndexLocation;
	rItem->InstanceCount = 10;
	rItem->StartInstanceLocation = 0;
	rItem->IgnoreBoundingBox = true;
	rItem->BaseVertexLocation = rItem->Geo->DrawArgs[rItem->GeoName].BaseVertexLocation;
	data.DiffuseMapIndex = 0;
	rItem->DiffuseMapIndex = 0;

	auto sphereRItem = std::make_unique<RenderItem>();
	sphereRItem->Name = "sphere";
	sphereRItem->GeoName = "skySphere";
	sphereRItem->Geo = mGeometries["AppObjects"].get();
	sphereRItem->PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	sphereRItem->IndexCount = sphereRItem->Geo->DrawArgs[sphereRItem->GeoName].IndexCount;
	sphereRItem->StartIndexLocation = sphereRItem->Geo->DrawArgs[sphereRItem->GeoName].StartIndexLocation;
	sphereRItem->InstanceCount = 11;
	sphereRItem->StartInstanceLocation = 0;
	sphereRItem->IgnoreBoundingBox = true;
	sphereRItem->BaseVertexLocation = sphereRItem->Geo->DrawArgs[sphereRItem->GeoName].BaseVertexLocation;
	data.DiffuseMapIndex = 1;
	sphereRItem->DiffuseMapIndex = 0;
	
	XMStoreFloat4x4(&data.TexTransform, XMMatrixIdentity());
	for (int i = 0, j = 0; i < 5; ++i, j += 2) {
		XMStoreFloat4x4(&data.World, XMMatrixTranslation(-20.0f, 5.0f, (i - 2) * 15.0f));
		rItem->Instances.emplace_back(data);
		XMStoreFloat4x4(&data.World, XMMatrixTranslation(+20.0f, 5.0f, (i -2) * 15.0f));
		rItem->Instances.emplace_back(data);

		XMStoreFloat4x4(&data.World, XMMatrixScaling(2.0f, 2.0f, 2.0f)*XMMatrixTranslation(-20.0f, 12.0f, (i - 2) * 15.0f));
		sphereRItem->Instances.emplace_back(data);
		spherePositions[j] = data.World;
		XMStoreFloat4x4(&data.World, XMMatrixScaling(2.0f, 2.0f, 2.0f)*XMMatrixTranslation(+20.0f, 12.0f, (i - 2) * 15.0f));
		sphereRItem->Instances.emplace_back(data);
		spherePositions[j + 1] = data.World;
	}

	mPathRadius = 10.0f;
	XMStoreFloat4x4(&data.World, XMMatrixScaling(2.0f, 2.0f, 2.0f)*XMMatrixTranslation(mCenterSpherePos.x + mPathRadius,
		mCenterSpherePos.y, mCenterSpherePos.z + mPathRadius));
	sphereRItem->Instances.emplace_back(data);

	mRenderItemInstanceCounts[rItem->Name] = (UINT)rItem->Instances.size();
	mRenderItemInstanceCounts[sphereRItem->Name] = (UINT)sphereRItem->Instances.size();
	mRenderLayer[RenderLayers::Opaque].emplace_back(rItem.get());
	mAllRenderItems.emplace_back(std::move(rItem));

	mRenderLayer[RenderLayers::Opaque].emplace_back(sphereRItem.get());
	mAllRenderItems.emplace_back(std::move(sphereRItem));


	rItem = std::make_unique<RenderItem>();
	rItem->Geo = mGeometries["AppObjects"].get();
	rItem->Name = "reflector";
	rItem->GeoName = "skySphere";
	rItem->PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	rItem->IndexCount = rItem->Geo->DrawArgs[rItem->GeoName].IndexCount;
	rItem->StartIndexLocation = rItem->Geo->DrawArgs[rItem->GeoName].StartIndexLocation;
	rItem->InstanceCount = 1;
	rItem->StartInstanceLocation = 0;
	rItem->BaseVertexLocation = rItem->Geo->DrawArgs[rItem->GeoName].BaseVertexLocation;
	XMStoreFloat4x4(&data.World, XMMatrixScaling(3.0f, 3.0f, 3.0f)*XMMatrixTranslation(mCenterSpherePos.x, mCenterSpherePos.y, mCenterSpherePos.z));
	rItem->Instances.emplace_back(data);
	rItem->IgnoreBoundingBox = true;
	mRenderItemInstanceCounts[rItem->Name] = rItem->InstanceCount;
	mRenderLayer[(UINT)RenderLayers::Reflectors].emplace_back(rItem.get());
	mAllRenderItems.emplace_back(std::move(rItem));
}

void CubeApp::BuildDescriptorHeaps()
{
	// InstanceData structured buffer per render item
	auto numRenderItems = mAllRenderItems.size();
	mPassCbOffset = (int)(numRenderItems * mNumFrameResources);
	mObjectTextureSrvOffset = mPassCbOffset + (mNumPassConstantBuffers * mNumFrameResources);
	mCubeMapSrvOffset = mObjectTextureSrvOffset + mObjectTextures.size();
	mDynamicCubeMapSrvOffset = mCubeMapSrvOffset + mCubeMaps.size();
	auto numDescriptors = mDynamicCubeMapSrvOffset + 1; // <- dynamic cube map srv stored in last descriptor
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.NumDescriptors = (UINT)numDescriptors;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0u;

	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mCbvSrvHeap)));
}

void CubeApp::BuildDescriptors()
{
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	UINT passCbByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
	cbvDesc.SizeInBytes = passCbByteSize;
	for (UINT i = 0; i < (UINT)mNumFrameResources; ++i) {
		auto passResource = mFrameResources[i]->PassCB->Resource();
		for (UINT j = 0; j < mNumPassConstantBuffers; ++j) {
			cbvDesc.BufferLocation = passResource->GetGPUVirtualAddress() + j * passCbByteSize;
			auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvSrvHeap->GetCPUDescriptorHandleForHeapStart(), 
				mPassCbOffset + i * mNumPassConstantBuffers + j, mCbvSrvDescriptorSize);
			md3dDevice->CreateConstantBufferView(&cbvDesc, handle);
		}
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	auto dHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvSrvHeap->GetCPUDescriptorHandleForHeapStart(), mObjectTextureSrvOffset, mCbvSrvDescriptorSize);
	for (UINT i = 0; i < (UINT)mObjectTextures.size(); ++i) {
		auto resource = mObjectTextures[mObjectTextureNames[i]]->Resource.Get();
		srvDesc.Format = resource->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = resource->GetDesc().MipLevels;
		md3dDevice->CreateShaderResourceView(resource, &srvDesc, dHandle);
		dHandle = dHandle.Offset(1, mCbvSrvDescriptorSize);
	}
	
	D3D12_SHADER_RESOURCE_VIEW_DESC cubeSrvDesc = {};
	cubeSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	cubeSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	cubeSrvDesc.TextureCube.MostDetailedMip = 0;
	cubeSrvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	dHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvSrvHeap->GetCPUDescriptorHandleForHeapStart(), mCubeMapSrvOffset, mCbvSrvDescriptorSize);
	for (UINT i = 0; i < (UINT)mCubeMaps.size(); ++i) {
		auto resource = mCubeMaps[mCubeMapNames[i]]->Resource.Get();
		cubeSrvDesc.Format = resource->GetDesc().Format;
		cubeSrvDesc.TextureCube.MipLevels = resource->GetDesc().MipLevels;
		md3dDevice->CreateShaderResourceView(resource, &cubeSrvDesc, dHandle);
		dHandle = dHandle.Offset(1, mCbvSrvDescriptorSize);
	}
	
	CD3DX12_CPU_DESCRIPTOR_HANDLE cubeRtvHandles[6];
	for (UINT i = 0; i < 6; ++i) {
		cubeRtvHandles[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
			i + SwapChainBufferCount, mRtvDescriptorSize);
	}

	mDynamicCubeMap = std::make_unique<DynamicCubeMap>(md3dDevice.Get(), mCubeMapSize, mCubeMapSize, mBackBufferFormat);
	mDynamicCubeMap->BuildDescriptors(dHandle,
		CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvSrvHeap->GetGPUDescriptorHandleForHeapStart(),
			mCbvSrvHeap->GetDesc().NumDescriptors - 1, mCbvSrvDescriptorSize),
		cubeRtvHandles);

	// build DSV
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Format = mDepthStencilFormat;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.Texture2D.MipSlice = 0;

	md3dDevice->CreateDepthStencilView(mCubeMapDsv.Get(), &dsvDesc,
		CD3DX12_CPU_DESCRIPTOR_HANDLE(mDsvHeap->GetCPUDescriptorHandleForHeapStart(),
			1, mDsvDescriptorSize));
}

void CubeApp::BuildInstanceDescriptors()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.StructureByteStride = sizeof(InstanceData);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	//					0		1		2		3		4
	//					5		6		7		8		9
	//					10		11		12		13		14
	// renderItems:		sky / grid / column / sphere / reflector
	// heap:			sky / grid / column / sphere
	// every render item has an 
	for (UINT i = 0; i < (UINT)mNumFrameResources; ++i) {
		UINT j = 0;
		for (auto& rItem : mAllRenderItems) {
			auto res = mFrameResources[i]->RenderItemInstanceCBs[rItem->Name]->Resource();
			srvDesc.Buffer.NumElements = mRenderItemInstanceCounts[rItem->Name];
			md3dDevice->CreateShaderResourceView(res, &srvDesc,
				CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvSrvHeap->GetCPUDescriptorHandleForHeapStart(),
					(UINT)(i * mAllRenderItems.size() + j), mCbvSrvDescriptorSize));
			++j;
		}
	}
}

void CubeApp::BuildMaterials()
{
	auto mat = std::make_unique<Material>(mNumFrameResources);
	mat->MatCBIndex = 0;
	mat->DiffuseAlbedo = { 0.2f, 0.8f, 0.2f, 1.0f };
	mat->Name = "grass";
	mMaterials[mat->Name] = std::move(mat);
}

void CubeApp::BuildCubeAxes()
{
	mCubeAxes[0] = { 1.0f, 0.0f, 0.0f };
	mCubeAxes[1] = { -1.0f, 0.0f, 0.0f };
	mCubeAxes[2] = { 0.0f, 1.0f, 0.0f };
	mCubeAxes[3] = { 0.0f, -1.0f, 0.0f };
	mCubeAxes[4] = { 0.0f, 0.0f, 1.0f };
	mCubeAxes[5] = { 0.0f, 0.0f, -1.0f };
}

void CubeApp::CompileShadersAndInputLayout()
{
	D3D_SHADER_MACRO lights[] = {
		"NUM_DIR_LIGHTS", "0",
		"NUM_SPOT_LIGHTS", "0",
		"NUM_POINT_LIGHTS", "0",
		NULL, NULL
	};
	mShaders["EnvObjVS"] = d3dUtil::CompileShader(L"Shaders\\EnvObject.hlsl", lights, "VS", "vs_5_1");
	mShaders["EnvObjPS"] = d3dUtil::CompileShader(L"Shaders\\EnvObject.hlsl", lights, "PS", "ps_5_1");
	mShaders["SkyCubeVS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", lights, "VS", "vs_5_1");
	mShaders["SkyCubePS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", lights, "PS", "ps_5_1");

	D3D_SHADER_MACRO macros[] = {
		"NORMAL_RENDER", "0",
		"NUM_DIR_LIGHTS", "1",
		"NUM_SPOT_LIGHTS", "0",
		"NUM_POINT_LIGHTS", "0",
		NULL, NULL
	};

	mShaders["ReflectorVS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", macros, "VS", "vs_5_1");
	mShaders["ReflectorPS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", macros, "PS", "ps_5_1");

	mInputLayout[(UINT)InputLayoutType::Opaque] = {
		{ "POSITION", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, 0u, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u },
		{ "NORMAL", 0u, DXGI_FORMAT_R32G32B32_FLOAT, 0u, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u },
		{ "TEXCOORD", 0u, DXGI_FORMAT_R32G32_FLOAT, 0u, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u }
	};
	mInputLayout[(UINT)InputLayoutType::Sky] = mInputLayout[(UINT)InputLayoutType::Opaque];
}

void CubeApp::BuildRootSignature()
{
	const int numRootParams = 2;
	auto srvTable0 = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	auto cbvTable1 = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

	CD3DX12_ROOT_PARAMETER rootParams[numRootParams];
	rootParams[0].InitAsDescriptorTable(1, &srvTable0);
	rootParams[1].InitAsDescriptorTable(1, &cbvTable1);

	auto rootSigDesc = CD3DX12_ROOT_SIGNATURE_DESC(numRootParams, rootParams,
		0u, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> sig;
	ComPtr<ID3DBlob> err;

	auto handle = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &sig, &err);

	if (err != nullptr) {
		::OutputDebugStringA((char*)err->GetBufferPointer());
	}

	ThrowIfFailed(handle);

	ThrowIfFailed(md3dDevice->CreateRootSignature(0u, sig->GetBufferPointer(),
		sig->GetBufferSize(), IID_PPV_ARGS(&mRootSignature)));
}

void CubeApp::BuildCommonRootSignature()
{
	const int numRootParams = 5;
	mPassCbvRootParamIndex = 0;
	CD3DX12_DESCRIPTOR_RANGE cbvTable0(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);		// Pass Constants
	mCubeMapSrvRootParamIndex = 1;
	CD3DX12_DESCRIPTOR_RANGE srvTable00(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);	// Cube Map
	CD3DX12_DESCRIPTOR_RANGE srvTable01(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 1);	// Material Data
	mInstanceDataRootParamIndex = 3;
	CD3DX12_DESCRIPTOR_RANGE srvTable1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);		// Instance Data
	mObjectTextureRootParamIndex = 4;
	CD3DX12_DESCRIPTOR_RANGE srvTable2(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);		// Textures



	CD3DX12_ROOT_PARAMETER rootParams[numRootParams];
	rootParams[mPassCbvRootParamIndex].InitAsDescriptorTable(1, &cbvTable0);
	rootParams[mCubeMapSrvRootParamIndex].InitAsDescriptorTable(1, &srvTable00);
	rootParams[2].InitAsDescriptorTable(1, &srvTable01);
	rootParams[mInstanceDataRootParamIndex].InitAsDescriptorTable(1, &srvTable1);
	rootParams[mObjectTextureRootParamIndex].InitAsDescriptorTable(1, &srvTable2);
	auto samplers = GetStaticSamplers();

	auto rootSigDesc = CD3DX12_ROOT_SIGNATURE_DESC(numRootParams, rootParams, (UINT)samplers.size(), samplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> rootSig;
	ComPtr<ID3DBlob> error;
	
	auto handle = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSig, &error);

	if (error != nullptr)
		::OutputDebugStringA((char*)error->GetBufferPointer());
	
	ThrowIfFailed(handle);

	ThrowIfFailed(md3dDevice->CreateRootSignature(0u, rootSig->GetBufferPointer(), rootSig->GetBufferSize(),
		IID_PPV_ARGS(&mCommonRootSignature)));

}

void CubeApp::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;
	ZeroMemory(&desc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

	desc.pRootSignature = mCommonRootSignature.Get();
	desc.VS = { reinterpret_cast<BYTE*>(mShaders["EnvObjVS"]->GetBufferPointer()),
		(UINT)mShaders["EnvObjVS"]->GetBufferSize() };
	desc.PS = { reinterpret_cast<BYTE*>(mShaders["EnvObjPS"]->GetBufferPointer()),
		(UINT)mShaders["EnvObjPS"]->GetBufferSize() };

	desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	desc.SampleMask = UINT_MAX;
	auto rState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	rState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	rState.CullMode = D3D12_CULL_MODE_NONE;
	desc.RasterizerState = rState;
	desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	desc.InputLayout = { mInputLayout[(UINT)InputLayoutType::Opaque].data(),
		(UINT)mInputLayout[(UINT)InputLayoutType::Opaque].size()
	};
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = mBackBufferFormat;
	desc.DSVFormat = mDepthStencilFormat;
	desc.SampleDesc.Quality = m4xMsaaState ? m4xMsaaQuality - 1 : 0;
	desc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	desc.NodeMask = 0u;
	desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&mPSOs["wireframe"])));

	auto opaqueDesc = desc;
	opaqueDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	opaqueDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaqueDesc, IID_PPV_ARGS(&mPSOs["opaque"])));

	auto skyDesc = desc;
	skyDesc.pRootSignature = mCommonRootSignature.Get();
	skyDesc.VS = { reinterpret_cast<BYTE*>(mShaders["SkyCubeVS"]->GetBufferPointer()),
		(UINT)mShaders["SkyCubeVS"]->GetBufferSize() };
	skyDesc.PS = { reinterpret_cast<BYTE*>(mShaders["SkyCubePS"]->GetBufferPointer()),
		(UINT)mShaders["SkyCubePS"]->GetBufferSize() };
	skyDesc.InputLayout = { mInputLayout[(UINT)InputLayoutType::Sky].data(),
		(UINT)mInputLayout[(UINT)InputLayoutType::Sky].size()
	};

	skyDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	skyDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
	skyDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&skyDesc, IID_PPV_ARGS(&mPSOs["sky"])));

	auto reflectorDesc = opaqueDesc;
	reflectorDesc.pRootSignature = mCommonRootSignature.Get();
	reflectorDesc.VS = { reinterpret_cast<BYTE*>(mShaders["ReflectorVS"]->GetBufferPointer()),
		(UINT)mShaders["ReflectorVS"]->GetBufferSize() };
	reflectorDesc.PS = { reinterpret_cast<BYTE*>(mShaders["ReflectorPS"]->GetBufferPointer()),
		(UINT)mShaders["ReflectorPS"]->GetBufferSize() };
	reflectorDesc.InputLayout = { mInputLayout[(UINT)InputLayoutType::Sky].data(),
		(UINT)mInputLayout[(UINT)InputLayoutType::Sky].size()
	};

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&reflectorDesc, IID_PPV_ARGS(&mPSOs["reflector"])));
}

void CubeApp::BuildFrameResources()
{
	for (UINT i = 0; i < (UINT)mNumFrameResources; ++i) {
		mFrameResources.emplace_back(std::make_unique<FrameResource>(
			md3dDevice.Get(), mNumPassConstantBuffers, mRenderItemInstanceCounts, (UINT)mMaterials.size()));
	}
}

void CubeApp::BuildCubeMaps()
{
	struct ca {
		std::wstring fileName;
		std::string name;
	};

	std::vector<ca> cubeApps = {
		{ L"Textures/grasscube1024.dds", "grassSky" },
		{ L"Textures/desertcube1024.dds", "desertSky" },
		{ L"Textures/snowcube1024.dds", "snowSky" },
		{ L"Textures/sunsetcube1024.dds", "sunsetSky"}
	};

	for (auto& cApp : cubeApps)
	{
		auto tex = std::make_unique<Texture>();
		tex->Name = cApp.name;
		tex->Filename = cApp.fileName;
		ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(), mCommandList.Get(),
			tex->Filename.c_str(), tex->Resource, tex->UploadHeap));
		mCubeMapNames.emplace_back(std::move(cApp.name));
		mCubeMaps[tex->Name] = std::move(tex);
	}
}

void CubeApp::BuildObjectTextures()
{
	struct ot {
		std::wstring fileName;
		std::string name;
	};

	std::vector<ot> objectTextures = {
		{ L"Textures/tile.dds", "tile" },
		{ L"Textures/stone.dds", "stone" }
	};

	for (auto& oTexture : objectTextures)
	{
		auto tex = std::make_unique<Texture>();
		tex->Name = oTexture.name;
		tex->Filename = oTexture.fileName;
		ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(), mCommandList.Get(),
			tex->Filename.c_str(), tex->Resource, tex->UploadHeap));
		mObjectTextureNames.emplace_back(std::move(oTexture.name));
		mObjectTextures[tex->Name] = std::move(tex);
	}
}

void CubeApp::BuildCubeMapDsv()
{
	D3D12_RESOURCE_DESC dsvDesc;
	dsvDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	dsvDesc.Format = mDepthStencilFormat;
	dsvDesc.Alignment = 0u;
	dsvDesc.Width = mCubeMapSize;
	dsvDesc.Height = mCubeMapSize;
	dsvDesc.DepthOrArraySize = 1;
	dsvDesc.MipLevels = 1;
	dsvDesc.SampleDesc.Count = 1;
	dsvDesc.SampleDesc.Quality = 0;
	dsvDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	dsvDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = mDepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;

	ThrowIfFailed(md3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE, &dsvDesc, D3D12_RESOURCE_STATE_COMMON, &optClear, IID_PPV_ARGS(&mCubeMapDsv)));

	mCommandList->ResourceBarrier(1, 
		&CD3DX12_RESOURCE_BARRIER::Transition(mCubeMapDsv.Get(),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_DEPTH_WRITE));
}

void CubeApp::UpdateLights()
{
	Light dirLight;
	XMVECTOR dir = -MathHelper::SphericalToCartesian(1.0f, mSunPhi, XM_PIDIV4);
	XMStoreFloat3(&mMainPassCB.Lights[0].Direction, dir);
	mMainPassCB.Lights[0].Strength = { 1.0f, 1.0f, 0.9f };
}

void CubeApp::UpdateCubeCams(float x, float y, float z) {
	XMFLOAT3 targets[6] = {
		XMFLOAT3(x + 1.0f, y, z),
		XMFLOAT3(x - 1.0f, y, z),
		XMFLOAT3(x, y + 1.0f, z),
		XMFLOAT3(x, y - 1.0f, z),
		XMFLOAT3(x, y, z + 1.0f),
		XMFLOAT3(x, y, z - 1.0f)
	};

	XMFLOAT3 ups[6] = {
		XMFLOAT3(0.0f, 1.0f, 0.0f),
		XMFLOAT3(0.0f, 1.0f, 0.0f),
		XMFLOAT3(0.0f, 0.0f, -1.0f),
		XMFLOAT3(0.0f, 0.0f, 1.0f),
		XMFLOAT3(0.0f, 1.0f, 0.0f),
		XMFLOAT3(0.0f, 1.0f, 0.0f)
	};

	XMFLOAT3 center = XMFLOAT3(x, y, z);

	for (UINT i = 0; i < 6; ++i) {
		mCubeCameras[i].LookAt(center, targets[i], ups[i]);
		mCubeCameras[i].SetLens(XM_PIDIV2, 1.0f, 1.0f, 1000.0f);
		mCubeCameras[i].UpdateViewMatrix();
	}
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> CubeApp::GetStaticSamplers() const
{
	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0,
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP
	);

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1,
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP
	);

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP
	);

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP
	);

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		0.0f,
		8
	);



	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		0.0f,
		8
	);

	return { pointWrap, pointClamp, linearWrap, linearClamp, anisotropicWrap, anisotropicClamp };
}

void CubeApp::AnimateSphere(RenderItem* renderItem, const GameTimer& gt) {
	static float posT = 0.0f;
	static float transT = 0.0f;
	// 90 degrees in 1 second
	posT += XM_PIDIV2*gt.DeltaTime();
	
	// spheres will rise and fall according to sine wave completing every 4 seconds
	transT += 5.0*gt.DeltaTime()*sinf(gt.TotalTime());


	for (UINT i = 0; i < renderItem->Instances.size() - 1; ++i) {
		XMMATRIX worldPos = XMLoadFloat4x4(&spherePositions[i]);
		XMMATRIX scale = XMMatrixScaling(spherePositions[i]._11, spherePositions[i]._22, spherePositions[i]._33);
		XMMATRIX translation = XMMatrixTranslation(spherePositions[i]._41, spherePositions[i]._42 + transT + 0.1f, spherePositions[i]._43);
		XMStoreFloat4x4(&renderItem->Instances[i].World,
			scale*XMMatrixRotationY(posT)*translation);
	}

	XMFLOAT3 dir = XMFLOAT3(mPathRadius*cosf(posT), 0.0f, mPathRadius*sinf(posT));
	XMFLOAT3 pos = XMFLOAT3(mCenterSpherePos.x + dir.x, mCenterSpherePos.y, mCenterSpherePos.z + dir.z);
	XMStoreFloat4x4(&renderItem->Instances[renderItem->InstanceCount - 1].World,
		XMMatrixScaling(2.0f, 2.0f, 2.0f)*XMMatrixRotationY(posT)*XMMatrixTranslation(pos.x, pos.y, pos.z));
	++renderItem->NumFramesDirty;
}


void CubeApp::Update(const GameTimer& gt)
{
	OnKeyboardInput(gt);
	mCurrFrameIndex = (mCurrFrameIndex + 1) % mNumFrameResources;
	mCurrentFrameResource = mFrameResources[mCurrFrameIndex].get();

	if (mCurrentFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrentFrameResource->Fence) {
		auto eventHandle = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
		mFence->SetEventOnCompletion(mCurrentFrameResource->Fence, eventHandle);
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	if (mCamera->UpdateViewMatrix()) {
		MarkRenderItemsDirty();
	};

	auto opaqueCount = mRenderLayer[(int)RenderLayers::Opaque].size();
	AnimateSphere(mRenderLayer[(int)RenderLayers::Opaque][opaqueCount - 1], gt);
	UpdateInstances(gt);
	UpdateMainPassCB(gt);
	UpdateCubeCams(mCenterSpherePos.x, mCenterSpherePos.y, mCenterSpherePos.z);
	UpdateCubeMapPassCBs(gt);
}

void CubeApp::Draw(const GameTimer & gt)
{
	auto cmdAlloc = mCurrentFrameResource->CmdListAlloc;
	ThrowIfFailed(cmdAlloc->Reset());
	ThrowIfFailed(mCommandList->Reset(cmdAlloc.Get(), mPSOs["opaque"].Get()));

	mCommandList->SetGraphicsRootSignature(mCommonRootSignature.Get());
	ID3D12DescriptorHeap* descriptorHeaps[1] = { mCbvSrvHeap.Get() };

	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootDescriptorTable(mCubeMapSrvRootParamIndex,
		CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvSrvHeap->GetGPUDescriptorHandleForHeapStart(),
			mCubeMapSrvOffset + (UINT)mActiveSky, mCbvSrvDescriptorSize));

	DrawSceneToCubeMap();

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET));

	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	mCommandList->SetGraphicsRootDescriptorTable(mPassCbvRootParamIndex,
		CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvSrvHeap->GetGPUDescriptorHandleForHeapStart(),
			mPassCbOffset + mCurrFrameIndex * mNumPassConstantBuffers, mCbvSrvDescriptorSize));

	mCommandList->SetGraphicsRootDescriptorTable(mCubeMapSrvRootParamIndex, mDynamicCubeMap->Srv());
	mCommandList->SetPipelineState(mPSOs["reflector"].Get());
	DrawRenderItems(mRenderLayer[(UINT)RenderLayers::Reflectors], 4u);

	mCommandList->SetGraphicsRootDescriptorTable(mCubeMapSrvRootParamIndex,
		CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvSrvHeap->GetGPUDescriptorHandleForHeapStart(),
			mCubeMapSrvOffset + (UINT)mActiveSky, mCbvSrvDescriptorSize));
	mCommandList->SetPipelineState(mPSOs["opaque"].Get());
	DrawRenderItems(mRenderLayer[(UINT)RenderLayers::Opaque], 1u);


	mCommandList->SetPipelineState(mPSOs["sky"].Get());
	DrawRenderItems(mRenderLayer[(UINT)RenderLayers::Skies], 0u);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* commandLists[1] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	ThrowIfFailed(mSwapChain->Present(0, 0));

	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	mCurrentFrameResource->Fence = ++mCurrentFence;
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void CubeApp::DrawRenderItems(const std::vector<RenderItem*>& renderItems, UINT instanceOffset, CD3DX12_GPU_DESCRIPTOR_HANDLE* srvHandle)
{
	for (UINT i = 0; i < (UINT)renderItems.size(); ++i) {
		auto rItem = renderItems[i];
		if (srvHandle == nullptr) {
			mCommandList->SetGraphicsRootDescriptorTable(mObjectTextureRootParamIndex,
				CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvSrvHeap->GetGPUDescriptorHandleForHeapStart(),
					mObjectTextureSrvOffset + rItem->DiffuseMapIndex, mCbvSrvDescriptorSize));
		}
		else {
			mCommandList->SetGraphicsRootDescriptorTable(mObjectTextureRootParamIndex,
				*srvHandle);
		}
		mCommandList->SetGraphicsRootDescriptorTable(mInstanceDataRootParamIndex,
			CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvSrvHeap->GetGPUDescriptorHandleForHeapStart(),
				(UINT)mAllRenderItems.size() * mCurrFrameIndex + i + instanceOffset, mCbvSrvDescriptorSize));
		mCommandList->IASetVertexBuffers(0, 1, &rItem->Geo->VertexBufferView());
		mCommandList->IASetIndexBuffer(&rItem->Geo->IndexBufferView());
		mCommandList->IASetPrimitiveTopology(rItem->PrimitiveTopology);
		mCommandList->DrawIndexedInstanced(rItem->IndexCount, rItem->InstanceCount, rItem->StartIndexLocation, rItem->BaseVertexLocation, rItem->StartInstanceLocation);
	}
}

void CubeApp::UpdateInstances(const GameTimer & gt)
{
	XMMATRIX view = mCamera->View();
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	BoundingFrustum& cameraFrustum = mCamera->GetBoundingFrustum();
	BoundingFrustum localFrustum;
	for (auto& renderItem : mAllRenderItems) {
		if (renderItem->NumFramesDirty > 0) {
			UINT visibleInstanceCount = 0;
			UINT rItemInstances = 0;
			for (UINT i = 0; i < (UINT)renderItem->Instances.size(); ++i) {

				XMMATRIX world = XMLoadFloat4x4(&renderItem->Instances[i].World);
				XMMATRIX invWorld = XMMatrixInverse(&XMMatrixDeterminant(world), world);

				XMMATRIX invViewWorld = XMMatrixMultiply(invView, invWorld);
				cameraFrustum.Transform(localFrustum, invViewWorld);

				if (renderItem->IgnoreBoundingBox || localFrustum.Contains(renderItem->Bounds) != DirectX::DISJOINT) {
					InstanceData data = {};
					XMStoreFloat4x4(&data.World, XMMatrixTranspose(world));
					XMStoreFloat4x4(&data.TexTransform, XMMatrixTranspose(XMLoadFloat4x4(&renderItem->Instances[i].TexTransform)));
					data.DiffuseMapIndex = renderItem->Instances[i].DiffuseMapIndex;
					mCurrentFrameResource->RenderItemInstanceCBs[renderItem->Name]->CopyData(visibleInstanceCount++, data);
					++rItemInstances;
				}
			}
			renderItem->InstanceCount = visibleInstanceCount;
			renderItem->NumFramesDirty--;
		}
	}
}

void CubeApp::UpdateMainPassCB(const GameTimer & gt)
{
	XMMATRIX view = mCamera->View();
	XMMATRIX proj = mCamera->Proj();
	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(XMMatrixInverse(&XMMatrixDeterminant(view), view)));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(XMMatrixInverse(&XMMatrixDeterminant(proj), proj)));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj)));
	XMStoreFloat3(&mMainPassCB.EyePos, mCamera->GetPosition());
	mMainPassCB.RenderTargetSize = { (float)mClientWidth, (float)mClientHeight };
	mMainPassCB.InvRenderTargetSize = { 1.0f / mClientWidth, 1.0f / mClientHeight };
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();
	mMainPassCB.AmbientLight = { 1.0f, 1.0f, 1.0f, 1.0f };
	UpdateLights();
	mCurrentFrameResource->PassCB->CopyData(0, mMainPassCB);
}

void CubeApp::UpdateCubeMapPassCBs(const GameTimer & gt)
{
	PassConstants pc = {};
	for (UINT i = 0; i < 6; ++i) {

		XMMATRIX view = mCubeCameras[i].View();
		XMMATRIX proj = mCubeCameras[i].Proj();
		XMMATRIX viewProj = XMMatrixMultiply(view, proj);
		XMStoreFloat4x4(&pc.View, XMMatrixTranspose(view));
		XMStoreFloat4x4(&pc.Proj, XMMatrixTranspose(proj));
		XMStoreFloat4x4(&pc.InvView, XMMatrixTranspose(XMMatrixInverse(&XMMatrixDeterminant(view), view)));
		XMStoreFloat4x4(&pc.InvProj, XMMatrixTranspose(XMMatrixInverse(&XMMatrixDeterminant(proj), proj)));
		XMStoreFloat4x4(&pc.ViewProj, XMMatrixTranspose(viewProj));
		XMStoreFloat4x4(&pc.InvViewProj, XMMatrixTranspose(XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj)));
		XMStoreFloat3(&pc.EyePos, mCubeCameras[i].GetPosition());
		pc.RenderTargetSize = { (float)mCubeMapSize, (float)mCubeMapSize };
		pc.InvRenderTargetSize = { 1.0f / mCubeMapSize, 1.0f / mCubeMapSize };
		pc.NearZ = 1.0f;
		pc.FarZ = 1000.0f;
		pc.TotalTime = gt.TotalTime();
		pc.DeltaTime = gt.DeltaTime();
		pc.AmbientLight = { 1.0f, 1.0f, 1.0f, 1.0f };
		UpdateLights();
		mCurrentFrameResource->PassCB->CopyData(i + 1, pc);
	}
}

void CubeApp::DrawSceneToCubeMap()
{
	mCommandList->RSSetViewports(1, &mDynamicCubeMap->Viewport());
	mCommandList->RSSetScissorRects(1, &mDynamicCubeMap->ScissorRect());

	UINT mPassCBStart = mPassCbOffset + mNumPassConstantBuffers * mCurrFrameIndex + 1;
	auto passCbHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvSrvHeap->GetGPUDescriptorHandleForHeapStart(), mPassCBStart, mCbvSrvDescriptorSize);
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		mDynamicCubeMap->Resource(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
	for (int i = 0; i < 6; ++i) {
		mCommandList->ClearRenderTargetView(mDynamicCubeMap->Rtv(i), DirectX::Colors::LightSteelBlue, 0, nullptr);
		mCommandList->ClearDepthStencilView(mhCubeMapDsv, D3D12_CLEAR_FLAG_STENCIL | D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		
		mCommandList->OMSetRenderTargets(1, &mDynamicCubeMap->Rtv(i), true, &mhCubeMapDsv);
		mCommandList->SetGraphicsRootDescriptorTable(mPassCbvRootParamIndex, passCbHandle);

		DrawRenderItems(mRenderLayer[(UINT)RenderLayers::Opaque], 1u);

		mCommandList->SetPipelineState(mPSOs["sky"].Get());
		DrawRenderItems(mRenderLayer[(UINT)RenderLayers::Skies], 0u);

		mCommandList->SetPipelineState(mPSOs["opaque"].Get());
		passCbHandle = passCbHandle.Offset(1, mCbvSrvDescriptorSize);
	}

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		mDynamicCubeMap->Resource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void CubeApp::MarkRenderItemsDirty()
{
}

void CubeApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	SetCapture(mhMainWnd);
	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void CubeApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void CubeApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0) {
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

		mCamera->Rotate(dx);
		mCamera->Pitch(dy);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void CubeApp::OnResize()
{
	D3DApp::OnResize();
	mCamera->SetLens(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);

}

void CubeApp::CreateRtvAndDsvDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvDesc;
	rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDesc.NumDescriptors = SwapChainBufferCount + 6;
	rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvDesc.NodeMask = 0u;

	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&mRtvHeap)));

	D3D12_DESCRIPTOR_HEAP_DESC dsvDesc;
	dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvDesc.NumDescriptors = 2;
	dsvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvDesc.NodeMask = 0u;

	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&dsvDesc, IID_PPV_ARGS(&mDsvHeap)));
	mhCubeMapDsv = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDsvHeap->GetCPUDescriptorHandleForHeapStart(), 1, mDsvDescriptorSize);
}

void CubeApp::OnKeyboardInput(const GameTimer & gt)
{
	static float debounceTime = 0.1f;
	static float debounce = 0.0f;
	debounce += gt.DeltaTime();
	if (GetAsyncKeyState(VK_UP) & 0x8000) {
		mCamera->Walk(10.0f*gt.DeltaTime());
	}

	if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
		mCamera->Walk(-10.0f*gt.DeltaTime());
	}

	if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
		mCamera->Rotate(-1.0f*gt.DeltaTime());
	}

	if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
		mCamera->Rotate(1.0f*gt.DeltaTime());
	}

	if (GetAsyncKeyState('R') & 0x8000) {
		mSunPhi += 1.0f*gt.DeltaTime();
	}

	if (debounce > debounceTime) {
		if (GetAsyncKeyState('G') & 0x8000) {
			mActiveSky = SkyType::Grass;
			debounce = 0.0f;
		}
		else if (GetAsyncKeyState('D') & 0x8000) {
			mActiveSky = SkyType::Desert;
			debounce = 0.0f;
		}
		else if (GetAsyncKeyState('S') & 0x8000) {
			mActiveSky = SkyType::Snow;
			debounce = 0.0f;
		}
		else if (GetAsyncKeyState('U') & 0x8000) {
			mActiveSky = SkyType::Sunset;
			debounce = 0.0f;
		}
	}
}

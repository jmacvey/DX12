#include "stdafx.h"
#include "TessApp.h"

TessApp::TessApp(HINSTANCE hInstance) : D3DApp(hInstance) {
}

void TessApp::BuildRootSignature()
{
	const int numRootParams = 4;
	CD3DX12_ROOT_PARAMETER params[numRootParams];
	CD3DX12_DESCRIPTOR_RANGE cbvTable0(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // per object settings
	CD3DX12_DESCRIPTOR_RANGE cbvTable1(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1); // pass settings
	CD3DX12_DESCRIPTOR_RANGE cbvTable2(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2); // material settings
	CD3DX12_DESCRIPTOR_RANGE srvTable0(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // diffuse map

	params[0].InitAsDescriptorTable(1, &cbvTable0);
	params[1].InitAsDescriptorTable(1, &cbvTable1);
	params[2].InitAsDescriptorTable(1, &cbvTable2);
	params[3].InitAsDescriptorTable(1, &srvTable0);

	auto samplers = GetStaticSamplers();

	auto rootSigDesc = CD3DX12_ROOT_SIGNATURE_DESC(numRootParams, params, (UINT)samplers.size(), samplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> rootSig;
	ComPtr<ID3DBlob> error;

	auto hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSig, &error);

	if (error != nullptr) {
		::OutputDebugStringA((char*)error->GetBufferPointer());
	}

	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(0, rootSig->GetBufferPointer(), rootSig->GetBufferSize(),
		IID_PPV_ARGS(&mRootSignature)));
}

void TessApp::BuildShadersAndInputLayout()
{
	D3D_SHADER_MACRO landGeo[] = {
		"LAND_GEOMETRY", "0",
		NULL, NULL
	};

	D3D_SHADER_MACRO sphereGeo[] = {
		"SPHERE_GEOMETRY", "0",
		NULL, NULL
	};
	mShaders["PassThroughVS"] = d3dUtil::CompileShader(L"Shaders\\PassThrough.hlsl", nullptr, "VS", "vs_5_0");
	mShaders["PassThroughPS"] = d3dUtil::CompileShader(L"Shaders\\PassThrough.hlsl", nullptr, "PS", "ps_5_0");
	mShaders["PatchTessVS"] = d3dUtil::CompileShader(L"Shaders\\PatchTess.hlsl", nullptr, "VS", "vs_5_0");
	mShaders["PatchTessPS"] = d3dUtil::CompileShader(L"Shaders\\PatchTess.hlsl", nullptr, "PS", "ps_5_0");
	mShaders["PatchTessHS"] = d3dUtil::CompileShader(L"Shaders\\PatchTess.hlsl", nullptr, "HS", "hs_5_0");
	mShaders["PatchTessDS"] = d3dUtil::CompileShader(L"Shaders\\PatchTess.hlsl", nullptr, "DS", "ds_5_0");
	mShaders["BezierTessVS"] = d3dUtil::CompileShader(L"Shaders\\Bezier.hlsl", nullptr, "VS", "vs_5_0");
	mShaders["BezierTessHS"] = d3dUtil::CompileShader(L"Shaders\\Bezier.hlsl", nullptr, "HS", "hs_5_0");
	mShaders["BezierTessDS"] = d3dUtil::CompileShader(L"Shaders\\Bezier.hlsl", nullptr, "DS", "ds_5_0");
	mShaders["BezierTessPS"] = d3dUtil::CompileShader(L"Shaders\\Bezier.hlsl", nullptr, "PS", "ps_5_0");
	mShaders["TriTessVS"] = d3dUtil::CompileShader(L"Shaders\\TriTess.hlsl", nullptr, "VS", "vs_5_0");
	mShaders["TriTessHS"] = d3dUtil::CompileShader(L"Shaders\\TriTess.hlsl", nullptr, "HS", "hs_5_0");
	mShaders["TriTessDS"] = d3dUtil::CompileShader(L"Shaders\\TriTess.hlsl", landGeo, "DS", "ds_5_0");
	mShaders["TriTessPS"] = d3dUtil::CompileShader(L"Shaders\\TriTess.hlsl", nullptr, "PS", "ps_5_0");
	mShaders["TriTessDSSphere"] = d3dUtil::CompileShader(L"Shaders\\TriTess.hlsl", sphereGeo, "DS", "ds_5_0");
	mShaders["QuadBezierVS"] = d3dUtil::CompileShader(L"Shaders\\QuadraticBezier.hlsl", nullptr, "VS", "vs_5_0");
	mShaders["QuadBezierHS"] = d3dUtil::CompileShader(L"Shaders\\QuadraticBezier.hlsl", nullptr, "HS", "hs_5_0");
	mShaders["QuadBezierDS"] = d3dUtil::CompileShader(L"Shaders\\QuadraticBezier.hlsl", nullptr, "DS", "ds_5_0");
	mShaders["QuadBezierPS"] = d3dUtil::CompileShader(L"Shaders\\QuadraticBezier.hlsl", nullptr, "PS", "ps_5_0");
	mInputLayout = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void TessApp::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueDesc;
	ZeroMemory(&opaqueDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

	opaqueDesc.VS = {
		reinterpret_cast<BYTE*>(mShaders["PassThroughVS"]->GetBufferPointer()),
		(UINT)mShaders["PassThroughVS"]->GetBufferSize()
	};

	opaqueDesc.PS = {
		reinterpret_cast<BYTE*>(mShaders["PassThroughPS"]->GetBufferPointer()),
		(UINT)mShaders["PassThroughPS"]->GetBufferSize()
	};

	opaqueDesc.pRootSignature = mRootSignature.Get();
	opaqueDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaqueDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaqueDesc.SampleMask = UINT_MAX;
	auto rState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	//rState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	// rState.CullMode = D3D12_CULL_MODE_NONE;
	opaqueDesc.RasterizerState = rState;
	opaqueDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	opaqueDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaqueDesc.NumRenderTargets = 1;
	opaqueDesc.RTVFormats[0] = mBackBufferFormat;
	opaqueDesc.DSVFormat = mDepthStencilFormat;
	opaqueDesc.SampleDesc.Quality = m4xMsaaState ? m4xMsaaQuality - 1 : 0;
	opaqueDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaqueDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaqueDesc, IID_PPV_ARGS(&mPSOs["PassThrough"])));

	auto buildDesc = [&](const std::string& tessName, D3D12_PRIMITIVE_TOPOLOGY_TYPE topology) {
		auto desc = opaqueDesc;
		desc.VS = {
			reinterpret_cast<BYTE*>(mShaders[tessName + "VS"]->GetBufferPointer()),
			(UINT)mShaders[tessName + "VS"]->GetBufferSize()
		};
		desc.HS = {
			reinterpret_cast<BYTE*>(mShaders[tessName + "HS"]->GetBufferPointer()),
			(UINT)mShaders[tessName + "HS"]->GetBufferSize()
		};
		desc.DS = {
			reinterpret_cast<BYTE*>(mShaders[tessName + "DS"]->GetBufferPointer()),
			(UINT)mShaders[tessName + "DS"]->GetBufferSize()
		};
		desc.PS = {
			reinterpret_cast<BYTE*>(mShaders[tessName + "PS"]->GetBufferPointer()),
			(UINT)mShaders[tessName + "PS"]->GetBufferSize()
		};
		desc.PrimitiveTopologyType = topology;
		ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&mPSOs[tessName])));
		return desc;
	};
	buildDesc("PatchTess", D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH);
	buildDesc("BezierTess", D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH);
	buildDesc("QuadBezier", D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH);
	auto triDesc = buildDesc("TriTess", D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH);

	triDesc.DS = {
		reinterpret_cast<BYTE*>(mShaders["TriTessDSSphere"]->GetBufferPointer()),
		(UINT)mShaders["TriTessDSSphere"]->GetBufferSize()
	};

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&triDesc, IID_PPV_ARGS(&mPSOs["TriTessSphere"])));

	mActivePSO = mPSOs["PatchTess"];
}

void TessApp::BuildFrameResources()
{
	for (UINT i = 0; i < (UINT)mNumFrameResources; ++i) {
		mFrameResources.emplace_back(std::make_unique<TessFrameResource>(
			md3dDevice.Get(), 1, mRenderItems.size(), mMaterials.size()));
	}
}

void TessApp::BuildDescriptorHeaps()
{
	auto numObjects = mNumFrameResources * (mRenderItems.size() + mMaterials.size() + 1) + mTextures.size();
	mMatCbOffset = (int)mRenderItems.size() * mNumFrameResources;
	mMainPassCbOffset = (int)(mRenderItems.size() + mMaterials.size()) * mNumFrameResources;
	mTextureOffset = mMainPassCbOffset + mNumFrameResources;
	D3D12_DESCRIPTOR_HEAP_DESC dHeapDesc = {};
	dHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	dHeapDesc.NumDescriptors = (UINT)numObjects;
	dHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	dHeapDesc.NodeMask = 0u;

	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&dHeapDesc, IID_PPV_ARGS(&mCbvHeap)));
}

void TessApp::BuildDescriptors()
{
	auto objCbByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	auto passCbByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstantsThrough));
	UINT numRenderItems = (UINT)mRenderItems.size();

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbDesc = {};
	for (UINT i = 0; i < (UINT)mNumFrameResources; ++i) {
		auto objectCB = mFrameResources[i]->ObjectCB->Resource();
		for (UINT j = 0; j < numRenderItems; ++j) {

			auto cbAddress = objectCB->GetGPUVirtualAddress();
			cbAddress += j * objCbByteSize;
			cbDesc.SizeInBytes = objCbByteSize;
			cbDesc.BufferLocation = cbAddress;
			auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart(),
				i*numRenderItems + j,
				mCbvSrvDescriptorSize);

			md3dDevice->CreateConstantBufferView(&cbDesc, handle);
		}
	}

	// 1 material per object
	auto materialCbSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));
	for (UINT i = 0; i < (UINT)mNumFrameResources; ++i) {
		auto matCB = mFrameResources[i]->MatCB->Resource();
		for (UINT j = 0; j < (UINT)mMaterials.size(); ++j) {
			auto cbAddress = matCB->GetGPUVirtualAddress();
			cbAddress += j * materialCbSize;
			cbDesc.SizeInBytes = materialCbSize;
			cbDesc.BufferLocation = cbAddress;

			auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart(),
				mMatCbOffset + i * mMaterials.size() + j, mCbvSrvDescriptorSize);
			md3dDevice->CreateConstantBufferView(&cbDesc, handle);
		}
	}

	for (UINT i = 0; i < (UINT)mNumFrameResources; ++i) {
		auto passCB = mFrameResources[i]->PassCB->Resource();
		auto cbAddress = passCB->GetGPUVirtualAddress();
		cbDesc.SizeInBytes = passCbByteSize;
		cbDesc.BufferLocation = cbAddress;
		auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart(),
			mMainPassCbOffset + i, mCbvSrvDescriptorSize);

		md3dDevice->CreateConstantBufferView(&cbDesc, handle);
	}
}

void TessApp::BuildShaderResourceViews()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.PlaneSlice = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	
	auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart(), mTextureOffset, mCbvSrvDescriptorSize);
	for (auto& tex : mTextures) {
		srvDesc.Texture2D.MipLevels = tex.second->Resource->GetDesc().MipLevels;
		srvDesc.Format = tex.second->Resource->GetDesc().Format;
		md3dDevice->CreateShaderResourceView(tex.second->Resource.Get(), &srvDesc, handle);
		handle.Offset(1, mCbvSrvDescriptorSize);
	}
}

void TessApp::BuildGridGeometry() {

	std::vector<Vertex> vertices = {
		{ { -10.0f, 0.0f, +10.0f } },
		{ { +10.0f, 0.0f, +10.0f } },
		{ { -10.0f, 0.0f, -10.0f } },
		{ { +10.0f, 0.0f, -10.0f } }
	};

	std::vector<uint16_t> indices = { 0, 1, 2, 3 };

	std::vector<Vertex> bezierVertices =
	{
		// Row 0
		{ { -10.0f, -10.0f, +15.0f } },
		{ { -5.0f,  0.0f, +15.0f } },
		{ { +5.0f,  0.0f, +15.0f } },
		{ { +10.0f, 0.0f, +15.0f } },

		// Row 1
		{ { -15.0f, 0.0f, +5.0f } },
		{ { -5.0f,  0.0f, +5.0f } },
		{ { +5.0f,  20.0f, +5.0f } },
		{ { +15.0f, 0.0f, +5.0f } } ,

		// Row 2
		{ { -15.0f, 0.0f, -5.0f } },
		{ { -5.0f,  0.0f, -5.0f } },
		{ { +5.0f,  0.0f, -5.0f } },
		{ { +15.0f, 0.0f, -5.0f } },

		// Row 3
		{ { -10.0f, 10.0f, -15.0f } },
		{ { -5.0f,  0.0f, -15.0f } },
		{ { +5.0f,  0.0f, -15.0f } },
		{ { +25.0f, 10.0f, -15.0f } }
	};

	std::vector<std::uint16_t> bezierIndices =
	{
		0, 1, 2, 3,
		4, 5, 6, 7,
		8, 9, 10, 11,
		12, 13, 14, 15
	};

	std::vector<Vertex> triVertices = {
		{ { -10.0f, 0.0f, +10.0f } },
		{ { -10.0f, 0.0f, -10.0f } },
		{ { +10.0f, 0.0f, -10.0f } },
		{ { +10.0f, 0.0f, +10.0f } }
	};

	std::vector<uint16_t> triIndices = { 0, 1, 2, 0, 2, 3 };

	std::vector<Vertex> quadBezierVertices =
	{
		// Row 0
		{ { -10.0f, 0.0f, -20.0f } },
		{ { 0.0f,  10.0f, -20.0f } },
		{ { +10.0f,  0.0f, -20.0f } },


		{ { -10.0f, 0.0f, 0.0f } },
		{ { 0.0f,  10.0f, 0.0f } },
		{ { +10.0f,  0.0f, 0.0f } },

		{ { -10.0f, 0.0f, +20.0f } },
		{ { 0.0f,  10.0f, +20.0f } },
		{ { +10.0f,  0.0f, +20.0f } },

	};



	std::vector<std::uint16_t> quadBezierIndices = {
		0, 1, 2, 3, 4,
		5, 6, 7, 8
	};

	auto geoGen = GeometryGenerator();
	auto icosahedron = geoGen.CreateGeosphere(2.0f, 0);

	UINT vertexSz = (UINT)vertices.size() + (UINT)bezierVertices.size() +
		(UINT)triVertices.size() + (UINT)icosahedron.Vertices.size() +
		(UINT)quadBezierVertices.size();
	auto vbByteSize = vertexSz * sizeof(Vertex);

	std::vector<Vertex> allVertices;
	allVertices.resize(vertexSz);

	auto copyVertices = [&](const std::vector<Vertex>& toCopy, UINT startIndex) {
		for (UINT i = startIndex; (i - startIndex) < (UINT)toCopy.size(); ++i) {
			allVertices[i] = toCopy[i - startIndex];
		}
	};

	auto mapVertices = [&](const std::vector<GeometryGenerator::Vertex>& toCopy, UINT startIndex) {
		for (UINT i = startIndex; (i - startIndex) < (UINT)toCopy.size(); ++i) {
			allVertices[i] = { { toCopy[i - startIndex].Position } };
		}
	};

	copyVertices(vertices, 0u);
	copyVertices(bezierVertices, (UINT)vertices.size());
	copyVertices(triVertices, (UINT)vertices.size() + (UINT)bezierVertices.size());
	mapVertices(icosahedron.Vertices, (UINT)vertices.size() + (UINT)bezierVertices.size() + (UINT)triVertices.size());
	copyVertices(quadBezierVertices, (UINT)vertices.size() + (UINT)bezierIndices.size() + (UINT)triVertices.size() + (UINT)icosahedron.Vertices.size());


	UINT szIndices = (UINT)indices.size() + (UINT)bezierIndices.size() + (UINT)triIndices.size() + (UINT)icosahedron.GetIndices16().size() + (UINT)quadBezierIndices.size();
	auto ibByteSize = szIndices * sizeof(std::uint16_t);

	std::vector<std::int16_t> allIndices;
	allIndices.resize(szIndices);

	auto copyIndices = [&](const std::vector<std::uint16_t>& toCopy, UINT startIndex) {
		for (UINT i = startIndex; (i - startIndex) < (UINT)toCopy.size(); ++i) {
			allIndices[i] = toCopy[i - startIndex];
		}
	};
	copyIndices(indices, 0u);
	copyIndices(bezierIndices, (UINT)indices.size());
	copyIndices(triIndices, (UINT)indices.size() + (UINT)bezierIndices.size());
	copyIndices(icosahedron.GetIndices16(), (UINT)indices.size() + (UINT)bezierIndices.size() + (UINT)triIndices.size());
	copyIndices(quadBezierIndices, (UINT)indices.size() + (UINT)bezierIndices.size() + (UINT)triIndices.size() + (UINT)icosahedron.GetIndices16().size());

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "grid";
	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = (UINT)vbByteSize;
	geo->IndexBufferByteSize = (UINT)ibByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));

	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), allVertices.data(), vbByteSize);
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), allIndices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), allVertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), allIndices.data(), ibByteSize, geo->IndexBufferUploader);

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["grid"] = submesh;

	submesh.IndexCount = (UINT)bezierIndices.size();
	submesh.StartIndexLocation = (UINT)indices.size();
	submesh.BaseVertexLocation = (UINT)vertices.size();

	geo->DrawArgs["bezier"] = submesh;

	submesh.IndexCount = (UINT)triIndices.size();
	submesh.StartIndexLocation = (UINT)indices.size() + (UINT)bezierIndices.size();
	submesh.BaseVertexLocation = (UINT)vertices.size() + (UINT)bezierVertices.size();

	geo->DrawArgs["triGrid"] = submesh;

	submesh.IndexCount = (UINT)icosahedron.GetIndices16().size();
	submesh.StartIndexLocation = (UINT)indices.size() + (UINT)bezierIndices.size() + (UINT)triIndices.size();
	submesh.BaseVertexLocation = (UINT)vertices.size() + (UINT)bezierVertices.size() + (UINT)triVertices.size();

	geo->DrawArgs["sphere"] = submesh;

	submesh.IndexCount = (UINT)quadBezierIndices.size();
	submesh.StartIndexLocation = (UINT)indices.size() + (UINT)bezierIndices.size() + (UINT)triIndices.size() + (UINT)icosahedron.GetIndices16().size();
	submesh.BaseVertexLocation = (UINT)vertices.size() + (UINT)bezierVertices.size() + (UINT)triVertices.size() + (UINT)quadBezierVertices.size();

	geo->DrawArgs["quadBezier"] = submesh;
	mGeometries[geo->Name] = std::move(geo);
}

void TessApp::BuildRenderItems()
{
	auto renderItem = std::make_unique<RenderItem>();
	renderItem->ObjCBIndex = 0;
	renderItem->Mat = mMaterials["grass"].get();
	renderItem->World = MathHelper::Identity4x4();
	renderItem->Geo = mGeometries["grid"].get();
	renderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST;
	renderItem->IndexCount = renderItem->Geo->DrawArgs["grid"].IndexCount;
	renderItem->StartIndexLocation = renderItem->Geo->DrawArgs["grid"].StartIndexLocation;
	renderItem->BaseVertexLocation = renderItem->Geo->DrawArgs["grid"].BaseVertexLocation;

	mRenderLayers[(UINT)TessLayer::Opaque].emplace_back(renderItem.get());
	mRenderItems.emplace_back(std::move(renderItem));

	renderItem = std::make_unique<RenderItem>();
	renderItem->ObjCBIndex = 1;
	renderItem->Mat = mMaterials["grass"].get();
	renderItem->World = MathHelper::Identity4x4();
	renderItem->Geo = mGeometries["grid"].get();
	renderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST;
	renderItem->IndexCount = renderItem->Geo->DrawArgs["bezier"].IndexCount;
	renderItem->StartIndexLocation = renderItem->Geo->DrawArgs["bezier"].StartIndexLocation;
	renderItem->BaseVertexLocation = renderItem->Geo->DrawArgs["bezier"].BaseVertexLocation;

	mRenderLayers[(UINT)TessLayer::Bezier].emplace_back(renderItem.get());
	mRenderItems.emplace_back(std::move(renderItem));

	renderItem = std::make_unique<RenderItem>();
	renderItem->ObjCBIndex = 2;
	renderItem->Mat = mMaterials["grass"].get();
	renderItem->World = MathHelper::Identity4x4();
	renderItem->Geo = mGeometries["grid"].get();
	renderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
	renderItem->IndexCount = renderItem->Geo->DrawArgs["triGrid"].IndexCount;
	renderItem->StartIndexLocation = renderItem->Geo->DrawArgs["triGrid"].StartIndexLocation;
	renderItem->BaseVertexLocation = renderItem->Geo->DrawArgs["triGrid"].BaseVertexLocation;

	mRenderLayers[(UINT)TessLayer::Triangle].emplace_back(renderItem.get());
	mRenderItems.emplace_back(std::move(renderItem));


	renderItem = std::make_unique<RenderItem>();
	renderItem->ObjCBIndex = 3;
	renderItem->Mat = mMaterials["grass"].get();
	XMStoreFloat4x4(&renderItem->World, XMMatrixTranslation(0.0f, .0f, 0.0f));
	renderItem->Geo = mGeometries["grid"].get();
	renderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
	renderItem->IndexCount = renderItem->Geo->DrawArgs["sphere"].IndexCount;
	renderItem->StartIndexLocation = renderItem->Geo->DrawArgs["sphere"].StartIndexLocation;
	renderItem->BaseVertexLocation = renderItem->Geo->DrawArgs["sphere"].BaseVertexLocation;

	mRenderLayers[(UINT)TessLayer::Sphere].emplace_back(renderItem.get());
	mRenderItems.emplace_back(std::move(renderItem));

	renderItem = std::make_unique<RenderItem>();
	renderItem->ObjCBIndex = 4;
	renderItem->Mat = mMaterials["grass"].get();
	renderItem->World = MathHelper::Identity4x4();
	renderItem->Geo = mGeometries["grid"].get();
	renderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_9_CONTROL_POINT_PATCHLIST;
	renderItem->IndexCount = renderItem->Geo->DrawArgs["quadBezier"].IndexCount;
	renderItem->StartIndexLocation = renderItem->Geo->DrawArgs["quadBezier"].StartIndexLocation;
	renderItem->BaseVertexLocation = renderItem->Geo->DrawArgs["quadBezier"].BaseVertexLocation;

	mRenderLayers[(UINT)TessLayer::QuadBezier].emplace_back(renderItem.get());
	mRenderItems.emplace_back(std::move(renderItem));
}

void TessApp::BuildMaterials()
{
	auto mat = std::make_unique<Material>(mNumFrameResources);
	mat->Name = "grass";
	mat->MatCBIndex = 0;
	mat->DiffuseSrvHeapIndex = 0;
	mat->DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	mMaterials[mat->Name] = std::move(mat);
}

void TessApp::BuildTextures()
{
	auto tex = std::make_unique<Texture>();
	tex->Name = "grass";
	tex->Filename = L"Textures\\grass.dds";
	CreateDDSTextureFromFile12(md3dDevice.Get(), mCommandList.Get(), tex->Filename.c_str(), tex->Resource, tex->UploadHeap);

	mTextures[tex->Name] = std::move(tex);
}

void TessApp::UpdateCB(const GameTimer & gt)
{
	for (auto& rItem : mRenderItems) {
		if (rItem->NumFramesDirty != 0) {
			ObjectConstants ocb;
			XMMATRIX world = XMLoadFloat4x4(&rItem->World);
			XMStoreFloat4x4(&ocb.World, XMMatrixTranspose(world));
			mCurrentFrameResource->ObjectCB->CopyData(rItem->ObjCBIndex, ocb);
			rItem->NumFramesDirty--;
		}
	}

	for (auto& mat : mMaterials) {
		auto material = mat.second.get();
		if (material->NumFramesDirty != 0) {
			MaterialConstants mcb; 
			mcb.DiffuseAlbedo = material->DiffuseAlbedo;
			mcb.FresnelR0 = material->FresnelR0;
			mcb.Roughness = material->Roughness;
			XMMATRIX mt = XMLoadFloat4x4(&material->MatTransform);
			XMStoreFloat4x4(&mcb.MatTransform, mt);
			mCurrentFrameResource->MatCB->CopyData(material->MatCBIndex, mcb);
		}
	}
}

void TessApp::UpdatePassCB(const GameTimer & gt)
{
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMStoreFloat4x4(&mPassConstants.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mPassConstants.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mPassConstants.ViewProj, XMMatrixTranspose(XMMatrixMultiply(view, proj)));
	mPassConstants.EyePos = mEyePos;
	XMVECTOR dir = -MathHelper::SphericalToCartesian(1.0f, mSunTheta, mSunPhi);
	XMStoreFloat3(&mPassConstants.Lights[0].Direction, dir);
	mPassConstants.Lights[0].Strength = { 1.0f, 1.0f, 0.9f };
	mCurrentFrameResource->PassCB->CopyData(0, mPassConstants);
}

bool TessApp::Initialize()
{
	if (!D3DApp::Initialize()) {
		return false;
	}

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildMaterials();
	BuildTextures();
	BuildGridGeometry();
	BuildRenderItems();
	BuildFrameResources();
	BuildDescriptorHeaps();
	BuildDescriptors();
	BuildShaderResourceViews();

	BuildPSOs();


	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdLists[1] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	FlushCommandQueue();

	return true;
}

void TessApp::Update(const GameTimer & gt)
{

	UpdateCamera(gt);
	HandleKeyboardInput(gt);

	mCurrentFrameResourceIndex = (mCurrentFrameResourceIndex + 1) % mNumFrameResources;
	mCurrentFrameResource = mFrameResources[mCurrentFrameResourceIndex].get();

	if (mCurrentFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrentFrameResource->Fence) {
		auto continueExec = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFrameResource->Fence, continueExec));
		WaitForSingleObject(continueExec, INFINITE);
		CloseHandle(continueExec);
	}

	UpdatePassCB(gt);
	UpdateCB(gt);
}

void TessApp::Draw(const GameTimer & gt)
{
	auto cmdAlloc = mCurrentFrameResource->CmdListAlloc.Get();

	ThrowIfFailed(cmdAlloc->Reset());

	ThrowIfFailed(mCommandList->Reset(cmdAlloc, mActivePSO.Get()));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
	ID3D12DescriptorHeap* heaps[1] = { mCbvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(heaps), heaps);

	auto passCbOffset = mCurrentFrameResourceIndex + mMainPassCbOffset;
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(mCbvHeap->GetGPUDescriptorHandleForHeapStart(),
		passCbOffset, mCbvSrvDescriptorSize);
	mCommandList->SetGraphicsRootDescriptorTable(1, gpuHandle);


	for (auto i = 0; i < mRenderLayers[(int)mActiveRenderLayer].size(); ++i) {
		auto rItem = mRenderLayers[(int)mActiveRenderLayer][i];
		mCommandList->IASetPrimitiveTopology(rItem->PrimitiveType);
		mCommandList->IASetVertexBuffers(0, 1, &rItem->Geo->VertexBufferView());
		mCommandList->IASetIndexBuffer(&rItem->Geo->IndexBufferView());

		UINT cbOffset = (UINT)(mCurrentFrameResourceIndex * mRenderItems.size() + rItem->ObjCBIndex);
		UINT matOffset = mMatCbOffset + (UINT)(mCurrentFrameResourceIndex * mMaterials.size() + rItem->Mat->MatCBIndex);
		UINT texOffset = mTextureOffset + rItem->Mat->DiffuseSrvHeapIndex;

		gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart(),
			cbOffset, mCbvSrvDescriptorSize);
		mCommandList->SetGraphicsRootDescriptorTable(0, gpuHandle);

		gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart(), matOffset, mCbvSrvDescriptorSize);
		mCommandList->SetGraphicsRootDescriptorTable(2, gpuHandle);

		gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart(), texOffset, mCbvSrvDescriptorSize);
		mCommandList->SetGraphicsRootDescriptorTable(3, gpuHandle);

		mCommandList->DrawIndexedInstanced(rItem->IndexCount, 1, rItem->StartIndexLocation,
			rItem->BaseVertexLocation, 0);
	}


	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* commandLists[1] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	// swap the buffer
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	mCurrentFrameResource->Fence = ++mCurrentFence;
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void TessApp::OnResize()
{
	D3DApp::OnResize();
	auto P = XMMatrixPerspectiveFovLH(XM_PIDIV4, AspectRatio(), mNearZ, mFarZ);
	XMStoreFloat4x4(&mProj, P);
}

void TessApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void TessApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;
	SetCapture(mhMainWnd);
}

void TessApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0) {
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

		mTheta += dx;
		mPhi += dy;

		mPhi = MathHelper::Clamp(mPhi, 1.0f, XM_PI - 1.0f);
	}
	else if ((btnState & MK_RBUTTON) != 0) {
		float dx = 5.0f*static_cast<float>(x - mLastMousePos.x);
		float dy = 5.0f*static_cast<float>(y - mLastMousePos.y);

		mRadius += dx - dy;
		mRadius = MathHelper::Clamp(mRadius, mNearZ + 1.0f, mFarZ - 1.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void TessApp::HandleKeyboardInput(const GameTimer & gt)
{
	const static float debounceTime = 0.25f;
	static float t = 0.2f;
	t += gt.DeltaTime();

	if (t > debounceTime) {
		if (GetAsyncKeyState('B') & 0x8000) {
			ToggleRenderLayer(TessLayer::Bezier, "BezierTess");
			t = 0.0f;
		}

		if (GetAsyncKeyState('T') & 0x8000) {
			ToggleRenderLayer(TessLayer::Triangle, "TriTess");
			t = 0.0f;
		}

		if (GetAsyncKeyState('S') & 0x8000) {
			ToggleRenderLayer(TessLayer::Sphere, "TriTessSphere");
			t = 0.0f;
		}

		if (GetAsyncKeyState('Q') & 0x8000) {
			ToggleRenderLayer(TessLayer::QuadBezier, "QuadBezier");
			t = 0.0f;
		}
	}

	if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
		mSunTheta += gt.DeltaTime()*1.0f;
	}
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
		mSunTheta -= gt.DeltaTime()*1.0f;
	}
	if (GetAsyncKeyState(VK_UP) & 0x8000) {
		mSunPhi -= gt.DeltaTime()*1.0f;
	}
	if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
		mSunPhi += gt.DeltaTime()*1.0f;
	}
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 1> TessApp::GetStaticSamplers() const
{
	auto anisotropicWrap = CD3DX12_STATIC_SAMPLER_DESC(0,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		0.0f, 8u);

	return { anisotropicWrap };
}

void TessApp::UpdateCamera(const GameTimer& gt)
{
	auto x = mRadius*sinf(mPhi)*cosf(mTheta);
	auto z = mRadius*sinf(mPhi)*sinf(mTheta);
	auto y = mRadius*cosf(mPhi);

	mEyePos.x = x;
	mEyePos.y = y;
	mEyePos.z = z;

	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX V = XMMatrixLookAtLH(pos, XMVectorZero(), up);
	XMStoreFloat4x4(&mView, V);
}

void TessApp::ToggleRenderLayer(TessLayer rLayer, const std::string& psoStr)
{
	mActivePSO = mActiveRenderLayer == rLayer ? mPSOs["PatchTess"] : mPSOs[psoStr];
	mActiveRenderLayer = mActiveRenderLayer == rLayer ? TessLayer::Opaque : rLayer;
}

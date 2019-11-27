#include "Plane.h"
#include <d3dcompiler.h>
#include <d3dx12.h>

using namespace DirectX;

bool Plane::CreatePipeline()
{
	auto result = D3DCompileFromFile(L"Plane.hlsl", nullptr, nullptr, "PlaneVS", "vs_5_0", D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_planeVertexShader, nullptr);

	// ピクセルシェーダ
	result = D3DCompileFromFile(L"Plane.hlsl", nullptr, nullptr, "PlanePS", "ps_5_0", D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_planePixelShader, nullptr);

	D3D12_INPUT_ELEMENT_DESC InputLayout[] = {
		// 座標
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0, D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		// UV
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};

	// パイプラインを作るためのGPSの変数の作成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};

	//ルートシグネチャと頂点レイアウト
	gpsDesc.pRootSignature = _planeRS;
	gpsDesc.InputLayout.pInputElementDescs = InputLayout;// 配列の開始位置
	gpsDesc.InputLayout.NumElements = _countof(InputLayout);// 要素の数を入れる

	//シェーダのセット
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(_planeVertexShader);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(_planePixelShader);

	// レンダーターゲット数の指定
	gpsDesc.NumRenderTargets = 1;
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	//深度ステンシル
	gpsDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	gpsDesc.DepthStencilState.DepthEnable = true;
	gpsDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;// 必須
	gpsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;// 小さいほうを通す

	gpsDesc.DepthStencilState.StencilEnable = false;

	//ラスタライザ
	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	//その他
	gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.NodeMask = 0;
	gpsDesc.SampleDesc.Count = 1;
	gpsDesc.SampleDesc.Quality = 0;
	gpsDesc.SampleMask = 0xffffffff;
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//三角形

	result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&_planeGPS));

	return SUCCEEDED(result);
}

bool Plane::CreateRootSignature()
{
	ID3DBlob* signature = nullptr;
	ID3DBlob* error = nullptr;

	// サンプラの設定
	D3D12_STATIC_SAMPLER_DESC SamplerDesc[1] = {};
	SamplerDesc[0].Filter = D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR;// 特別なフィルタを使用しない
	SamplerDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;// 画が繰り返し描画される
	SamplerDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	SamplerDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	SamplerDesc[0].MaxLOD = D3D12_FLOAT32_MAX;// 上限なし
	SamplerDesc[0].MinLOD = 0.0f;// 下限なし
	SamplerDesc[0].MipLODBias = 0.0f;// MIPMAPのバイアス
	SamplerDesc[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;// エッジの色(黒)
	SamplerDesc[0].ShaderRegister = 0;// 使用するレジスタスロット
	SamplerDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;// どのくらいのデータをシェーダに見せるか(全部)

	// レンジの設定
	D3D12_DESCRIPTOR_RANGE descRange[2] = {};
	// WVP
	descRange[0].NumDescriptors = 1;
	descRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;//定数
	descRange[0].BaseShaderRegister = 0;//レジスタ番号
	descRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// シャドウ
	descRange[1].NumDescriptors = 1;
	descRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//シェーダーリソース
	descRange[1].BaseShaderRegister = 0;//レジスタ番号
	descRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ルートパラメータ変数の設定
	D3D12_ROOT_PARAMETER rootParam[2] = {};
	rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[0].DescriptorTable.pDescriptorRanges = &descRange[0];//対応するレンジへのポインタ
	rootParam[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//すべてのシェーダから参照

	rootParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[1].DescriptorTable.pDescriptorRanges = &descRange[1];//対応するレンジへのポインタ
	rootParam[1].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//すべてのシェーダから参照

	// ルートシグネチャを作るための変数の設定
	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.pStaticSamplers = SamplerDesc;
	rsd.pParameters = rootParam;
	rsd.NumParameters = 2;
	rsd.NumStaticSamplers = 1;

	auto result = D3D12SerializeRootSignature(
		&rsd,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&signature,
		&error);

	// ルートシグネチャ本体の作成
	result = _dev->CreateRootSignature(
		0,
		signature->GetBufferPointer(),
		signature->GetBufferSize(),
		IID_PPV_ARGS(&_planeRS));

	return SUCCEEDED(result);
}

bool Plane::CreateVertexBuffer()
{
	std::vector<unsigned short> indices = { 0,2,1,2,3,1 };
	//頂点バッファ生成
	PlaneVertex vertices[] = {
		{DirectX::XMFLOAT3(-250,0,-250),DirectX::XMFLOAT2(0,1)},
		{DirectX::XMFLOAT3(-250,0,250),DirectX::XMFLOAT2(0,0)},
		{DirectX::XMFLOAT3(250,0,-250),DirectX::XMFLOAT2(1,1)},
		{DirectX::XMFLOAT3(250,0,250),DirectX::XMFLOAT2(1,0)}
	};

	auto result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_vertexBuffer)
	);	PlaneVertex* mapver = nullptr;	result = _vertexBuffer->Map(0, nullptr, (void**)&mapver);
	std::copy(std::begin(vertices), std::end(vertices), mapver);	_vertexBuffer->Unmap(0, nullptr);	_vbView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
	_vbView.SizeInBytes = sizeof(vertices);
	_vbView.StrideInBytes = sizeof(PlaneVertex);
	return SUCCEEDED(result);
}

bool Plane::CreateIndexBuffer()
{
	std::vector<unsigned short> indices = { 0,2,1,2,3,1 };

	auto result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((indices.size() * sizeof(indices[0]))),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_indexBuffer)
	);	// インデックスバッファのマッピング
	_idView.BufferLocation = _indexBuffer->GetGPUVirtualAddress();//バッファの場所
	_idView.Format = DXGI_FORMAT_R16_UINT;//フォーマット(shortだからR16)
	_idView.SizeInBytes = indices.size() * sizeof(indices[0]);//総サイズ

	unsigned short* idxMap = nullptr;
	result = _indexBuffer->Map(0, nullptr, (void**)&idxMap);
	std::copy(std::begin(indices), std::end(indices), idxMap);
	_indexBuffer->Unmap(0, nullptr);

	return SUCCEEDED(result);
}

Plane::Plane(ID3D12Device* dev, D3D12_VIEWPORT _view, D3D12_RECT scissor):_dev(dev),_viewPort(_view), _scissor(scissor)
{
	CreateVertexBuffer();

	CreateIndexBuffer();

	CreateRootSignature();

	CreatePipeline();
}


Plane::~Plane()
{
}

void Plane::Draw(ID3D12GraphicsCommandList * list, ID3D12DescriptorHeap * wvp, ID3D12DescriptorHeap * shadow)
{
	float clearColor[] = { 1,1,1,1 };

	// パイプラインのセット
	list->SetPipelineState(_planeGPS);

	// ルートシグネチャをセット
	list->SetGraphicsRootSignature(_planeRS);

	//ビューポートとシザー設定
	list->RSSetViewports(1, &_viewPort);
	list->RSSetScissorRects(1, &_scissor);

	// WVPのセット
	auto wvpStart = wvp->GetGPUDescriptorHandleForHeapStart();
	list->SetDescriptorHeaps(1, &wvp);
	list->SetGraphicsRootDescriptorTable(0, wvpStart);

	// Shadowのセット
	auto shadowStart = shadow->GetGPUDescriptorHandleForHeapStart();
	list->SetDescriptorHeaps(1, &shadow);
	list->SetGraphicsRootDescriptorTable(1, shadowStart);

	// 頂点バッファビューの設定
	list->IASetVertexBuffers(0, 1, &_vbView);

	// インデックスバッファビューの設定
	list->IASetIndexBuffer(&_idView);

	// トポロジーのセット
	list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 描画部
	list->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

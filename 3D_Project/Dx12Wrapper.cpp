#include "Dx12Wrapper.h"
#include "Application.h"
#include <wrl.h>
#include <d3dcompiler.h>
#include <DirectXTex.h>
#include "PMDmodel.h"
#include "PMXmodel.h"

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"DirectXTex.lib")

// デバイスの作成
HRESULT Dx12Wrapper::DeviceInit()
{
	D3D_FEATURE_LEVEL levels[] = {
		  D3D_FEATURE_LEVEL_12_1,
		  D3D_FEATURE_LEVEL_12_0,
		  D3D_FEATURE_LEVEL_11_1,
		  D3D_FEATURE_LEVEL_11_0,
	};

	// レベルチェック
	D3D_FEATURE_LEVEL level = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_12_1;
	HRESULT result = S_OK;
	for (auto lv : levels) {
		// 作成
		result = D3D12CreateDevice(nullptr, lv, IID_PPV_ARGS(&_dev));
		// 成功しているかのチェック
		if (SUCCEEDED(result)) {
			level = lv;
			break;
		}
	}

	result = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgi));

	// DirectInputオブジェクト
	result = DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8,
		(void**)&_directInput, NULL);

	// キーデバイスの作成
	result = _directInput->CreateDevice(GUID_SysKeyboard, &_keyBoadDev, nullptr);
	result =  _keyBoadDev->SetDataFormat(&c_dfDIKeyboard);
	result = _keyBoadDev->SetCooperativeLevel(_hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);

	return result;
}

HRESULT Dx12Wrapper::CreateSwapChainAndCmdQue()
{
	HRESULT result = S_OK;
	/* --コマンドキューの作成-- */
	{
	D3D12_COMMAND_QUEUE_DESC cmdQDesc = {};
	cmdQDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmdQDesc.NodeMask = 0;
	cmdQDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmdQDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	// コマンドキューの作成とチェック
	if (FAILED(result = _dev->CreateCommandQueue(&cmdQDesc, IID_PPV_ARGS(&_cmdQue))))
	{
		return result;
	};
	}
	/* -- ここまで -- */

	/* --スワップチェインの作成--　*/
	{
	// ウィンドウサイズの取得
	Size window = Application::Instance().GetWindowSize();

	// スワップチェイン用の変数の作成
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	{
		swapChainDesc.Width = window.width; // ウィンドウの幅
		swapChainDesc.Width = window.height; // ウィンドウの高さ
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // フォーマットの指定(今回はRGBA各8ビットのノーマル)
		swapChainDesc.Stereo = FALSE; // 3DS的な表現をするかどうか、基本はfalse
		swapChainDesc.SampleDesc.Count = 1; // よくわからん
		swapChainDesc.SampleDesc.Quality = 0; // よくわからん
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // バッファの使用法(今回は出力用だよって言ってる)
		swapChainDesc.BufferCount = 2; // バッファの数(表と裏で二枚なので２)
		swapChainDesc.Scaling = DXGI_SCALING_STRETCH; // バッファのスケーリング方法(今回のはサイズを無条件で引き延ばすやつ)
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // バッファの切り替え時にどう切り替えるかを指定、今回はドライバ依存。
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED; // アルファ値の設定(今回はアルファの指定はなし)
		swapChainDesc.Flags = 0; // バックバッファからフロントバッファへの移行のときのオプションが設定できるらしい。よくわからんから０
	}

	// スワップチェイン本体の作成
	result = _dxgi->CreateSwapChainForHwnd(_cmdQue,
		_hwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)(&_swapchain));
	}
	/* --ここまで-- */

	/* --スワップチェインに使うレンダーターゲットビューの作成-- */
	{
		// スワップチェインリソースひーぷ
		D3D12_DESCRIPTOR_HEAP_DESC swcHeapDesc = {};
		swcHeapDesc.NumDescriptors = 2;
		swcHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		swcHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		result = _dev->CreateDescriptorHeap(&swcHeapDesc, IID_PPV_ARGS(&_swcDescHeap));
		// ヒープの先頭ハンドルを取得する
		D3D12_CPU_DESCRIPTOR_HANDLE swchandle = _swcDescHeap->GetCPUDescriptorHandleForHeapStart();

		// スワップチェインの取得
		DXGI_SWAP_CHAIN_DESC swcDesc = {};
		_swapchain->GetDesc(&swcDesc);

		// レンダーターゲットの数を取得
		int renderTargetsNum = swcDesc.BufferCount;

		// 取得したレンダーターゲットの数分だけ確保する
		renderTargets.resize(renderTargetsNum);

		// デスクリプタの一個当たりのサイズの取得
		int descriptorSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// レンダーターゲットビュー本体の作成
		for (int i = 0; i < renderTargetsNum; i++)
		{
			result = _swapchain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
			if (FAILED(result))break;

			_dev->CreateRenderTargetView(renderTargets[i], nullptr, swchandle);
			swchandle.ptr += descriptorSize;
		}
	}
	return result;
}

HRESULT Dx12Wrapper::CreateCmdListAndAlloc()
{
	HRESULT result = S_OK;

	_dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAlloc));
	_dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAlloc, nullptr, IID_PPV_ARGS(&_cmdList));

	_cmdList->Close();

	return result;
}

HRESULT Dx12Wrapper::CreateFence()
{
	HRESULT result = S_OK;
	_fenceValue = 0;
	result = _dev->CreateFence(_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
	return result;
}

HRESULT Dx12Wrapper::LoadShader()
{
	HRESULT result;
	auto name = isPMD? pmdModel->GetUseShader():pmxModel->GetUseShader();

	// 頂点シェーダ
	result = D3DCompileFromFile(name, nullptr, nullptr, "vs", "vs_5_0", D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION, 0, &vertexShader, nullptr);

	// ピクセルシェーダ
	result = D3DCompileFromFile(name, nullptr, nullptr, "ps", "ps_5_0", D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pixelShader, nullptr);

	return result;
}

// シグネチャの設定
HRESULT Dx12Wrapper::CreateRootSignature()
{
	HRESULT result;

	// サンプラの設定
	D3D12_STATIC_SAMPLER_DESC SamplerDesc[2] = {};
	SamplerDesc[0].Filter = D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT;// 特別なフィルタを使用しない
	SamplerDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;// 画が繰り返し描画される
	SamplerDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	SamplerDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	SamplerDesc[0].MaxLOD = D3D12_FLOAT32_MAX;// 上限なし
	SamplerDesc[0].MinLOD = 0.0f;// 下限なし
	SamplerDesc[0].MipLODBias = 0.0f;// MIPMAPのバイアス
	SamplerDesc[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;// エッジの色(黒)
	SamplerDesc[0].ShaderRegister = 0;// 使用するレジスタスロット
	SamplerDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;// どのくらいのデータをシェーダに見せるか(全部)
	SamplerDesc[0].RegisterSpace = 0;// わからん
	SamplerDesc[0].MaxAnisotropy = 0;// FilterがAnisotropyのときのみ有効
	SamplerDesc[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;// 常に否定

	SamplerDesc[1] = SamplerDesc[0];//変更点以外をコピー
	SamplerDesc[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;//繰り返さない
	SamplerDesc[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;//繰り返さない
	SamplerDesc[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;//繰り返さない
	SamplerDesc[1].ShaderRegister = 1; //シェーダスロット番号を忘れないように

	// レンジの設定
	D3D12_DESCRIPTOR_RANGE descRange[4] = {};// テクスチャと定数二つとぼーん
	descRange[0].NumDescriptors = 1;
	descRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;//定数
	descRange[0].BaseShaderRegister = 0;//レジスタ番号
	descRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	descRange[1].NumDescriptors = 1;
	descRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;//定数
	descRange[1].BaseShaderRegister = 1;//レジスタ番号
	descRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// テクスチャー
	descRange[2].NumDescriptors = 4;// テクスチャとスフィアの二つとトゥーン
	descRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//てくすちゃ
	descRange[2].BaseShaderRegister = 0;//レジスタ番号
	descRange[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ボーン
	descRange[3].NumDescriptors = 1;
	descRange[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descRange[3].BaseShaderRegister = 2;
	descRange[3].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ルートパラメータ変数の設定
	D3D12_ROOT_PARAMETER rootParam[3] = {};
	rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[0].DescriptorTable.pDescriptorRanges = &descRange[0];//対応するレンジへのポインタ
	rootParam[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//すべてのシェーダから参照

	// テクスチャ
	rootParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[1].DescriptorTable.pDescriptorRanges = &descRange[1];//対応するレンジへのポインタ
	rootParam[1].DescriptorTable.NumDescriptorRanges = 2;
	rootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//ピクセルシェーダから参照

	// ボーン
	rootParam[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[2].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[2].DescriptorTable.pDescriptorRanges = &descRange[3];
	rootParam[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	// ルートシグネチャを作るための変数の設定
	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.pStaticSamplers = SamplerDesc;
	rsd.pParameters = rootParam;
	rsd.NumParameters = 3;
	rsd.NumStaticSamplers = 2;

	result = D3D12SerializeRootSignature(
		&rsd, 
		D3D_ROOT_SIGNATURE_VERSION_1, 
		&signature, 
		&error);

	// ルートシグネチャ本体の作成
	result = _dev->CreateRootSignature(
		0,
		signature->GetBufferPointer(),
		signature->GetBufferSize(),
		IID_PPV_ARGS(&_rootSignature));	

	return result;
}

// GraphicsPipelineState(描画用パイプライン)の作成
HRESULT Dx12Wrapper::CreateGraphicsPipelineState()
{
	HRESULT result;	

	auto inputLayoutDescs = pmxModel->GetInputLayout();
	//auto inputLayoutDescs = pmdModel->GetInputLayout();

	// パイプラインを作るためのGPSの変数の作成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};

	//ルートシグネチャと頂点レイアウト
	gpsDesc.pRootSignature = _rootSignature;
	gpsDesc.InputLayout.pInputElementDescs = inputLayoutDescs.data();// 配列の開始位置
	gpsDesc.InputLayout.NumElements = static_cast<unsigned int>(inputLayoutDescs.size());// 要素の数を入れる

	//シェーダのセット
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader);

	//レンダーターゲット
	gpsDesc.NumRenderTargets = 1;
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	//深度ステンシル
	gpsDesc.DepthStencilState.DepthEnable = true;
	gpsDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;// 必須
	gpsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;// 小さいほうを通す

	gpsDesc.DepthStencilState.StencilEnable = false;
	gpsDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	//ラスタライザ
	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	//レンダーターゲットブレンド設定用構造体
	D3D12_RENDER_TARGET_BLEND_DESC renderBlend = {};
	renderBlend.BlendEnable = true;
	renderBlend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	renderBlend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	renderBlend.BlendOp = D3D12_BLEND_OP_ADD;
	renderBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
	renderBlend.DestBlendAlpha = D3D12_BLEND_ZERO;
	renderBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	renderBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	//ブレンドステート設定用構造体
	D3D12_BLEND_DESC blend = {};
	blend.AlphaToCoverageEnable = false;
	blend.IndependentBlendEnable = false;
	blend.RenderTarget[0] = renderBlend;

	//その他
	gpsDesc.BlendState = blend;
	gpsDesc.NodeMask = 0;
	gpsDesc.SampleDesc.Count = 1;
	gpsDesc.SampleDesc.Quality = 0;
	gpsDesc.SampleMask = 0xffffffff;
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//三角形

	result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&_pipelineState));
	return result;
}


HRESULT Dx12Wrapper::CreateConstantBuffer()
{
	// コンスタン十バッファ用のヒーププロップ
	D3D12_HEAP_PROPERTIES cbvHeapProp = {};
	cbvHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	cbvHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	cbvHeapProp.CreationNodeMask = 1;
	cbvHeapProp.VisibleNodeMask = 1;
	cbvHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_DESCRIPTOR_HEAP_DESC rgstDesc = {};
	rgstDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	rgstDesc.NodeMask = 0;
	rgstDesc.NumDescriptors = 1;
	rgstDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	auto result = _dev->CreateDescriptorHeap(&rgstDesc,IID_PPV_ARGS(&_rgstDescHeap));
	auto wsize = Application::Instance().GetWindowSize();

	// 座標の初期値
	eye = XMFLOAT3(0,10,-15);
	target = XMFLOAT3(0, 10,0);
	up = XMFLOAT3(0,1,0);

	angle = 0.f;

	_wvp.world = DirectX::XMMatrixRotationY(angle);

	_wvp.view = DirectX::XMMatrixLookAtLH(
		DirectX::XMLoadFloat3(&eye),
		DirectX::XMLoadFloat3(&target),
		DirectX::XMLoadFloat3(&up)
	);

	_wvp.projection = XMMatrixPerspectiveFovLH(
		XM_PIDIV4,
		static_cast<float>(wsize.width) / static_cast<float>(wsize.height),
		0.1f,
		300.0f
	);

	// サイズを調整
	size_t size = sizeof(_wvp);
	size = (size + 0xff)&~0xff;

	// コンスタントバッファの作成
	result = _dev->CreateCommittedResource(&cbvHeapProp,
		D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_constBuff));

	// Mapping
	result = _constBuff->Map(0,nullptr,(void**)&_wvpMP);
	std::memcpy(_wvpMP,&_wvp,sizeof(_wvp));

	// コンスタントバッファ用のデスク
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _constBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = static_cast<unsigned int>(size);

	auto handle = _rgstDescHeap->GetCPUDescriptorHandleForHeapStart();
	// コンスタントバッファビューの作成
	_dev->CreateConstantBufferView(&cbvDesc, handle);

	return result;
}

HRESULT Dx12Wrapper::CreateDSV()
{
	Size wsize = Application::Instance().GetWindowSize();

	D3D12_DESCRIPTOR_HEAP_DESC _dsvHeapDesc = {};
	_dsvHeapDesc.NumDescriptors = 1;
	_dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	auto result = _dev->CreateDescriptorHeap(&_dsvHeapDesc, IID_PPV_ARGS(&_dsvDescHeap));
	
	// DSV用のリソースデスク
	D3D12_RESOURCE_DESC depthResDesc = {};
	depthResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthResDesc.Width = wsize.width;
	depthResDesc.Height = wsize.height;
	depthResDesc.DepthOrArraySize = 1;
	depthResDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthResDesc.SampleDesc.Count = 1;
	depthResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	// DSV用のヒーププロップ
	D3D12_HEAP_PROPERTIES depthHeapProp = {};
	depthHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	depthHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	depthHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	//クリアバリュー
	D3D12_CLEAR_VALUE _depthClearValue = {};
	_depthClearValue.DepthStencil.Depth = 1.0f;
	_depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;

	result = _dev->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&_depthClearValue,
		IID_PPV_ARGS(&_depthBuffer));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvdec = {};
	dsvdec.Format = DXGI_FORMAT_D32_FLOAT;
	dsvdec.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvdec.Texture2D.MipSlice = 0;
	dsvdec.Flags = D3D12_DSV_FLAG_NONE;
	
	_dev->CreateDepthStencilView(_depthBuffer, &dsvdec, _dsvDescHeap->GetCPUDescriptorHandleForHeapStart());

	return result;
}

void Dx12Wrapper::WaitWithFence()
{
	int cnt = 0;
	while (_fence->GetCompletedValue() != _fenceValue){
		//びじーるぷ
	};
}

void Dx12Wrapper::ExecuteCommand()
{
	_cmdQue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&_cmdList);
	_cmdQue->Signal(_fence, ++_fenceValue);
}

Dx12Wrapper::Dx12Wrapper(HWND hwnd):_hwnd(hwnd)
{
	// イニシャライズ
	auto result = CoInitializeEx(0, COINIT_MULTITHREADED);
}

Dx12Wrapper::~Dx12Wrapper()
{
	// キーボードデバイスの解放
	_keyBoadDev->Unacquire();

	if (_keyBoadDev != NULL)
		_keyBoadDev->Release();

	if (_directInput != NULL)
		_directInput->Release();
}

// いろんな初期化チェック
int Dx12Wrapper::Init()
{
	// ﾃﾞﾊﾞｯｸﾞﾚｲﾔｰをON
#if defined(_DEBUG)
	ID3D12Debug* debug;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))){
		debug->EnableDebugLayer();
		debug->Release();
	}
#endif

	// デバイスの生成
	if (FAILED(DeviceInit())) 
		return 1;
	isPMD = false;
	// モデル読み込み
	//pmxModel = std::make_shared<PMXmodel>(_dev, "model/PMX/ちびフラン/ちびフラン標準ボーン.pmx");
	pmdModel = std::make_shared<PMDmodel>(_dev, "model/PMD/初音ミク.pmd");
	pmxModel = std::make_shared<PMXmodel>(_dev, "model/PMX/ちびルーミア/ちびルーミア.pmx");
	//pmxModel = std::make_shared<PMXmodel>(_dev, "model/PMX/GUMI/GUMIβ_V3.pmx");

	// スワップチェインの生成
	if (FAILED(CreateSwapChainAndCmdQue())) 
		return 2;

	// コマンド系の生成
	if (FAILED(CreateCmdListAndAlloc())) 
		return 3;

	// フェンスの生成
	if (FAILED(CreateFence())) 
		return 4;

	// シェーダー読み込み
	if (FAILED(LoadShader()))
		return 5;

	// シグネチャの生成
	if (FAILED(CreateRootSignature())) 
		return 6;

	// パイプラインの作成
	if (FAILED(CreateGraphicsPipelineState()))
		return 7;

	// 深度バッファの作成
	if (FAILED(CreateDSV())) 
		return 8;

	// 定数バッファの作成
	if (FAILED(CreateConstantBuffer())) 
		return 9;

	return 0;
}

// 毎フレーム呼び出す更新処理
void Dx12Wrapper::UpDate()
{
	auto wsize = Application::Instance().GetWindowSize();

	// キーの入力
	char Oldkey[256];
	int i = 0;
	for (auto& k : key)
	{
		Oldkey[i++] = k;
	}

	auto isOk = _keyBoadDev->GetDeviceState(sizeof(key), key);
	if (FAILED(isOk)) {
		// 失敗なら再開させてもう一度取得
		_keyBoadDev->Acquire();
		_keyBoadDev->GetDeviceState(sizeof(key), key);
	}

	float addsize = 0.1f;
	if (key[DIK_ESCAPE])
	{
		PostQuitMessage(0);
	}

	if (key[DIK_UP])
	{
		eye.y += addsize;
		target.y += addsize;
	}
	else if (key[DIK_DOWN])
	{
		eye.y-=addsize;
		target.y-=addsize;
	}

	if (key[DIK_Q]|| key[DIK_LEFT])
	{
		angle += 0.01f;
	}
	else if (key[DIK_E] || key[DIK_RIGHT])
	{
		angle -= 0.01f;
	}

	if (key[DIK_W])
	{
		eye.z+= addsize;
		target.z+= addsize;
	}
	else if (key[DIK_S])
	{
		eye.z-= addsize;
		target.z-= addsize;
	}

	if (key[DIK_D])
	{
		eye.x+= addsize;
		target.x+= addsize;
	}
	if (key[DIK_A])
	{
		eye.x-= addsize;
		target.x-= addsize;
	}

	if (key[DIK_R])
	{
		eye = XMFLOAT3(0, 10, -25);
		target = XMFLOAT3(0, 10, 0);
		up = XMFLOAT3(0, 1, 0);

		angle = 0.f;
	}
	if (key[DIK_P])
	{
		isOk = 0;
	}
	if (key[DIK_P]&&(Oldkey[DIK_P]==0))
	{
		float rota[3] = { XM_PIDIV4 , XM_PIDIV2 ,XM_PI / 30 * XM_PI };
		cnt++;
		_wvp.projection = XMMatrixPerspectiveFovLH(
			rota[cnt%3],
			static_cast<float>(wsize.width) / static_cast<float>(wsize.height),
			0.1f,
			300.0f
		);
	}

	_wvp.view = DirectX::XMMatrixLookAtLH(
		DirectX::XMLoadFloat3(&eye),
		DirectX::XMLoadFloat3(&target),
		DirectX::XMLoadFloat3(&up)
	);
	isPMD?pmdModel->UpDate():pmxModel->UpDate(key);
	

	_wvp.view = DirectX::XMMatrixRotationY(angle)*_wvp.view;

	_wvp.wvp = _wvp.world;
	_wvp.wvp *= _wvp.view;
	_wvp.wvp *= _wvp.projection;
	*_wvpMP = _wvp;

	auto heapStart = _swcDescHeap->GetCPUDescriptorHandleForHeapStart();
	float clearColor[] = { 0.5f,0.5f,0.5f,0.0f };//クリアカラー設定

	// ビューポート
	D3D12_VIEWPORT _viewport;
	_viewport.TopLeftX = 0;
	_viewport.TopLeftY = 0;
	_viewport.Width  = static_cast<float>(Application::Instance().GetWindowSize().width);
	_viewport.Height = static_cast<float>(Application::Instance().GetWindowSize().height);
	_viewport.MaxDepth = 1.0f;
	_viewport.MinDepth = 0.0f;

	// シザー（きりとり？）
	D3D12_RECT _scissorRect;
	_scissorRect.left = 0;
	_scissorRect.top = 0;
	_scissorRect.right = Application::Instance().GetWindowSize().width;
	_scissorRect.bottom = Application::Instance().GetWindowSize().height;

	// バックバッファのインデックスをとってきて入れ替える
	auto bbIndex = _swapchain->GetCurrentBackBufferIndex();
	int DescriptorSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	heapStart.ptr += bbIndex * DescriptorSize;

	// バリアの作成------------//
	D3D12_RESOURCE_BARRIER BarrierDesc = {};
	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.Transition.pResource = renderTargets[bbIndex];
	BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	_cmdAlloc->Reset();//アロケータリセット
	_cmdList->Reset(_cmdAlloc, nullptr);//コマンドリストリセット

	// パイプラインのセット
	_cmdList->SetPipelineState(_pipelineState);

	// ルートシグネチャをセット
	_cmdList->SetGraphicsRootSignature(_rootSignature);

	// でスクリプターヒープのセット(レジスタ)
	_cmdList->SetDescriptorHeaps(1, &_rgstDescHeap);

	// デスクリプタテーブルのセット(レジスタ)
	D3D12_GPU_DESCRIPTOR_HANDLE rgpuStart = _rgstDescHeap->GetGPUDescriptorHandleForHeapStart();
	_cmdList->SetGraphicsRootDescriptorTable(0, rgpuStart);
	
	//ビューポートとシザー設定
	_cmdList->RSSetViewports(1, &_viewport);
	_cmdList->RSSetScissorRects(1, &_scissorRect);

	// リソースバリア
	_cmdList->ResourceBarrier(1, &BarrierDesc);

	//レンダーターゲット設定
	_cmdList->OMSetRenderTargets(1, &heapStart, false, &_dsvDescHeap->GetCPUDescriptorHandleForHeapStart());

	//クリア
	_cmdList->ClearRenderTargetView(heapStart, clearColor, 0, nullptr);

	// でぷすのクリア
	_cmdList->ClearDepthStencilView(_dsvDescHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

	// インデックス情報のセット
	_cmdList->IASetIndexBuffer(&(isPMD? pmdModel->GetIndexView():pmxModel->GetIndexView()));

	// 頂点バッファビューの設定
	_cmdList->IASetVertexBuffers(0, 1, &(isPMD ? pmdModel->GetVertexView(): pmxModel->GetVertexView()));

	// トポロジーのセット
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// モデルのマテリアル適用
	// どのインデックスから始めるかを入れておく変数
	unsigned int offset = 0;
	auto boneheap = isPMD? pmdModel->GetBoneHeap():pmxModel->GetBoneHeap();
	auto materialheap = isPMD ? pmdModel->GetMaterialHeap():pmxModel->GetMaterialHeap();

	// デスクリプターハンドル一枚のサイズ取得
	int incsize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	/*　ボーンを動かす　*/

	// ハンドルの取得
	auto bohandle = boneheap->GetGPUDescriptorHandleForHeapStart();

	// デスクリプタヒープのセット
	_cmdList->SetDescriptorHeaps(1, &boneheap);

	// デスクリプタテーブルのセット
	_cmdList->SetGraphicsRootDescriptorTable(2, bohandle);


	/*　マテリアルの適用　*/

	// ハンドルの取得
	auto mathandle = materialheap->GetGPUDescriptorHandleForHeapStart();

	// デスクリプタヒープのセット
	_cmdList->SetDescriptorHeaps(1, &materialheap);


	// 描画ループ

	//for (auto& m : pmdModel->GetMaterials()) {
	for (auto& m : pmxModel->GetMaterials()) {
		// デスクリプタテーブルのセット
		_cmdList->SetGraphicsRootDescriptorTable(1, mathandle);

		// 描画部
		_cmdList->DrawIndexedInstanced(m.faceVerCnt, 1, offset, 0, 0);

		// ポインタの加算
		mathandle.ptr += incsize*5;// 5枚あるから5倍
		
		// 変数の加算
		offset += m.faceVerCnt;
	}

	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

	// リソースバリア
	_cmdList->ResourceBarrier(1, &BarrierDesc);

	//コマンドのクローズ
	_cmdList->Close();

	// コマンド追加とフェンス
	ExecuteCommand();

	// 待ち
	WaitWithFence();

	// 画面の更新
	_swapchain->Present(1, 0);
}

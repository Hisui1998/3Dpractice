#include "Dx12Wrapper.h"
#include "Application.h"
#include <wrl.h>
#include <d3dcompiler.h>
#include <DirectXTex.h>
#include <algorithm>
#include "PMDmodel.h"
#include "PMXmodel.h"

#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"DirectXTex.lib")

const FLOAT clearColor[] = { 0.5f,0.5f,0.5f,0.0f };//クリアカラー設定

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

// スワップチェインとコマンドキューの作成
HRESULT Dx12Wrapper::CreateSwapChainAndCmdQue()
{
	// コマンドキュー作成に使う情報
	D3D12_COMMAND_QUEUE_DESC cmdQDesc = {};
	cmdQDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmdQDesc.NodeMask = 0;
	cmdQDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmdQDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	// コマンドキューの作成
	auto result = _dev->CreateCommandQueue(&cmdQDesc, IID_PPV_ARGS(&_cmdQue));

	// ウィンドウサイズの取得
	Size window = Application::Instance().GetWindowSize();

	// スワップチェイン用の変数の作成
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = window.width; // ウィンドウの幅
	swapChainDesc.Width = window.height; // ウィンドウの高さ
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = FALSE; // 3DS的な表現をするかどうか、基本はfalse
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0; 
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // バッファの使用法(今回は出力用だよって言ってる)
	swapChainDesc.BufferCount = 2; // バッファの数(表と裏で二枚なので２)
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH; // バッファのスケーリング方法(今回のはサイズを無条件で引き延ばすやつ)
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // バッファの切り替え時にどう切り替えるかを指定、今回はドライバ依存。
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED; // アルファ値の設定(今回はアルファの指定はなし)
	swapChainDesc.Flags = 0; // バックバッファからフロントバッファへの移行のときのオプションが設定できるらしい。よくわからんから０

	// スワップチェイン本体の作成
	result = _dxgi->CreateSwapChainForHwnd(_cmdQue,
		_hwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)(&_swapchain));

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

	// レンダーターゲットビュー本体をバッファの数分作成する
	for (int i = 0; i < renderTargetsNum; i++)
	{
		result = _swapchain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
		if (FAILED(result))break;

		_dev->CreateRenderTargetView(renderTargets[i], nullptr, swchandle);
		swchandle.ptr += descriptorSize;
	}

	return result;
}

// コマンドリストとコマンドアロケータの作成
HRESULT Dx12Wrapper::CreateCmdListAndAlloc()
{
	HRESULT result = S_OK;

	_dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAlloc));
	_dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAlloc, nullptr, IID_PPV_ARGS(&_cmdList));

	_cmdList->Close();

	return result;
}

// フェンスの生成
HRESULT Dx12Wrapper::CreateFence()
{
	HRESULT result = S_OK;
	_fenceValue = 0;
	result = _dev->CreateFence(_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
	return result;
}

// WVP用の定数バッファの作成
HRESULT Dx12Wrapper::CreateWVPConstantBuffer()
{
	// デスクリプタヒープ作成用の設定
	D3D12_DESCRIPTOR_HEAP_DESC wvpDesc = {};
	wvpDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	wvpDesc.NodeMask = 0;
	wvpDesc.NumDescriptors = 1;
	wvpDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	auto result = _dev->CreateDescriptorHeap(&wvpDesc,IID_PPV_ARGS(&_wvpDescHeap));
	auto wsize = Application::Instance().GetWindowSize();

	// 座標の初期値
	eye = XMFLOAT3(0,10,-20);
	target = XMFLOAT3(0, 10,0);
	up = XMFLOAT3(0,1,0);

	auto plane = XMFLOAT4(0, 1, 0, 0);//平面の方程式
	lightVec = XMFLOAT3(-10, 20, -10);// ライトの平行光線の方向

	angle = 0.f;

	// ライト座標の算出
	XMFLOAT3 lightpos;
	XMStoreFloat3(&lightpos,
		XMVector4Normalize(XMLoadFloat3(&lightVec)) * XMVector3Length(XMLoadFloat3(&target) - XMLoadFloat3(&eye)));//ライト座標
	
	// ライトから注視点への行列の計算
	XMMATRIX lightview = DirectX::XMMatrixLookAtLH(
		DirectX::XMLoadFloat3(&lightpos),
		DirectX::XMLoadFloat3(&target),
		DirectX::XMLoadFloat3(&up)
	);

	// 正射影行列の算出
	lightproj = XMMatrixOrthographicLH(40, 40, 0.1f,1000.f);

	// ワールド行列の計算
	_wvp.world = DirectX::XMMatrixRotationY(angle);

	// ライトから地面への射影行列（嘘影）の計算
	_wvp.shadow = XMMatrixShadow(XMLoadFloat4(&plane), XMLoadFloat3(&lightVec));

	// ビュー行列の計算（視点から注視点への行列）
	_wvp.view = DirectX::XMMatrixLookAtLH(DirectX::XMLoadFloat3(&eye),DirectX::XMLoadFloat3(&target),DirectX::XMLoadFloat3(&up));

	// プロジェクション行列の計算
	_wvp.projection = XMMatrixPerspectiveFovLH(XM_PIDIV4,static_cast<float>(wsize.width) / static_cast<float>(wsize.height),0.1f,1000.0f);

	// ライトビュープロジェクションの算出
	_wvp.lvp = lightview * lightproj;

	// サイズを調整
	size_t size = sizeof(_wvp);
	size = (size + 0xff)&~0xff;

	// コンスタン十バッファ用のヒーププロパティ
	D3D12_HEAP_PROPERTIES cbvHeapProp = {};
	cbvHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	cbvHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	cbvHeapProp.CreationNodeMask = 1;
	cbvHeapProp.VisibleNodeMask = 1;
	cbvHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

	// コンスタントバッファの作成
	result = _dev->CreateCommittedResource(&cbvHeapProp,
		D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_wvpBuff));

	// Mapping
	result = _wvpBuff->Map(0,nullptr,(void**)&_wvpMP);
	std::memcpy(_wvpMP,&_wvp,sizeof(_wvp));

	// コンスタントバッファ用のデスク
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _wvpBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = static_cast<unsigned int>(size);

	auto handle = _wvpDescHeap->GetCPUDescriptorHandleForHeapStart();
	// コンスタントバッファビューの作成
	_dev->CreateConstantBufferView(&cbvDesc, handle);

	return result;
}

// 深度ステンシルビューの作成
HRESULT Dx12Wrapper::CreateDepthStencilView()
{
	Size wsize = Application::Instance().GetWindowSize();

	D3D12_DESCRIPTOR_HEAP_DESC _dsvHeapDesc = {};
	_dsvHeapDesc.NumDescriptors = 1;
	_dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	auto result = _dev->CreateDescriptorHeap(&_dsvHeapDesc, IID_PPV_ARGS(&_dsvDescHeap));
	result = _dev->CreateDescriptorHeap(&_dsvHeapDesc, IID_PPV_ARGS(&_lightDescHeap));

	_dsvHeapDesc.NumDescriptors = 1;
	_dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	_dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	// シェーダーリソースデスクリプタヒープの作成
	result = _dev->CreateDescriptorHeap(&_dsvHeapDesc, IID_PPV_ARGS(&_dsvSrvHeap));
	result = _dev->CreateDescriptorHeap(&_dsvHeapDesc, IID_PPV_ARGS(&_lightSrvHeap));

	// DSV用のリソースデスク
	D3D12_RESOURCE_DESC depthResDesc = {};
	depthResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthResDesc.Width = wsize.width;
	depthResDesc.Height = wsize.height;
	depthResDesc.DepthOrArraySize = 1;
	depthResDesc.Format = DXGI_FORMAT_R32_TYPELESS;
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

	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&_depthClearValue,
		IID_PPV_ARGS(&_depthBuffer));

	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&_depthClearValue,
		IID_PPV_ARGS(&_lightBuffer));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvdec = {};
	dsvdec.Format = DXGI_FORMAT_D32_FLOAT;
	dsvdec.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvdec.Texture2D.MipSlice = 0;
	dsvdec.Flags = D3D12_DSV_FLAG_NONE;
	
	auto heapstart = _dsvDescHeap->GetCPUDescriptorHandleForHeapStart();

	_dev->CreateDepthStencilView(_depthBuffer, &dsvdec, heapstart);

	heapstart = _lightDescHeap->GetCPUDescriptorHandleForHeapStart();

	_dev->CreateDepthStencilView(_lightBuffer, &dsvdec, heapstart);// ライトから見た深度を書き込むヒープ

	
	// シェーダーリソースビューの作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	auto srvstart = _dsvSrvHeap->GetCPUDescriptorHandleForHeapStart();

	_dev->CreateShaderResourceView(_depthBuffer, &srvDesc, srvstart);

	srvstart = _lightSrvHeap->GetCPUDescriptorHandleForHeapStart();

	_dev->CreateShaderResourceView(_lightBuffer, &srvDesc, srvstart);

	return result;
}

// 待ちを行う関数
void Dx12Wrapper::WaitWithFence()
{
	int cnt = 0;
	while (_fence->GetCompletedValue() != _fenceValue){
		//びじーるぷ
	};
}

// コマンドの追加とシグナルの送信を行う
void Dx12Wrapper::ExecuteCommand()
{
	_cmdQue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&_cmdList);
	_cmdQue->Signal(_fence, ++_fenceValue);
}

/* ポリゴン系関数(あとでクラスに分ける予定) */
// 板ポリ用の頂点バッファ作成
HRESULT Dx12Wrapper::CreatePolygonVertexBuffer()
{
	Vertex vertices[] = {
		XMFLOAT3(-1,-1,0),XMFLOAT2(0,1),//左下
		XMFLOAT3(-1,1,0),XMFLOAT2(0,0),//左上
		XMFLOAT3(1,-1,0),XMFLOAT2(1,1),//右下
		XMFLOAT3(1,1,0),XMFLOAT2(1,0),//右上
	};

	//ペラバッファ生成
	auto result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_peraVertBuff)
	);	Vertex* mapver = nullptr;	result = _peraVertBuff->Map(0, nullptr, (void**)&mapver);
	std::copy(std::begin(vertices), std::end(vertices), mapver);	_peraVertBuff->Unmap(0, nullptr);	_peravbView.BufferLocation = _peraVertBuff->GetGPUVirtualAddress();
	_peravbView.SizeInBytes = sizeof(vertices);
	_peravbView.StrideInBytes = sizeof(Vertex);

	return result;
}

// 一枚目ポリゴンの作成
HRESULT Dx12Wrapper::CreateFirstPolygon()
{
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = 1;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	// レンダーターゲットデスクリプタヒープの作成
	auto result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&_rtvDescHeap));

	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NumDescriptors = 1;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;// タイプ変更

	// シェーダーリソースデスクリプタヒープの作成
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&_srvDescHeap));

	// リソースの作成
	D3D12_RESOURCE_DESC desc = {};
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = Application::Instance().GetWindowSize().width;
	desc.Height = Application::Instance().GetWindowSize().height;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.DepthOrArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.MipLevels = 1;


	D3D12_CLEAR_VALUE clear = { DXGI_FORMAT_R8G8B8A8_UNORM ,{0.5f,0.5f,0.5f,0.0f}, };

	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		&clear,
		IID_PPV_ARGS(&_peraBuffer));

	// レンダーターゲットビューの作成
	_dev->CreateRenderTargetView(_peraBuffer, nullptr, _rtvDescHeap->GetCPUDescriptorHandleForHeapStart());

	// シェーダーリソースビューの作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = desc.Format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	_dev->CreateShaderResourceView(_peraBuffer, &srvDesc, _srvDescHeap->GetCPUDescriptorHandleForHeapStart());
	return result;
}

// 二枚目ポリゴンの作成
HRESULT Dx12Wrapper::CreateSecondPolygon()
{
	// デスクリプタヒープ作成用の情報構造体
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = 1;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	// レンダーターゲットデスクリプタヒープの作成
	auto result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&_rtvDescHeap2));

	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NumDescriptors = 1;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;// タイプ変更

	// シェーダーリソースデスクリプタヒープの作成
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&_srvDescHeap2));

	// リソースの作成用情報構造体
	D3D12_RESOURCE_DESC desc = {};
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = Application::Instance().GetWindowSize().width;
	desc.Height = Application::Instance().GetWindowSize().height;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.DepthOrArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.MipLevels = 1;

	// クリアバリュー
	D3D12_CLEAR_VALUE clear = { DXGI_FORMAT_R8G8B8A8_UNORM ,{0.5f,0.5f,0.5f,0.0f}, };

	// リソース作成
	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		&clear,
		IID_PPV_ARGS(&_peraBuffer2));

	// レンダーターゲットビューの作成
	_dev->CreateRenderTargetView(_peraBuffer2, nullptr, _rtvDescHeap2->GetCPUDescriptorHandleForHeapStart());

	// シェーダーリソースビューの作成用情報構造体
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = desc.Format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	// シェーダーリソースビューの作成
	_dev->CreateShaderResourceView(_peraBuffer2, &srvDesc, _srvDescHeap2->GetCPUDescriptorHandleForHeapStart());
	return result;
}

// 一枚目ポリゴン用のルートシグネチャの作成
HRESULT Dx12Wrapper::CreateFirstSignature()
{
	// サンプラの設定
	D3D12_STATIC_SAMPLER_DESC SamplerDesc[1] = {};
	SamplerDesc[0].Filter = D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR;// 線形補完する
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

	// レンジの設定
	D3D12_DESCRIPTOR_RANGE descRange[3] = {};// 一枚ポリゴン
	descRange[0].NumDescriptors = 1;
	descRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;// シェーダーリソース
	descRange[0].BaseShaderRegister = 0;//レジスタ番号
	descRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// 深度
	descRange[1].NumDescriptors = 1;
	descRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;// シェーダーリソース
	descRange[1].BaseShaderRegister = 1;//レジスタ番号
	descRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ライトからの深度
	descRange[2].NumDescriptors = 1;
	descRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;// シェーダーリソース
	descRange[2].BaseShaderRegister = 2;//レジスタ番号
	descRange[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ルートパラメータ変数の設定
	D3D12_ROOT_PARAMETER rootParam[3] = {};
	rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[0].DescriptorTable.pDescriptorRanges = &descRange[0];//対応するレンジへのポインタ
	rootParam[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//すべてのシェーダから参照

	// カメラからの深度
	rootParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[1].DescriptorTable.pDescriptorRanges = &descRange[1];//対応するレンジへのポインタ
	rootParam[1].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//すべてのシェーダから参照

	// ライトからの深度
	rootParam[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[2].DescriptorTable.pDescriptorRanges = &descRange[2];//対応するレンジへのポインタ
	rootParam[2].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//すべてのシェーダから参照

	// ルートシグネチャを作るための変数の設定
	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.pStaticSamplers = SamplerDesc;
	rsd.pParameters = rootParam;
	rsd.NumParameters = 3;
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
		IID_PPV_ARGS(&_peraSignature));

	return result;
}

// 一枚目ポリゴン用のパイプラインの作成
HRESULT Dx12Wrapper::CreateFirstPopelineState()
{
	auto result = D3DCompileFromFile(L"PeraShader.hlsl", nullptr, nullptr, "peraVS", "vs_5_0", D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION, 0, &peraVertShader, nullptr);

	// ピクセルシェーダ
	result = D3DCompileFromFile(L"PeraShader.hlsl", nullptr, nullptr, "peraPS", "ps_5_0", D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION, 0, &peraPixShader, nullptr);

	D3D12_INPUT_ELEMENT_DESC peraLayoutDescs[] =
	{
		// Pos
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0, D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		// UV
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};

	// パイプラインを作るためのGPSの変数の作成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};

	//ルートシグネチャと頂点レイアウト
	gpsDesc.pRootSignature = _peraSignature;
	gpsDesc.InputLayout.pInputElementDescs = peraLayoutDescs;
	gpsDesc.InputLayout.NumElements = _countof(peraLayoutDescs);

	//シェーダのセット
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(peraVertShader);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(peraPixShader);

	//レンダーターゲット
	gpsDesc.NumRenderTargets = 1;
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	//深度ステンシル
	gpsDesc.DepthStencilState.DepthEnable = false;
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

	result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&_peraPipeline));

	return result;
}

// 二枚目ポリゴン用のルートシグネチャの作成
HRESULT Dx12Wrapper::CreateSecondSignature()
{
	// サンプラの設定
	D3D12_STATIC_SAMPLER_DESC SamplerDesc[1] = {};
	SamplerDesc[0].Filter = D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR;// 線形補完する
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

	// レンジの設定
	D3D12_DESCRIPTOR_RANGE descRange[1] = {};// 一枚ポリゴン
	descRange[0].NumDescriptors = 1;
	descRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;// シェーダーリソース
	descRange[0].BaseShaderRegister = 0;//レジスタ番号
	descRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ルートパラメータ変数の設定
	D3D12_ROOT_PARAMETER rootParam[1] = {};
	rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[0].DescriptorTable.pDescriptorRanges = &descRange[0];//対応するレンジへのポインタ
	rootParam[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//すべてのシェーダから参照

	// ルートシグネチャを作るための変数の設定
	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.pStaticSamplers = SamplerDesc;
	rsd.pParameters = rootParam;
	rsd.NumParameters = 1;
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
		IID_PPV_ARGS(&_peraSignature2));

	return result;
}

// 二枚目ポリゴン用のパイプラインの作成
HRESULT Dx12Wrapper::CreateSecondPopelineState()
{
	auto result = D3DCompileFromFile(L"pera2.hlsl", nullptr, nullptr, "pera2VS", "vs_5_0", D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION, 0, &peraVertShader2, nullptr);

	// ピクセルシェーダ
	result = D3DCompileFromFile(L"pera2.hlsl", nullptr, nullptr, "pera2PS", "ps_5_0", D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION, 0, &peraPixShader2, nullptr);

	D3D12_INPUT_ELEMENT_DESC peraLayoutDescs[] =
	{
		// Pos
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0, D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		// UV
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};

	// パイプラインを作るためのGPSの変数の作成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};

	//ルートシグネチャと頂点レイアウト
	gpsDesc.pRootSignature = _peraSignature2;
	gpsDesc.InputLayout.pInputElementDescs = peraLayoutDescs;
	gpsDesc.InputLayout.NumElements = _countof(peraLayoutDescs);

	//シェーダのセット
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(peraVertShader2);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(peraPixShader2);

	//レンダーターゲット
	gpsDesc.NumRenderTargets = 1;
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	//深度ステンシル
	gpsDesc.DepthStencilState.DepthEnable = false;
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

	result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&_peraPipeline2));

	return result;
}

// 一枚目ポリゴンの描画
void Dx12Wrapper::DrawFirstPolygon()
{
	// ヒープの開始位置を取得
	auto heapStart = _rtvDescHeap2->GetCPUDescriptorHandleForHeapStart();// 二枚目のポリゴンに描画
	auto depthStart = _dsvDescHeap->GetCPUDescriptorHandleForHeapStart();
	auto lightStart = _lightDescHeap->GetCPUDescriptorHandleForHeapStart();

	// バリアの情報設定
	D3D12_RESOURCE_BARRIER BarrierDesc = {};
	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.Transition.pResource = _peraBuffer2;// 二枚目のポリゴンに描画
	BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	
	// コマンドアロケータとリストのリセット
	_cmdAlloc->Reset();
	_cmdList->Reset(_cmdAlloc, nullptr);

	// パイプラインのセット
	_cmdList->SetPipelineState(_peraPipeline);

	// ルートシグネチャをセット
	_cmdList->SetGraphicsRootSignature(_peraSignature);

	//ビューポートとシザー設定
	_cmdList->RSSetViewports(1, &_viewPort);
	_cmdList->RSSetScissorRects(1, &_scissor);

	// リソースバリア
	_cmdList->ResourceBarrier(1, &BarrierDesc);

	// レンダーターゲットの設定
	_cmdList->OMSetRenderTargets(1, &heapStart, false, nullptr);
	_cmdList->ClearRenderTargetView(heapStart, clearColor, 0, nullptr);

	// 頂点バッファビューの設定
	_cmdList->IASetVertexBuffers(0, 1, &_peravbView);

	// トポロジーのセット
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// デスクリプタのセット
	_cmdList->SetDescriptorHeaps(1,&_srvDescHeap);
	_cmdList->SetGraphicsRootDescriptorTable(0, _srvDescHeap->GetGPUDescriptorHandleForHeapStart());

	// カメラ深度のセット
	auto dstart = _dsvSrvHeap->GetGPUDescriptorHandleForHeapStart();
	_cmdList->SetDescriptorHeaps(1, &_dsvSrvHeap);
	_cmdList->SetGraphicsRootDescriptorTable(1, dstart);

	// ライト深度のセット
	auto lstart = _lightSrvHeap->GetGPUDescriptorHandleForHeapStart();
	_cmdList->SetDescriptorHeaps(1, &_lightSrvHeap);
	_cmdList->SetGraphicsRootDescriptorTable(2, lstart);

	// ポリゴンの描画
	_cmdList->DrawInstanced(4, 1, 0, 0);

	// バリアをもとに戻す
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	_cmdList->ResourceBarrier(1, &BarrierDesc);

	// リストのクローズ
	_cmdList->Close();
}

// 二枚目ポリゴンの描画
void Dx12Wrapper::DrawSecondPolygon()
{
	// ヒープの開始位置を取得
	auto heapStart = _swcDescHeap->GetCPUDescriptorHandleForHeapStart();
	auto depthStart = _dsvDescHeap->GetCPUDescriptorHandleForHeapStart();

	// バックバッファのインデックスをとってきて入れ替える
	auto bbIndex = _swapchain->GetCurrentBackBufferIndex();
	int DescriptorSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	heapStart.ptr += bbIndex * DescriptorSize;// ポインタをバッファサイズ分ずらす

	// バリアの情報設定
	D3D12_RESOURCE_BARRIER BarrierDesc = {};
	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.Transition.pResource = renderTargets[bbIndex];
	BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	
	// コマンドアロケータとリストのリセット
	_cmdAlloc->Reset();
	_cmdList->Reset(_cmdAlloc, nullptr);

	// パイプラインとルートシグネチャをセット
	_cmdList->SetPipelineState(_peraPipeline2);
	_cmdList->SetGraphicsRootSignature(_peraSignature2);

	//ビューポートとシザーを設定
	_cmdList->RSSetViewports(1, &_viewPort);
	_cmdList->RSSetScissorRects(1, &_scissor);

	// リソースバリアを張る
	_cmdList->ResourceBarrier(1, &BarrierDesc);

	// レンダーターゲットの設定
	_cmdList->OMSetRenderTargets(1, &heapStart, false, &depthStart);
	_cmdList->ClearRenderTargetView(heapStart, clearColor, 0, nullptr);// クリア

	// 頂点バッファビューの設定
	_cmdList->IASetVertexBuffers(0, 1, &_peravbView);

	// トポロジーのセット
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// デスクリプタのセット
	_cmdList->SetDescriptorHeaps(1, &_srvDescHeap2);
	_cmdList->SetGraphicsRootDescriptorTable(0, _srvDescHeap2->GetGPUDescriptorHandleForHeapStart());

	// ポリゴンの描画
	_cmdList->DrawInstanced(4, 1, 0, 0);

	// バリアをもとに戻す
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	_cmdList->ResourceBarrier(1, &BarrierDesc);

	// リストのクローズ
	_cmdList->Close();
}

/// ポリゴン関数ここまで


// コンストラクタ
Dx12Wrapper::Dx12Wrapper(HWND hwnd) :_hwnd(hwnd)
{
	// イニシャライズ
	auto result = CoInitializeEx(0, COINIT_MULTITHREADED);
}
// デストラクタ
Dx12Wrapper::~Dx12Wrapper()
{
	// キーボードデバイスの解放
	_keyBoadDev->Unacquire();

	if (_keyBoadDev != NULL)
		_keyBoadDev->Release();

	if (_directInput != NULL)
		_directInput->Release();
}

// キー入力更新関数
void Dx12Wrapper::KeyUpDate()
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

	float addsize = 0.5f;
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
		eye.y -= addsize;
		target.y -= addsize;
	}

	if (key[DIK_Q] || key[DIK_LEFT])
	{
		angle += addsize / 10;
	}
	else if (key[DIK_E] || key[DIK_RIGHT])
	{
		angle -= addsize / 10;
	}


	if (key[DIK_W])
	{
		eye.z += addsize;
		target.z += addsize;
	}
	else if (key[DIK_S])
	{
		eye.z -= addsize;
		target.z -= addsize;
	}

	if (key[DIK_D])
	{
		eye.x += addsize;
		target.x += addsize;
	}
	if (key[DIK_A])
	{
		eye.x -= addsize;
		target.x -= addsize;
	}

	if (key[DIK_R])
	{
		eye = XMFLOAT3(0, 10, -25);
		target = XMFLOAT3(0, 10, 0);
		up = XMFLOAT3(0, 1, 0);
	}
	if (key[DIK_P])
	{
		isOk = 0;
	}
	if (key[DIK_P] && (Oldkey[DIK_P] == 0))
	{
		float rota[3] = { XM_PIDIV4 , XM_PIDIV2 ,XM_PI / 30 * XM_PI };
		_persLevel++;
		_wvp.projection = XMMatrixPerspectiveFovLH(
			rota[_persLevel % 3],
			static_cast<float>(wsize.width) / static_cast<float>(wsize.height),
			0.1f,
			1000.0f
		);
	}

	pmdModel->UpDate();// PMDモデルの更新

	// PMXモデルの更新
	for (auto& model : pmxModels)
	{
		model->UpDate(key);
	}

	// 座標が視点に近い順に描画順を並び替える
	std::sort(pmxModels.begin(), pmxModels.end(), [](std::shared_ptr<PMXmodel>& a, std::shared_ptr<PMXmodel>& b)
	{
		return (a->GetPos().z > b->GetPos().z);
	});

	// カメラ
	// 現在のビュー行列を保存
	_wvp.view = DirectX::XMMatrixLookAtLH(
		DirectX::XMLoadFloat3(&eye),
		DirectX::XMLoadFloat3(&target),
		DirectX::XMLoadFloat3(&up));

	_wvp.view = DirectX::XMMatrixRotationY(angle)*_wvp.view;// Y軸を基準に回転する

	_wvp.wvp = _wvp.world;
	_wvp.wvp *= _wvp.view;
	_wvp.wvp *= _wvp.projection;

	// ライト座標の算出

	XMFLOAT3 lightpos;
	XMStoreFloat3(&lightpos,
		XMVector4Normalize(XMLoadFloat3(&lightVec)) * XMVector3Length(XMLoadFloat3(&target) - XMLoadFloat3(&eye)));//ライト座標

	// ライトから注視点への行列の計算
	XMMATRIX lightview = DirectX::XMMatrixLookAtLH(
		DirectX::XMLoadFloat3(&lightpos),
		DirectX::XMLoadFloat3(&target),
		DirectX::XMLoadFloat3(&up)
	);


	lightview = DirectX::XMMatrixRotationY(angle)*lightview;

	_wvp.lvp = lightview * lightproj;

	*_wvpMP = _wvp;// 転送用に書き込む
}

// 画面描画関数
void Dx12Wrapper::ScreenUpDate()
{
	auto wsize = Application::Instance().GetWindowSize();

	D3D12_RESOURCE_BARRIER BarrierDesc = {};
	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.Transition.pResource = _peraBuffer;// 一枚目のポリゴンに描画
	BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	// 深度バッファビューヒープの開始位置をとってくる
	auto depthStart = _dsvDescHeap->GetCPUDescriptorHandleForHeapStart();
	auto lightStart = _lightDescHeap->GetCPUDescriptorHandleForHeapStart();

	auto peraStart = _rtvDescHeap->GetCPUDescriptorHandleForHeapStart();// 一枚目のポリゴンに描画
	
	_cmdAlloc->Reset();
	_cmdList->Reset(_cmdAlloc, nullptr);//コマンドリストリセット

	// シャドウの描画
	// ライト深度のクリア
	_cmdList->ClearDepthStencilView(lightStart, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	//レンダーターゲット設定
	_cmdList->OMSetRenderTargets(0, nullptr, false, &lightStart);

	for (auto &model : pmxModels)
	{
		model->PreDrawShadow(_cmdList, _wvpDescHeap);
	}

	// モデルの描画
	// カメラ深度のクリア
	_cmdList->ClearDepthStencilView(depthStart, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// リソースバリア
	_cmdList->ResourceBarrier(1, &BarrierDesc);

	//レンダーターゲット設定
	_cmdList->OMSetRenderTargets(1, &peraStart, false, &depthStart);

	//レンダーターゲットのクリア
	_cmdList->ClearRenderTargetView(peraStart, clearColor, 0, nullptr);
	   	 
	for (auto &model : pmxModels)
	{
		model->Draw(_cmdList, _wvpDescHeap, _lightSrvHeap);
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

	// ポリゴンの描画
	DrawFirstPolygon();

	// コマンド追加とフェンス
	ExecuteCommand();

	// 待ち
	WaitWithFence();

	// ポリゴンの描画
	DrawSecondPolygon();

	// コマンド追加とフェンス
	ExecuteCommand();

	// 待ち
	WaitWithFence();

	// 画面の更新
	_swapchain->Present(1, 0);
}

// 初期化
int Dx12Wrapper::Init()
{
	// ﾃﾞﾊﾞｯｸﾞﾚｲﾔｰをON
#if defined(_DEBUG)
	ID3D12Debug* debug;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
		debug->EnableDebugLayer();
		debug->Release();
	}
#endif
	// ビューポートの設定
	_viewPort.TopLeftX = 0;
	_viewPort.TopLeftY = 0;
	_viewPort.Width = static_cast<float>(Application::Instance().GetWindowSize().width);
	_viewPort.Height = static_cast<float>(Application::Instance().GetWindowSize().height);
	_viewPort.MaxDepth = 1.0f;
	_viewPort.MinDepth = 0.0f;

	// シザーの設定
	_scissor.left = 0;
	_scissor.top = 0;
	_scissor.right = Application::Instance().GetWindowSize().width;
	_scissor.bottom = Application::Instance().GetWindowSize().height;

	// デバイスの生成
	if (FAILED(DeviceInit()))
		return 1;

	// モデル読み込み
	pmdModel = std::make_shared<PMDmodel>(_dev, "model/PMD/初音ミク.pmd");
	//pmxModels.emplace_back(std::make_shared<PMXmodel>(_dev, "model/PMX/ちびフラン/ちびフラン.pmx", "VMD/45秒MIKU.vmd"));
	//pmxModels.emplace_back(std::make_shared<PMXmodel>(_dev, "model/PMX/2B/na_2b_0407.pmx", "VMD/45秒GUMI.vmd"));
	//pmxModels.emplace_back(std::make_shared<PMXmodel>(_dev, "model/PMX/GUMI/GUMIβ_V3.pmx", "VMD/DanceRobotDance_Motion.vmd"));
	//pmxModels.emplace_back(std::make_shared<PMXmodel>(_dev, "model/PMX/ちびルーミア/ちびルーミア標準ボーン.pmx", "VMD/45秒GUMI.vmd"));
	pmxModels.emplace_back(std::make_shared<PMXmodel>(_dev, "model/PMX/ちびルーミア/ちびルーミア.pmx", "VMD/ヤゴコロダンス.vmd"));

	/* Aポーズ(テスト用 */
	//pmxModels.emplace_back(std::make_shared<PMXmodel>(_dev, "model/PMX/ちびルーミア/ちびルーミア.pmx"));

	if (FAILED(CreateSwapChainAndCmdQue()))
		return 2;

	if (FAILED(CreateCmdListAndAlloc()))
		return 3;

	if (FAILED(CreateFence()))
		return 4;

	if (FAILED(CreateDepthStencilView()))
		return 7;

	if (FAILED(CreateWVPConstantBuffer()))
		return 10;


	if (FAILED(CreateFirstPolygon()))
		return 11;

	if (FAILED(CreateFirstSignature()))
		return 12;

	if (FAILED(CreateFirstPopelineState()))
		return 13;

	if (FAILED(CreateSecondPolygon()))
		return 14;

	if (FAILED(CreateSecondSignature()))
		return 15;

	if (FAILED(CreateSecondPopelineState()))
		return 16;

	if (FAILED(CreatePolygonVertexBuffer()))
		return 17;


	return 0;
}

// ラッパーの更新
void Dx12Wrapper::UpDate()
{
	KeyUpDate();
	ScreenUpDate();
}
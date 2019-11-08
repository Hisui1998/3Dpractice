#pragma once
#include <windows.h>
#include <d3dx12.h>
#include <d3d12.h>
#include <dinput.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <vector>
#include <map>

class PMDmodel;
class PMXmodel;

// 行列の構造体
struct WVPMatrix {
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX view;
	DirectX::XMMATRIX projection;
	DirectX::XMMATRIX wvp;
	DirectX::XMMATRIX lvp;
};

class Dx12Wrapper
{
private:
	//  ---デバイス系変数---  //
	HWND _hwnd;
	IDXGIFactory6* _dxgi = nullptr;
	IDXGISwapChain4* _swapchain = nullptr;
	ID3D12Device* _dev = nullptr;
	ID3D12Device* _keydev = nullptr;
	ID3D12CommandAllocator* _cmdAlloc = nullptr;
	ID3D12GraphicsCommandList* _cmdList = nullptr;
	ID3D12CommandQueue* _cmdQue = nullptr;

	LPDIRECTINPUT8       _directInput;// DirectInput本体
	LPDIRECTINPUTDEVICE8 _keyBoadDev;// キーボードデバイス

	ID3D12Fence* _fence = nullptr;
	UINT64 _fenceValue = 0;

	ID3D12DescriptorHeap* _swcDescHeap = nullptr;// SWC(スワップチェイン)デスクリプタヒープ
	ID3D12DescriptorHeap* _rtvDescHeap = nullptr;// RTV(レンダーターゲット)デスクリプタヒープ
	ID3D12DescriptorHeap* _dsvDescHeap;// DSV(深度)デスクリプタヒープ
	ID3D12DescriptorHeap* _depthSrvHeap;// 深度シェーダーリソースビューヒープ
	ID3D12DescriptorHeap* _srvDescHeap;// その他(テクスチャ、定数)デスクリプタヒープ
	ID3D12DescriptorHeap* _rgstDescHeap;

	std::vector<ID3D12Resource*>renderTargets;

	ID3D12Resource* _constBuff = nullptr;// 定数バッファ
	ID3D12Resource* _depthBuffer = nullptr;// 深度バッファ

	ID3DBlob* vertexShader = nullptr;
	ID3DBlob* pixelShader = nullptr;

	ID3D12RootSignature* _rootSignature = nullptr;
	ID3DBlob* signature = nullptr;
	ID3DBlob* error = nullptr;

	ID3D12PipelineState* _pipelineState = nullptr;

	/*　作成と初期化系関数　*/
	// デバイスの初期化
	HRESULT DeviceInit();

	// スワップチェインとコマンドキューの作成
	HRESULT CreateSwapChainAndCmdQue();

	// コマンドリストとコマンドアロケータの作成
	HRESULT CreateCmdListAndAlloc();

	// ルートシグネチャを作る関数(レンジとパラメータもこの中)
	HRESULT CreateRootSignature();

	// フェンスの作成
	HRESULT CreateFence();

	// パイプラインを作る関数(頂点レイアウトの設定はこの中)
	HRESULT CreateGraphicsPipelineState();

	// 深度バッファと深度バッファビューを作る関数
	HRESULT CreateDSV();

	// 定数バッファの作成
	HRESULT CreateConstantBuffer();

	// ---その他関数--- //
	// シェーダーのよみこみを行う関数
	HRESULT LoadShader();
	
	// バッファに画を書き終わるまでウェイトをかける関数
	void WaitWithFence();

	// コマンドを処理し終わったかどうかを返す関数
	void ExecuteCommand();

	//　---以下変数--- //
	
	WVPMatrix* _wvpMP;
	WVPMatrix _wvp;
	float angle;// 角度
	int cnt = 0;
	char key[256] = {};

	DirectX::XMFLOAT3 eye;
	DirectX::XMFLOAT3 target;
	DirectX::XMFLOAT3 up;

	std::shared_ptr<PMDmodel> pmdModel;
	std::shared_ptr<PMXmodel> pmxModel;
	bool isPMD = false;
public:
	Dx12Wrapper(HWND hwnd);
	~Dx12Wrapper();

	// 初期化
	int Init();

	// 更新
	void UpDate();
};


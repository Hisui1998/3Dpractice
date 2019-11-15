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

// 頂点情報
struct Vertex
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT2 uv;
};

class Dx12Wrapper
{
private:
	//  ---デバイス系変数---  //
	HWND _hwnd;// ウィンドウハンドル

	IDXGIFactory6* _dxgi = nullptr;// ファクトリ

	IDXGISwapChain4* _swapchain = nullptr;// スワップチェイン

	ID3D12Device* _dev = nullptr;// デバイス
	ID3D12Device* _keydev = nullptr;// キーボードデバイス

	// コマンド系
	ID3D12CommandAllocator* _cmdAlloc = nullptr;// コマンドアロケータ
	ID3D12GraphicsCommandList* _cmdList = nullptr;// コマンドリスト
	ID3D12CommandQueue* _cmdQue = nullptr;// コマンドキュー

	LPDIRECTINPUT8       _directInput;// DirectInput本体
	LPDIRECTINPUTDEVICE8 _keyBoadDev;// キーボードデバイス

	ID3D12Fence* _fence = nullptr;// フェンス
	UINT64 _fenceValue = 0;// フェンス値

	ID3D12DescriptorHeap* _dsvDescHeap;// DSV(深度)デスクリプタヒープ
	ID3D12Resource* _depthBuffer = nullptr;// 深度バッファ

	ID3D12DescriptorHeap* _cameraDescHeap;// カメラ用の定数バッファ用デスクリプタヒープ
	ID3D12Resource* _cameraBuff = nullptr;// カメラ用の定数バッファ

	D3D12_VIEWPORT _viewPort;// ビューポート
	D3D12_RECT _scissor;// シザー範囲

	// ペラポリ用
	ID3D12Resource* _peraBuffer = nullptr;// ペラポリ本体のバッファ
	ID3D12Resource* _peraBuffer2 = nullptr;// ペラポリ2本体のバッファ

	ID3D12Resource* _peraVertBuff = nullptr;// ペラポリ用頂点バッファ
	D3D12_VERTEX_BUFFER_VIEW _peravbView = {};// ペラポリ用頂点バッファビュー

	ID3D12DescriptorHeap* _rtvDescHeap = nullptr;// ペラポリ用レンダーターゲットデスクリプタヒープ	
	ID3D12DescriptorHeap* _rtvDescHeap2 = nullptr;// ペラポリ2用レンダーターゲットデスクリプタヒープ	
	ID3D12DescriptorHeap* _srvDescHeap = nullptr;// ペラポリ用シェーダーリソースデスクリプタヒープ
	ID3D12DescriptorHeap* _srvDescHeap2 = nullptr;// ペラポリ2用シェーダーリソースデスクリプタヒープ

	ID3DBlob* peraVertShader = nullptr;// ペラポリ用頂点シェーダ
	ID3DBlob* peraVertShader2 = nullptr;// ペラポリ2用頂点シェーダ
	ID3DBlob* peraPixShader = nullptr;// ペラポリ用ピクセルシェーダ
	ID3DBlob* peraPixShader2 = nullptr;// ペラポリ2用ピクセルシェーダ

	ID3D12PipelineState* _peraPipeline = nullptr;// ペラポリ用パイプライン
	ID3D12RootSignature* _peraSignature = nullptr;// ペラポリ用ルートシグネチャ
	

	// スワップチェイン用
	ID3D12DescriptorHeap* _swcDescHeap = nullptr;// SWC(スワップチェイン)デスクリプタヒープ		

	ID3D12RootSignature* _rootSignature = nullptr;
	ID3DBlob* signature = nullptr;
	ID3DBlob* error = nullptr;

	ID3D12PipelineState* _pipelineState = nullptr;
	std::vector<ID3D12Resource*>renderTargets;

	ID3DBlob* vertexShader = nullptr;// 頂点シェーダ
	ID3DBlob* pixelShader = nullptr;// ピクセルシェーダ

	/*　作成と初期化系関数　*/
	// デバイスの初期化
	HRESULT DeviceInit();

	// スワップチェインとコマンドキューの作成
	HRESULT CreateSwapChainAndCmdQue();

	// コマンドリストとコマンドアロケータの作成
	HRESULT CreateCmdListAndAlloc();

	// ルートシグネチャを作る関数(レンジとパラメータもこの中)
	HRESULT CreateRootSignature();

	HRESULT CreatePeraRootSignature();

	// フェンスの作成
	HRESULT CreateFence();

	// パイプラインを作る関数(頂点レイアウトの設定はこの中)
	HRESULT CreateGraphicsPipelineState();

	HRESULT CreatePeraPopelineState();

	// 深度バッファと深度バッファビューを作る関数
	HRESULT CreateDSV();

	// 定数バッファの作成
	HRESULT CreateConstantBuffer();

	// ヒープとビューの作成
	HRESULT CreateHeapAndView();

	// ぺらぽりの作成
	HRESULT CreatePolygon();

	void DrawPolygon();

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

	// カメラ用
	DirectX::XMFLOAT3 eye;// カメラの座標
	DirectX::XMFLOAT3 target;// ターゲットの座標
	DirectX::XMFLOAT3 up;// 方向

	std::shared_ptr<PMDmodel> pmdModel;
	std::vector<std::shared_ptr<PMXmodel>> pmxModels;
	bool isPMD = false;
public:
	Dx12Wrapper(HWND hwnd);
	~Dx12Wrapper();

	// 初期化
	int Init();

	// 更新
	void UpDate();
};

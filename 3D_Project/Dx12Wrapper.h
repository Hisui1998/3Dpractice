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
	DirectX::XMMATRIX shadow;
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
	// ラッパー用関数
	void KeyUpDate();// キー情報更新
	void ScreenUpDate();// 画面の更新

	// デバイスの初期化
	HRESULT DeviceInit();

	// スワップチェインとコマンドキューの作成
	HRESULT CreateSwapChainAndCmdQue();

	// コマンドリストとコマンドアロケータの作成
	HRESULT CreateCmdListAndAlloc();

	// フェンスの作成
	HRESULT CreateFence();

	// 深度バッファと深度バッファビューを作る関数
	HRESULT CreateDepthStencilView();

	// WVP用定数バッファの作成
	HRESULT CreateWVPConstantBuffer();

	// バッファに画を書き終わるまでウェイトをかける関数
	void WaitWithFence();

	// コマンドを処理し終わったかどうかを返す関数
	void ExecuteCommand();

	// ラッパー用変数
	HWND _hwnd;// ウィンドウハンドル
	IDXGIFactory6* _dxgi = nullptr;// ファクトリ
	IDXGISwapChain4* _swapchain = nullptr;// スワップチェイン
	ID3D12Device* _dev = nullptr;// デバイス

	LPDIRECTINPUT8       _directInput;// DirectInput本体
	LPDIRECTINPUTDEVICE8 _keyBoadDev;// キーボードデバイス

	// コマンド系
	ID3D12CommandAllocator* _cmdAlloc = nullptr;// コマンドアロケータ
	ID3D12GraphicsCommandList* _cmdList = nullptr;// コマンドリスト
	ID3D12CommandQueue* _cmdQue = nullptr;// コマンドキュー

	ID3D12Fence* _fence = nullptr;// フェンス
	UINT64 _fenceValue = 0;// フェンス値

	// 深度バッファ用
	ID3D12Resource* _depthBuffer = nullptr;// 深度バッファ
	ID3D12DescriptorHeap* _dsvDescHeap;// 深度バッファデスクリプタヒープ
	ID3D12DescriptorHeap* _dsvSrvHeap;// 深度SRVデスクリプタヒープ	

	// スワップチェイン用
	ID3D12DescriptorHeap* _swcDescHeap = nullptr;// SWC(スワップチェイン)デスクリプタヒープ
	std::vector<ID3D12Resource*>renderTargets;// スワップチェインで使うバッファのRTV	

	int _persLevel = 0;// パースのレベル
	char key[256] = {};

	D3D12_VIEWPORT _viewPort;// ビューポート
	D3D12_RECT _scissor;// シザー範囲

	// WVP用
	ID3D12Resource* _wvpBuff = nullptr;// WVP用の定数バッファ
	ID3D12DescriptorHeap* _wvpDescHeap;// WVP用の定数バッファデスクリプタヒープ
	WVPMatrix* _wvpMP;
	WVPMatrix _wvp;
	float angle;// 角度
	DirectX::XMFLOAT3 eye;// 視点の座標
	DirectX::XMFLOAT3 target;// どこを見ているかの座標
	DirectX::XMFLOAT3 up;// 軸

	std::shared_ptr<PMDmodel> pmdModel;
	std::vector<std::shared_ptr<PMXmodel>> pmxModels;


	/*板ポリ用関数(あとでクラスに分ける予定)*/
	// 板ポリゴン用の頂点バッファの作成
	HRESULT CreatePolygonVertexBuffer();

	// 一枚目ポリゴン用ルートシグネチャの作成
	HRESULT CreateFirstSignature();

	// 二枚目ポリゴン用ルートシグネチャの作成
	HRESULT CreateSecondSignature();

	// 一枚目ポリゴン用パイプライン生成
	HRESULT CreateFirstPopelineState();

	// 二枚目ポリゴン用パイプライン生成
	HRESULT CreateSecondPopelineState();

	// 一枚目ポリゴンの作成
	HRESULT CreateFirstPolygon();

	// 一枚目ポリゴンの作成
	HRESULT CreateSecondPolygon();

	// ポリゴン描画関数
	void DrawFirstPolygon();
	void DrawSecondPolygon();

	// ペラポリ用変数
	ID3D12Resource* _peraBuffer = nullptr;// ペラポリ本体のバッファ

	ID3D12Resource* _peraBuffer2 = nullptr;// ペラポリ2本体のバッファ

	ID3D12Resource* _peraVertBuff = nullptr;// ペラポリ用頂点バッファ
	D3D12_VERTEX_BUFFER_VIEW _peravbView = {};// ペラポリ用頂点バッファビュー

	ID3D12DescriptorHeap* _rtvDescHeap = nullptr;// ペラポリ用レンダーターゲットデスクリプタヒープ	
	ID3D12DescriptorHeap* _srvDescHeap = nullptr;// ペラポリ用シェーダーリソースデスクリプタヒープ

	ID3D12DescriptorHeap* _rtvDescHeap2 = nullptr;// ペラポリ2用レンダーターゲットデスクリプタヒープ	
	ID3D12DescriptorHeap* _srvDescHeap2 = nullptr;// ペラポリ2用シェーダーリソースデスクリプタヒープ

	ID3DBlob* peraVertShader = nullptr;// ペラポリ用頂点シェーダ
	ID3DBlob* peraPixShader = nullptr;// ペラポリ用ピクセルシェーダ

	ID3DBlob* peraVertShader2 = nullptr;// ペラポリ2用頂点シェーダ
	ID3DBlob* peraPixShader2 = nullptr;// ペラポリ2用ピクセルシェーダ

	ID3D12PipelineState* _peraPipeline = nullptr;// ペラポリ用パイプライン
	ID3D12RootSignature* _peraSignature = nullptr;// ペラポリ用ルートシグネチャ

	ID3D12PipelineState* _peraPipeline2 = nullptr;// ペラポリ2用パイプライン
	ID3D12RootSignature* _peraSignature2 = nullptr;// ペラポリ2用ルートシグネチャ

	// ルートシグネチャの作成時に使う変数
	ID3DBlob* signature = nullptr;
	ID3DBlob* error = nullptr;
	/// 板ポリ用ここまで
public:
	Dx12Wrapper(HWND hwnd);
	~Dx12Wrapper();

	// 初期化
	int Init();

	// 更新
	void UpDate();
};

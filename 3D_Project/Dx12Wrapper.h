#pragma once
#include <windows.h>
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <vector>
#include <DirectXMath.h>

// PMDファイルのヘッダ情報の構造体
struct PMDHeader {
	char signature[3];//pmd
	float version;// 00 00 80 3f==1.00
	char model_name[20];
	char comment[256];
};

/// パディングを１バイト単位にするやつ
#pragma pack (1)
// 頂点情報構造体
struct VertexInfo
{
	float pos[3];//12
	float normal_vec[3];//12
	float uv[2];//8
	unsigned short bone_num[2];//4
	unsigned char bone_weight;//1
	unsigned char edge_flag;//1
};

// PMDファイルのマテリアル構造体
struct PMDMaterial {
	DirectX::XMFLOAT3 diffuse_color;// 12
	float alpha;// 4
	float specularity;// 4
	DirectX::XMFLOAT3 specular_color;// 12
	DirectX::XMFLOAT3 mirror_color;// 12
	unsigned char toon_index;// 1
	unsigned char edge_flag;// 1
	// ここで２バイトあまる
	unsigned int indexNum;// 4
	char texFileName[20];//20
};
#pragma pack()
/// パディングを元に戻す()４バイト

// 色情報の構造体
struct PMDColor
{
	DirectX::XMFLOAT4 diffuse;
	DirectX::XMFLOAT4 specular;
	DirectX::XMFLOAT3 ambient;
};

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
	ID3D12CommandAllocator* _cmdAlloc = nullptr;
	ID3D12GraphicsCommandList* _cmdList = nullptr;
	ID3D12CommandQueue* _cmdQue = nullptr;

	D3D12_VERTEX_BUFFER_VIEW _vbView = {};
	D3D12_INDEX_BUFFER_VIEW _ibView = {};

	ID3D12Fence* _fence = nullptr;
	UINT64 _fenceValue = 0;

	ID3D12DescriptorHeap* _swcDescHeap = nullptr;// SWC(スワップチェイン)デスクリプタヒープ
	ID3D12DescriptorHeap* _rtvDescHeap = nullptr;// RTV(レンダーターゲット)デスクリプタヒープ
	ID3D12DescriptorHeap* _dsvDescHeap;// DSV(深度)デスクリプタヒープ
	ID3D12DescriptorHeap* _depthSrvHeap;// 深度シェーダーリソースビューヒープ
	ID3D12DescriptorHeap* _srvDescHeap;// その他(テクスチャ、定数)デスクリプタヒープ
	ID3D12DescriptorHeap* _rgstDescHeap;
	ID3D12DescriptorHeap* _matDescHeap;// マテリアルデスクリプタヒープ

	std::vector<ID3D12Resource*>renderTargets;

	ID3D12Resource* _vertexBuffer = nullptr;// 頂点バッファ
	ID3D12Resource* _indexBuffer = nullptr;// インデックスバッファ
	ID3D12Resource* _constBuff = nullptr;// 定数バッファ
	ID3D12Resource* _depthBuffer = nullptr;// 深度バッファ
	std::vector<ID3D12Resource*> _materialsBuff;// マテリアルバッファ(複数あるのでベクターにしている)
	std::vector<ID3D12Resource*> _textureBuffer;// テクスチャバッファ
	ID3D12Resource* whiteBuff = nullptr;

	ID3DBlob* vertexShader = nullptr;
	ID3DBlob* pixelShader = nullptr;

	ID3D12RootSignature* _rootSignature = nullptr;
	ID3DBlob* signature = nullptr;
	ID3DBlob* error = nullptr;

	ID3D12PipelineState* _pipelineState = nullptr;

	//  ---関数---  //
	/// Init系(?)

	// デバイスの初期化
	HRESULT DeviceInit();
	/// Create系(?)

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

	// インデックスバッファと頂点バッファの作成
	HRESULT CreateBuffersForIndexAndVertex();

	// 深度バッファと深度バッファビューを作る関数
	HRESULT CreateDSV();

	// 定数バッファの作成
	HRESULT CreateConstantBuffer();

	// マテリアルバッファとマテリアルバッファビューを作る関数
	HRESULT CreateMaterialBuffer();

	// 定数バッファの作成
	HRESULT CreateWhiteBuffer();
	// ---その他関数--- //

	// シェーダーのよみこみを行う関数
	HRESULT LoadShader();
			
	// モデルデータの読み込みを行う関数
	HRESULT LoadPMD();

	// テクスチャのパスを作り出す関数
	std::string GetTexPath(const std::string& modelPath, const char* texPath);
	
	// マルチバイト文字列からワイド文字列への変換を行う関数
	std::wstring GetWstringFromString(const std::string& str);

	// ファイルを読み込んでテクスチャバッファを作成する関数
	ID3D12Resource*	LoadTextureFromFile(std::string& texPath);

	// 拡張子を取得する関数
	std::string GetExtension(const std::string& path);

	// パスを分割する関数
	std::pair<std::string, std::string>SplitFileName(const std::string&path,const char splitter='*');

	// バッファに画を書き終わるまでウェイトをかける関数
	void WaitWithFence();

	// コマンドを処理し終わったかどうかを返す関数
	void ExecuteCommand();

	//　---以下変数--- //
	WVPMatrix* _wvpMP;
	WVPMatrix _wvp;

	std::vector<VertexInfo> _vivec;
	std::vector<unsigned short> verindex;
	std::vector<PMDMaterial> _materials;

	PMDColor* MapColor = nullptr;

	float angle;// 角度
public:
	Dx12Wrapper(HWND hwnd);
	~Dx12Wrapper();

	// 初期化
	int Init();

	// 更新
	void UpDate();
};


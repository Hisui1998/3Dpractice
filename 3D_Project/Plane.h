#pragma once
#include <d3d12.h>
#include <DirectXMath.h>

struct PlaneVertex
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT2 normal;
	DirectX::XMFLOAT2 uv;
};

class Plane
{
private:
	ID3D12Device* _dev;

	// パイプラインの生成
	bool CreatePipeline();

	// ルートシグネチャの生成
	bool CreateRootSignature();

	// 頂点バッファの作成
	bool CreateVertexBuffer();

	// 頂点バッファの作成
	bool CreateIndexBuffer();


	ID3D12PipelineState* _planeGPS;
	ID3D12RootSignature* _planeRS;

	ID3DBlob* _planeVertexShader = nullptr;// 頂点シェーダ
	ID3DBlob* _planePixelShader = nullptr;// ピクセルシェーダ

	D3D12_VIEWPORT _viewPort;// ビューポート
	D3D12_RECT _scissor;// シザー範囲

	// 頂点バッファ
	ID3D12Resource* _vertexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW _vbView = {};// 頂点バッファビュー

	// インデックス
	ID3D12Resource* _indexBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW _idView = {};// インデックスバッファビュー
public:
	Plane(ID3D12Device* _dev, D3D12_VIEWPORT _view, D3D12_RECT _scissor);
	~Plane();

	void Draw(ID3D12GraphicsCommandList* list, ID3D12DescriptorHeap* wvp, ID3D12DescriptorHeap*shadow);
};


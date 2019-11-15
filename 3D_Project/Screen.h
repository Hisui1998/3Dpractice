#pragma once
#include <d3d12.h>
#include <DirectXMath.h>

// 頂点情報
struct Vertex
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT2 uv;
};

class Polygon
{
private:
	ID3D12Device* _dev;

	// バッファの作成
	HRESULT CreateBuffer();
	// ヒープとビューの作成
	HRESULT CreateHeapAndView();
	// ペラポリ用
	ID3D12Resource* _peraBuffer = nullptr;// ペラポリ本体のバッファ

	ID3D12Resource* _peraVertBuff = nullptr;// ペラポリ用頂点バッファ
	D3D12_VERTEX_BUFFER_VIEW _peravbView = {};// ペラポリ用頂点バッファビュー

	ID3D12DescriptorHeap* _rtvDescHeap = nullptr;// ペラポリ用レンダーターゲットデスクリプタヒープ
	ID3D12DescriptorHeap* _srvDescHeap = nullptr;// ペラポリ用シェーダーリソースデスクリプタヒープ

	ID3DBlob* peraVertShader = nullptr;// ペラポリ用頂点シェーダ
	ID3DBlob* peraPixShader = nullptr;// ペラポリ用ピクセルシェーダ

	ID3D12PipelineState* _peraPipeline = nullptr;// ペラポリ用パイプライン
	ID3D12RootSignature* _peraSignature = nullptr;// ペラポリ用ルートシグネチャ
public:
	Polygon(ID3D12Device*dev);
	~Polygon();

};


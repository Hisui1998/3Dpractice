#pragma once
#include <d3d12.h>
#include <DirectXMath.h>


// プリミティヴオブジェクトの頂点情報の構造体
struct PrimitiveVertex {
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 uv;

/// 初期化用コンストラクタ
	// 何も入れないと0初期化
	PrimitiveVertex() {
		pos = DirectX::XMFLOAT3(0, 0, 0);
		normal = DirectX::XMFLOAT3(0, 0, 0);
		uv = DirectX::XMFLOAT2(0, 0);
	}

	// 座標情報と法線情報をとUV情報を
	PrimitiveVertex(DirectX::XMFLOAT3& p, DirectX::XMFLOAT3& norm, DirectX::XMFLOAT2& coord) {
		pos = p;
		normal = norm;
		uv = coord;
	}

	// 全部一個ずつ初期化するとき用
	PrimitiveVertex(float x, float y, float z, float nx, float ny, float nz, float u, float v) {
		pos.x = x;
		pos.y = y;
		pos.z = z;
		normal.x = nx;
		normal.y = ny;
		normal.z = nz;
		uv.x = u;
		uv.y = v;
	}
};

class PrimitiveMesh
{
protected:
	ID3D12Resource* _vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW _vertexView = {};
public:
	PrimitiveMesh();
	virtual ~PrimitiveMesh();
	virtual void Draw() = 0;
};


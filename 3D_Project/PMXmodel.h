#pragma once
#include <string>
#include <vector>
#include <map>
#include "Dx12Wrapper.h"

using namespace DirectX;

#pragma pack(1)
// PMXファイルのヘッダ情報
struct PMXHeader {
	unsigned char extension[4];//"PMX":4
	float version;// ver(2.0/2.1):4
	unsigned char bytesize;// 後続するデータ列のバイトサイズ  *PMX2.0は 8 で固定
	unsigned char data[8];// エンコード方式とかが入っているバイト文字列
};

// PMXモデル情報
struct PMXVertexInfo {
	XMFLOAT3 pos;
	XMFLOAT3 nomal;
	XMFLOAT2 uv;
	XMFLOAT4 adduv[4];
	unsigned char weight;
	int boneIdxSize[4];
	float boneweight[4];
	XMFLOAT3 SDEFdata[3];
	float edge;
};

struct PMXMaterial {
	XMFLOAT4 Diffuse;//16
	XMFLOAT3 Specular;//12
	float SpecularPow;//4
	XMFLOAT3 Ambient;//12
	unsigned char bitFlag;//1
	XMFLOAT4 edgeColor;//16
	float edgeSize;//4

	int texIndex;
	int sphereIndex;
	unsigned char sphereMode;
	unsigned char toonFlag;
	int toonIndex;
	// コメントがあるからFseekすること
	int faceVerCnt;
};
#pragma pack()

// 色情報の構造体
struct PMXColor
{
	DirectX::XMFLOAT4 diffuse;
	DirectX::XMFLOAT4 specular;
	DirectX::XMFLOAT3 ambient;
};

class PMXmodel
{
private:
	void LoadModel(ID3D12Device* _dev, const std::string modelPath);

	void CreateBoneTree();

	// 白テクスチャの作成
	HRESULT CreateWhiteTexture(ID3D12Device* _dev);

	// 黒テクスチャの作成
	HRESULT CreateBlackTexture(ID3D12Device* _dev);

	HRESULT CreateMaterialBuffer(ID3D12Device* _dev);

	HRESULT CreateBoneBuffer(ID3D12Device* _dev);

	std::wstring GetWstringFromString(const std::string& str);

	ID3D12Resource* LoadTextureFromFile(std::string & texPath, ID3D12Device* _dev);

	HRESULT CreateGrayGradationTexture(ID3D12Device* _dev);

	std::vector<PMXVertexInfo> vertexInfo;
	std::vector<unsigned int> _verindex;
	std::vector<std::string>_texVec;
	std::vector<PMXMaterial> _materials;

	// 白黒テクスチャ
	ID3D12Resource* whiteTex = nullptr;
	ID3D12Resource* blackTex = nullptr;
	ID3D12Resource* gradTex = nullptr;

	ID3D12Resource* _boneBuffer;// ボーンバッファ
	std::vector<ID3D12Resource*> _textureBuffer;// テクスチャバッファ
	std::vector<ID3D12Resource*> _sphBuffer;// 乗算スフィアマップ
	std::vector<ID3D12Resource*> _spaBuffer;// 加算スフィアマップ
	std::vector<ID3D12Resource*> _toonResources;// トゥーン
	std::vector<ID3D12Resource*> _materialsBuff;// マテリアルバッファ(複数あるのでベクターにしている)

	std::vector<DirectX::XMMATRIX>_boneMats;
	DirectX::XMMATRIX* _mappedBones;

	ID3D12DescriptorHeap* _boneHeap;// ボーンヒープ
	ID3D12DescriptorHeap* _matDescHeap;// マテリアルデスクリプタヒープ

	PMXColor* MapColor = nullptr;
	std::string FolderPath;
public:
	PMXmodel(ID3D12Device* _dev, const std::string modelPath);
	~PMXmodel();

	void UpDate();
	std::vector<PMXMaterial> GetMaterials() {
		return _materials;
	};
	std::vector<unsigned int> GetVertexIndex() {
		return _verindex;
	};
	std::vector<PMXVertexInfo> GetVertexInfo() {
		return vertexInfo;
	};

	ID3D12DescriptorHeap*& GetBoneHeap() { return _boneHeap; };
	ID3D12DescriptorHeap*& GetMaterialHeap() { return _matDescHeap; };
};


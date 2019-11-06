#pragma once
#include <string>
#include "Dx12Wrapper.h"

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
struct PMDVertexInfo
{
	float pos[3];//12
	float normal_vec[3];//12
	float uv[2];//8
	unsigned short bone_num[2];//4 // ボーン番号 1、番号 2
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
	unsigned int faceVerCnt;// 4
	char texFileName[20];//20
};

// PMDモデルのボーン情報構造体
struct PMDBoneInfo {
	char bone_name[20];// ボーン名:20
	unsigned short parent_bone_index;// 親のボーン番号(ないときは0xffff):2
	unsigned short tail_pos_bone_index;// よくわからんけど多分末尾かどうかみたいな：2
	char bone_type; // ボーンの種類:1
	unsigned short ik_parent_bone_index; // IKボーン番号(影響IKボーン。ない場合は0):2
	DirectX::XMFLOAT3 bone_head_pos; // ボーンのヘッドの座標:12
};
#pragma pack()
/// パディングを元に戻す()４バイト

// ボーンのノード情報
struct BoneNode {
	int boneIdx;// ボーンの行列配列と対応
	DirectX::XMFLOAT3 startPos;// ボーン始点
	DirectX::XMFLOAT3 endPos;// ボーン終点
	std::vector<BoneNode*>children;// 子供たちへのリンク
};

// 色情報の構造体
struct PMDColor
{
	DirectX::XMFLOAT4 diffuse;
	DirectX::XMFLOAT4 specular;
	DirectX::XMFLOAT3 ambient;
};


class PMDmodel
{
private:
	/* プライベート関数 */
	void LoadModel(ID3D12Device* _dev,const std::string modelPath);
	
	// 頂点バッファの作成
	HRESULT CreateVertexBuffer(ID3D12Device* _dev);

	// インデックスバッファの作成
	HRESULT CreateIndexBuffer(ID3D12Device* _dev);

	// 拡張子を取得する関数
	std::string GetExtension(const std::string& path);

	// テクスチャのパスを作り出す関数
	std::string GetTexPath(const std::string& modelPath, const char* texPath);

	// パスを分割する関数
	std::pair<std::string, std::string>SplitFileName(const std::string&path, const char splitter = '*');

	// ファイルを読み込んでテクスチャバッファを作成する関数
	ID3D12Resource*	LoadTextureFromFile(std::string& texPath,ID3D12Device* _dev);

	// マルチバイト文字列からワイド文字列への変換を行う関数
	std::wstring GetWstringFromString(const std::string& str);

	// ボーンツリー作成
	void CreateBoneTree();

	// マテリアルバッファとマテリアルバッファビューを作る関数
	HRESULT CreateMaterialBuffer(ID3D12Device* _dev);

	// ボーンバッファの作成
	HRESULT CreateBoneBuffer(ID3D12Device* _dev);

	// 白テクスチャの作成
	HRESULT CreateWhiteTexture(ID3D12Device* _dev);

	// 黒テクスチャの作成
	HRESULT CreateBlackTexture(ID3D12Device* _dev);

	// トゥーンのためのグラデーションテクスチャの作成
	HRESULT	CreateGrayGradationTexture(ID3D12Device* _dev);

	// 並行移動して回転
	void RotationMatrix(std::string bonename, DirectX::XMFLOAT3 theta);

	// 子のノードまで再帰的に行列乗算する関数
	void RecursiveMatrixMultiply(BoneNode& node, DirectX::XMMATRIX& MultiMat);

	/* プライベート変数 */
	ID3D12Resource* _boneBuffer;// ボーンバッファ
	std::vector<ID3D12Resource*> _textureBuffer;// テクスチャバッファ
	std::vector<ID3D12Resource*> _sphBuffer;// 乗算スフィアマップ
	std::vector<ID3D12Resource*> _spaBuffer;// 加算スフィアマップ
	std::vector<ID3D12Resource*> _toonResources;// トゥーン
	std::vector<ID3D12Resource*> _materialsBuff;// マテリアルバッファ(複数あるのでベクターにしている)

	ID3D12DescriptorHeap* _boneHeap;// ボーンヒープ
	ID3D12DescriptorHeap* _matDescHeap;// マテリアルデスクリプタヒープ

	std::vector<PMDVertexInfo> _vivec;// 頂点情報を格納している配列
	std::vector<unsigned short> _verindex;// 頂点番号が格納されている配列
	std::vector<PMDMaterial> _materials;// マテリアル

	// ボーン関係の変数
	std::vector<PMDBoneInfo> _bones;
	std::vector<DirectX::XMMATRIX>_boneMats;
	std::map<std::string, BoneNode>_boneMap;
	DirectX::XMMATRIX* _mappedBones;

	// テクスチャの名前とバッファが入っている配列
	std::map<std::string, ID3D12Resource*>_resourceTable;

	// 白黒テクスチャ
	ID3D12Resource* whiteTex = nullptr;
	ID3D12Resource* blackTex = nullptr;
	ID3D12Resource* gradTex = nullptr;
	
	// 頂点バッファ
	ID3D12Resource* _vertexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW _vbView = {};// 頂点バッファビュー

	// インデックスバッファ
	ID3D12Resource* _indexBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW _idxView = {};// インデックスバッファビュー

	PMDColor* MapColor = nullptr;
	float angle;
public:
	PMDmodel(ID3D12Device* _dev, const std::string modelPath);
	~PMDmodel();
	void UpDate();
	std::vector<PMDMaterial> GetMaterials();

	const std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputLayout();

	D3D12_VERTEX_BUFFER_VIEW GetVertexView() { return _vbView; };
	D3D12_INDEX_BUFFER_VIEW GetIndexView() { return _idxView; };
	const LPCWSTR GetUseShader();

	ID3D12DescriptorHeap*& GetBoneHeap();
	ID3D12DescriptorHeap*& GetMaterialHeap();
};

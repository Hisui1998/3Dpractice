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
	XMFLOAT3 normal;
	XMFLOAT2 uv;

	XMFLOAT4 adduv[4];

	unsigned char weight;

	XMINT4 boneIdx;
	XMFLOAT4 boneweight;

	XMFLOAT3 SDEFdata[3];
	float edge;
};

// 以下もーふの種別毎の構造体
struct MorphOffsets{
	struct VertexMorph
	{
		int verIdx;
		XMFLOAT3 pos;
	}vertexMorph;

	struct UVMorph
	{
		int verIdx;
		XMFLOAT4 uvOffset;//※通常UVはz,wが不要項目になるがモーフとしてのデータ値は記録しておく
	}uvMorph;

	struct BoneMorph {
		int boneIdx;
		XMFLOAT3 moveVal;
		XMFLOAT4 rotation;
	}boneMorph;

	struct MaterialMorph
	{
		int materialIdx;
		char type;
		// 以下　乗算：1.0/加算：0.0が初期値となる
		XMFLOAT4 diffuse;
		XMFLOAT3 specular;
		float specularPow;
		XMFLOAT3 ambient;
		XMFLOAT4 edgeColor;
		float edgeSize;
		XMFLOAT4 texPow;
		XMFLOAT4 sphTexPow;
		XMFLOAT4 toonTexPow;
	}materialMorph;

	struct GroupMorph {
		int MorphIdx;
		float MorphPar;// モーフ率 : グループモーフのモーフ値 * モーフ率 = 対象モーフのモーフ値
	}groupMorph;
};

// PMXもーふ情報
struct MorphHeader {
	unsigned char category;// どこを動かすかー1:眉(左下) 2:目(左上) 3:口(右上) 4:その他(右下)  | 0:システム予約
	unsigned char type;// もーふの種類ー0:グループ, 1:頂点, 2:ボーン, 3:UV, 4:追加UV1, 5:追加UV2, 6:追加UV3, 7:追加UV4, 8:マテリアル
	int dataNum;// 後続の要素数
};

// PMXマテリアル情報
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

// ボーン情報
struct BoneInfo {
	std::wstring name;
	XMFLOAT3 pos;// 絶対座標
	int parentboneIndex;// 親のボーンIndex

	int tranceLevel;// 変形階層

	unsigned short bitFlag;// ボーンフラグ(こっから継続データが変化)

	XMFLOAT3 offset;// 座標ｵﾌｾｯﾄ
	int boneIndex;// 接続先ボーン

	int grantIndex;// 付与ボーンのボーンインデックス
	float grantPar;// 付与率

	XMFLOAT3 axisvector;// 軸の方向ベクトル

	XMFLOAT3 axisXvector;// X軸の方向ベクトル
	XMFLOAT3 axisZvector;// Z軸の方向ベクトル 

	int key;// Key値

	// IK情報
	struct IKdata {
		int boneIdx;
		int loopCnt;// IKループ回数
		float limrad;// IKループ計算時の制限角度
		int linkNum;// 後続の要素数

		int linkboneIdx;// リンク先のボーンインデックス
		char isRadlim;// 角度制限を付けるかどうか

		XMFLOAT3 minRadlim;// 角度制限下限
		XMFLOAT3 maxRadlim;// 角度制限上限
	}IkData;
};
#pragma pack()

// ボーンのノード情報
struct PMXBoneNode {
	int boneIdx;
	XMFLOAT3 startPos;
	XMFLOAT3 endPos;
	std::vector<PMXBoneNode*> children;
};

// 色情報の構造体(正直いらんかもしれん)
struct PMXColor
{
	XMFLOAT4 diffuse;
	XMFLOAT4 specular;
	XMFLOAT3 ambient;
};

// PMXモデルのクラス
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

	void CreateBoneOrder(PMXBoneNode& node,int level);

	void RotationMatrix(std::wstring bonename, XMFLOAT3 theta);

	void RecursiveMatrixMultiply(PMXBoneNode& node, XMMATRIX& MultiMat);

	PMXHeader header;// ヘッダー情報が入ってるよ


	std::map<std::wstring, MorphHeader> _morphHeaders;// もーふの名前からもーふのヘッダー情報をとってくる
	std::map<std::wstring, std::vector<MorphOffsets>> _morphData;// もーふの名前からもーふ情報をとってくる

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


	std::vector<int>_orderMoveIdx;// 動かす順番にボーンインデックスを入れる（仮）
	std::vector<BoneInfo>_bones;
	std::vector<BoneInfo>_boneOrders[4];
	std::map<std::wstring, PMXBoneNode> _boneMap;//ボーンマップ
	std::vector<XMMATRIX>_boneMats;// 転送用
	XMMATRIX* _mappedBones = nullptr;

	float angle = 0;

	ID3D12DescriptorHeap* _boneHeap;// ボーンヒープ
	ID3D12DescriptorHeap* _matDescHeap;// マテリアルデスクリプタヒープ

	PMXColor* MapColor = nullptr;
	std::string FolderPath;
public:
	PMXmodel(ID3D12Device* _dev, const std::string modelPath);
	~PMXmodel();

	void UpDate(char key[256]);
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

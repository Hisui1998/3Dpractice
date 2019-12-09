#pragma once
#include <string>
#include <vector>
#include <map>
#include "Dx12Wrapper.h"

class VMDMotion;

using namespace DirectX;

// PMXモデル情報
struct PMXVertexInfo {
	XMFLOAT3 pos;
	XMFLOAT3 normal;
	XMFLOAT2 uv;

	XMFLOAT4 adduv[4];

	unsigned char weightType;

	int boneIdx[4];
	float boneweight[4];

	XMFLOAT3 SDEFdata[3];
	float edge;
};

// IKリンク情報
struct IKLink
{
	int linkboneIdx;// リンク先のボーンインデックス
	char isRadlim;// 角度制限を付けるかどうか

	XMFLOAT3 minRadlim;// 角度制限下限
	XMFLOAT3 maxRadlim;// 角度制限上限
};

// IK情報
struct IKdata {
	int boneIdx;// ターゲットボーンのインデックス
	int loopCnt;// IKループ回数
	float limrad;// IKループ計算時の制限角度
	int linkNum;// 後続の要素数

	std::vector<IKLink> ikLinks;// 後続の要素配列
};


// 戦犯
#pragma pack(1)
// PMXファイルのヘッダ情報
struct PMXHeader {
	unsigned char extension[4];//"PMX":4
	float version;// ver(2.0/2.1):4
	unsigned char bytesize;// 後続するデータ列のバイトサイズ  *PMX2.0は 8 で固定
	unsigned char data[8];// エンコード方式とかが入っているバイト文字列
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
	std::wstring name;// ボーン名
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

	IKdata ikdata;// IKのデータ
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
	ID3D12Device* _dev;

	void LoadModel(const std::string modelPath,const std::string vmdPath);

	void BufferUpDate();

	void MotionUpDate(int frameno);

	void MorphUpDate(int frameno);

	void CreateBoneTree();

	void SolveIK(BoneInfo&b);

	void SolveCCDIK(BoneInfo& ik);

	void SolveCosineIK(BoneInfo& ik);

	void SolveLookAt(BoneInfo& ik);

	// Z軸を特定の方向に向かせる
	XMMATRIX LookAtMatrix(const XMVECTOR& lookat, XMFLOAT3& up, XMFLOAT3& right);

	// 任意の軸を任意の方向へ向かせる
	XMMATRIX LookAtMatrix(const XMVECTOR origin, const XMVECTOR lookat, XMFLOAT3 up, XMFLOAT3 right);
	
	// パイプラインの生成
	HRESULT CreatePipeline();

	// ルートシグネチャの生成
	HRESULT CreateRootSignature();

	// 白テクスチャの作成
	HRESULT CreateWhiteTexture();

	// 黒テクスチャの作成
	HRESULT CreateBlackTexture();
	
	HRESULT CreateVertexBuffer();

	HRESULT CreateIndexBuffer();

	HRESULT CreateMaterialBuffer();

	HRESULT CreateBoneBuffer();

	std::wstring GetWstringFromString(const std::string& str);

	ID3D12Resource* LoadTextureFromFile(std::string & texPath);

	HRESULT CreateGrayGradationTexture();

	void RotationMatrix(const std::wstring bonename, const XMFLOAT4 &quat1);
	void RotationMatrix(const std::wstring bonename, const XMFLOAT4 &quat1, const XMFLOAT4 &quat2, float pow = 0.0f);

	void RecursiveMatrixMultiply(PMXBoneNode& node, XMMATRIX& MultiMat);

	float GetBezierPower(float x, const XMFLOAT2& a, const XMFLOAT2& b, uint8_t n);

	/* シャドウマップ用 */
	// シャドウマップ用ルートシグネチャの作成
	HRESULT CreateShadowRS();

	// シャドウマップ用パイプライン生成
	HRESULT CreateShadowPS();

	ID3DBlob* _shadowVertShader = nullptr;// シャドウマップ用頂点シェーダ
	ID3DBlob* _shadowPixShader = nullptr;// シャドウマップ用ピクセルシェーダ

	ID3D12PipelineState* _shadowMapGPS;
	ID3D12RootSignature* _shadowMapRS;
	/// シャドウマップここまで

	PMXHeader header;// ヘッダー情報が入ってるよ

	ID3D12PipelineState* _pmxPipeline=nullptr;// PMX描画用パイプライン
	ID3D12RootSignature* _pmxSignature =nullptr;// PMX描画用ルートシグネチャ

	D3D12_VIEWPORT _viewPort;// ビューポート
	D3D12_RECT _scissor;// シザー範囲

	std::shared_ptr<VMDMotion>_vmdData;// VMDのデータを格納しているポインタ

	std::map<std::wstring, MorphHeader> _morphHeaders;// モーフ名からモーフのカテゴリ等のヘッダ情報が取得できる(需要低め)
	std::map<std::wstring, std::vector<MorphOffsets>> _morphData;// モーフ名からモーフ情報が取得できる(ヘッダ情報からモーフ種類を取得して使う)

	std::vector<PMXVertexInfo> vertexInfo;// 頂点情報が入っている配列
	std::vector<PMXVertexInfo> firstVertexInfo;// 頂点情報の初期情報が入っている配列
	std::vector<unsigned int> _verindex;// 頂点インデックス情報が入っている
	std::vector<std::string>_texturePaths;// テクスチャのパスを格納している配列
	std::vector<PMXMaterial> _materials;// マテリアル情報が入っている配列
	PMXColor* MapColor = nullptr;// マテリアルのカラー情報を入れて転送する用のポインタ(中身確認用かも)

	// 頂点バッファ
	ID3D12Resource* _vertexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW _vbView = {};// 頂点バッファビュー

	// インデックスバッファ
	ID3D12Resource* _indexBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW _idxView = {};// インデックスバッファビュー

	// 白黒テクスチャ
	ID3D12Resource* whiteTex = nullptr;
	ID3D12Resource* blackTex = nullptr;
	ID3D12Resource* gradTex = nullptr;

	// テクスチャ系バッファ配列
	std::vector<ID3D12Resource*> _textureBuffer;// テクスチャバッファ配列
	static std::map<std::string, ID3D12Resource*>_texMap;// テクスチャネームからバッファを取り出す変数
	std::vector<ID3D12Resource*> _sphBuffer;// 乗算スフィアマップバッファ配列
	std::vector<ID3D12Resource*> _spaBuffer;// 加算スフィアマップバッファ配列
	std::vector<ID3D12Resource*> _toonResources;// トゥーンバッファ配列

	// マテリアル
	std::vector<ID3D12Resource*> _materialsBuff;// マテリアルバッファ
	ID3D12DescriptorHeap* _matDescHeap;// マテリアルデスクリプタヒープ

	// ボーン系の変数
	ID3D12Resource* _boneBuffer;// ボーンバッファ
	std::vector<BoneInfo>_bones;// ボーンの基本情報が入ってる
	std::map<std::wstring, PMXBoneNode> _boneTree;//ボーン名から子のノードを取得できる
	std::vector<XMMATRIX>_boneMats;// ボーン行列(中身はボーンインデックス順)
	XMMATRIX* _sendBone = nullptr;// ボーン行列の転送用ポインタ

	//std::map<IKdata>

	ID3D12DescriptorHeap* _boneHeap;// ボーンヒープ

	std::string FolderPath;// モデルが入っているフォルダまでのパス

	float _morphWeight;// もーふのウェイト(テスト用)
	int frame = 0;
	std::string modelname[4];
public:
	PMXmodel(ID3D12Device* dev, const std::string modelPath,const std::string vmdPath = "");
	~PMXmodel();

	void UpDate(char key[256]);

	// 描画関数(リストと深度バッファヒープ位置とWVP定数バッファ)
	void Draw(ID3D12GraphicsCommandList* list,ID3D12DescriptorHeap* constant,ID3D12DescriptorHeap*shadow,unsigned int instanceNum=1);

	// シャドウ深度の描画
	void PreDrawShadow(ID3D12GraphicsCommandList* list, ID3D12DescriptorHeap* wvp, unsigned int instanceNum = 1);

	std::vector<PMXMaterial> GetMaterials() { return _materials; };

	const std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputLayout();
	const XMFLOAT3 GetPos();

	const std::string GetModelName() { return modelname[0]; };
	D3D12_VERTEX_BUFFER_VIEW GetVertexView() { return _vbView; };
	D3D12_INDEX_BUFFER_VIEW GetIndexView() { return _idxView; };
	const LPCWSTR GetUseShader();
	ID3D12DescriptorHeap*& GetBoneHeap() { return _boneHeap; };
	ID3D12DescriptorHeap*& GetMaterialHeap() { return _matDescHeap; };
};

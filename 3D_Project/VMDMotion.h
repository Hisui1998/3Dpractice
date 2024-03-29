#pragma once
#include <DirectXMath.h>
#include <string>
#include <map>
#include <vector>
#include <string>

// ヘッダー情報
struct VMDHeader {
	char VmdHeader[30];
	char VmdModelName[20];
};

#pragma pack(1)
// モーション情報
struct MotionInfo {
	char BoneName[15]; // ボーン名
	unsigned int FrameNo; // フレーム番号
	DirectX::XMFLOAT3 Location; // 位置
	DirectX::XMFLOAT4 Quaternion;// 回転
	unsigned char Interpolation[64];// 補完
};

// 表情情報
struct MorphData { // 23 Bytes // 表情
	char SkinName[15]; // 表情名
	unsigned int FrameNo; // フレーム番号
	float Weight; // 表情の設定値(表情スライダーの値)
};
#pragma pack()

// 表情情報
struct MorphInfo {
	std::string SkinName; // 表情名
	unsigned int FrameNo; // フレーム番号
	float Weight; // 表情の設定値(表情スライダーの値)
	MorphInfo(std::string sn, unsigned int fn, float w) :SkinName(sn), FrameNo(fn), Weight(w){};
};


struct MotionData
{
	std::string BoneName; // ボーン名
	unsigned int FrameNo; // フレーム番号
	DirectX::XMFLOAT3 Location; // 位置
	DirectX::XMFLOAT4 Quaternion;// 回転
	DirectX::XMFLOAT2 p1, p2;//ベジェ用
};

// カメラデータ
struct CameraData{ 
	unsigned int FlameNo; // フレーム
	float Length; // -(距離)
	float Location[3]; // 位置
	float Rotation[3]; // オイラー角  X軸は符号が反転しているので注意 
	unsigned char Interpolation[24];// 補完
	unsigned int ViewingAngle; // 視界角
	unsigned char Perspective;// パース
};

// 照明データ
struct LightData {
	unsigned int FlameNo; // フレーム
	float RGB[3]; // RGB
	float Location[3]; // 座標
};

// セルフシャドウデータ
struct SelfShadowData {
	unsigned int FlameNo; // フレーム
	unsigned char Mode; // モード
	float Distance;// 距離
};

class VMDMotion
{
private:
	void LoadVMD(std::string fileName);

	std::map<std::string, std::vector<MotionData>> _motionData;// モーションデータ
	// フレームナンバーが同じモーフは合成して使う
	std::map<int, std::vector<MorphInfo>>  _morphData;// 表情データ

	static int flame;
	int totalframe;
	const int _waitFlame;// ループした後ウェイトをかけるフレーム数

	unsigned int MotionCount;// モーション数
	unsigned int MorphCount;// 表情数
	unsigned int CameraCount;// カメラ数
	unsigned int LightCount;// ライト数
	unsigned int ShadowCount;// シャドウ数
public:
	VMDMotion(std::string fileName, int waitFlame = -1);
	~VMDMotion();

	std::map<std::string, std::vector<MotionData>>GetMotionData();
	std::map<int, std::vector<MorphInfo>>GetMorphData();
	int Duration();
	const int GetTotalFrame();
};


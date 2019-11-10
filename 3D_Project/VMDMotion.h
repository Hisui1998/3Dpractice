#pragma once
#include <DirectXMath.h>
#include <string>
#include <map>
#include <vector>

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
	DirectX::XMFLOAT4 Rotatation;// 回転
	unsigned char Interpolation[64];// 補完
};


// 表情情報
struct MorphInfo { // 23 Bytes // 表情
	char SkinName[15]; // 表情名
	unsigned int FrameNo; // フレーム番号
	float Weight; // 表情の設定値(表情スライダーの値)
};
#pragma pack()


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

	std::map<std::string, std::vector<MotionInfo>> _motionData;// モーションデータ
	std::map<std::string, std::vector<MorphInfo>> _morphData;// 表情データ

	int flame;

	unsigned int MotionCount;// モーション数
	unsigned int MorphCount;// 表情数
	unsigned int CameraCount;// カメラ数
	unsigned int LightCount;// ライト数
	unsigned int ShadowCount;// シャドウ数
public:
	VMDMotion(std::string fileName);
	~VMDMotion();

	std::map<std::string, std::vector<MotionInfo>>GetMotionData();
	std::map<std::string, std::vector<MorphInfo>>GetMorphData();
	int Duration() { return ++flame; };
};


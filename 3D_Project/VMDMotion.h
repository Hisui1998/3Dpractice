#pragma once
#include <DirectXMath.h>
#include <string>
#include <map>
#include <vector>
#include <string>

// �w�b�_�[���
struct VMDHeader {
	char VmdHeader[30];
	char VmdModelName[20];
};

#pragma pack(1)
// ���[�V�������
struct MotionInfo {
	char BoneName[15]; // �{�[����
	unsigned int FrameNo; // �t���[���ԍ�
	DirectX::XMFLOAT3 Location; // �ʒu
	DirectX::XMFLOAT4 Quaternion;// ��]
	unsigned char Interpolation[64];// �⊮
};


// �\����
struct MorphInfo { // 23 Bytes // �\��
	char SkinName[15]; // �\�
	unsigned int FrameNo; // �t���[���ԍ�
	float Weight; // �\��̐ݒ�l(�\��X���C�_�[�̒l)
};
#pragma pack()

struct MotionData
{
	std::string BoneName; // �{�[����
	unsigned int FrameNo; // �t���[���ԍ�
	DirectX::XMFLOAT3 Location; // �ʒu
	DirectX::XMFLOAT4 Quaternion;// ��]
	DirectX::XMFLOAT2 p1, p2;//�x�W�F�p
};

// �J�����f�[�^
struct CameraData{ 
	unsigned int FlameNo; // �t���[��
	float Length; // -(����)
	float Location[3]; // �ʒu
	float Rotation[3]; // �I�C���[�p  X���͕��������]���Ă���̂Œ��� 
	unsigned char Interpolation[24];// �⊮
	unsigned int ViewingAngle; // ���E�p
	unsigned char Perspective;// �p�[�X
};

// �Ɩ��f�[�^
struct LightData {
	unsigned int FlameNo; // �t���[��
	float RGB[3]; // RGB
	float Location[3]; // ���W
};

// �Z���t�V���h�E�f�[�^
struct SelfShadowData {
	unsigned int FlameNo; // �t���[��
	unsigned char Mode; // ���[�h
	float Distance;// ����
};

class VMDMotion
{
private:
	void LoadVMD(std::string fileName);

	std::map<std::string, std::vector<MotionData>> _motionData;// ���[�V�����f�[�^
	// �t���[���i���o�[���������[�t�͍������Ďg��
	std::map<int, std::vector<MorphInfo>>  _morphData;// �\��f�[�^

	static int flame;
	int totalframe;
	const int _waitFlame;// ���[�v������E�F�C�g��������t���[����

	unsigned int MotionCount;// ���[�V������
	unsigned int MorphCount;// �\�
	unsigned int CameraCount;// �J������
	unsigned int LightCount;// ���C�g��
	unsigned int ShadowCount;// �V���h�E��
public:
	VMDMotion(std::string fileName, int waitFlame = -1);
	~VMDMotion();

	std::map<std::string, std::vector<MotionData>>GetMotionData();
	std::map<int, std::vector<MorphInfo>>GetMorphData();
	int Duration();
	const int GetTotalFrame();
};


#pragma once
#include <string>
#include <vector>
#include <map>
#include "Dx12Wrapper.h"

using namespace DirectX;

#pragma pack(1)
// PMX�t�@�C���̃w�b�_���
struct PMXHeader {
	unsigned char extension[4];//"PMX":4
	float version;// ver(2.0/2.1):4
	unsigned char bytesize;// �㑱����f�[�^��̃o�C�g�T�C�Y  *PMX2.0�� 8 �ŌŒ�
	unsigned char data[8];// �G���R�[�h�����Ƃ��������Ă���o�C�g������
};

// PMX���f�����
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

// �ȉ����[�ӂ̎�ʖ��̍\����
struct MorphOffsets{
	struct VertexMorph
	{
		int verIdx;
		XMFLOAT3 pos;
	}vertexMorph;

	struct UVMorph
	{
		int verIdx;
		XMFLOAT4 uvOffset;//���ʏ�UV��z,w���s�v���ڂɂȂ邪���[�t�Ƃ��Ẵf�[�^�l�͋L�^���Ă���
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
		// �ȉ��@��Z�F1.0/���Z�F0.0�������l�ƂȂ�
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
		float MorphPar;// ���[�t�� : �O���[�v���[�t�̃��[�t�l * ���[�t�� = �Ώۃ��[�t�̃��[�t�l
	}groupMorph;
};

// PMX���[�ӏ��
struct MorphHeader {
	unsigned char category;// �ǂ��𓮂������[1:��(����) 2:��(����) 3:��(�E��) 4:���̑�(�E��)  | 0:�V�X�e���\��
	unsigned char type;// ���[�ӂ̎�ށ[0:�O���[�v, 1:���_, 2:�{�[��, 3:UV, 4:�ǉ�UV1, 5:�ǉ�UV2, 6:�ǉ�UV3, 7:�ǉ�UV4, 8:�}�e���A��
	int dataNum;// �㑱�̗v�f��
};

// PMX�}�e���A�����
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
	// �R�����g�����邩��Fseek���邱��
	int faceVerCnt;
};

// �{�[�����
struct BoneInfo {
	std::wstring name;
	XMFLOAT3 pos;// ��΍��W
	int parentboneIndex;// �e�̃{�[��Index

	int tranceLevel;// �ό`�K�w

	unsigned short bitFlag;// �{�[���t���O(��������p���f�[�^���ω�)

	XMFLOAT3 offset;// ���W�̾��
	int boneIndex;// �ڑ���{�[��

	int grantIndex;// �t�^�{�[���̃{�[���C���f�b�N�X
	float grantPar;// �t�^��

	XMFLOAT3 axisvector;// ���̕����x�N�g��

	XMFLOAT3 axisXvector;// X���̕����x�N�g��
	XMFLOAT3 axisZvector;// Z���̕����x�N�g�� 

	int key;// Key�l

	// IK���
	struct IKdata {
		int boneIdx;
		int loopCnt;// IK���[�v��
		float limrad;// IK���[�v�v�Z���̐����p�x
		int linkNum;// �㑱�̗v�f��

		int linkboneIdx;// �����N��̃{�[���C���f�b�N�X
		char isRadlim;// �p�x������t���邩�ǂ���

		XMFLOAT3 minRadlim;// �p�x��������
		XMFLOAT3 maxRadlim;// �p�x�������
	}IkData;
};
#pragma pack()

// �{�[���̃m�[�h���
struct PMXBoneNode {
	int boneIdx;
	XMFLOAT3 startPos;
	XMFLOAT3 endPos;
	std::vector<PMXBoneNode*> children;
};

// �F���̍\����(��������񂩂������)
struct PMXColor
{
	XMFLOAT4 diffuse;
	XMFLOAT4 specular;
	XMFLOAT3 ambient;
};

// PMX���f���̃N���X
class PMXmodel
{
private:
	void LoadModel(ID3D12Device* _dev, const std::string modelPath);

	void CreateBoneTree();

	// ���e�N�X�`���̍쐬
	HRESULT CreateWhiteTexture(ID3D12Device* _dev);

	// ���e�N�X�`���̍쐬
	HRESULT CreateBlackTexture(ID3D12Device* _dev);

	HRESULT CreateMaterialBuffer(ID3D12Device* _dev);

	HRESULT CreateBoneBuffer(ID3D12Device* _dev);

	std::wstring GetWstringFromString(const std::string& str);

	ID3D12Resource* LoadTextureFromFile(std::string & texPath, ID3D12Device* _dev);

	HRESULT CreateGrayGradationTexture(ID3D12Device* _dev);

	void CreateBoneOrder(PMXBoneNode& node,int level);

	void RotationMatrix(std::wstring bonename, XMFLOAT3 theta);

	void RecursiveMatrixMultiply(PMXBoneNode& node, XMMATRIX& MultiMat);

	PMXHeader header;// �w�b�_�[��񂪓����Ă��


	std::map<std::wstring, MorphHeader> _morphHeaders;// ���[�ӂ̖��O������[�ӂ̃w�b�_�[�����Ƃ��Ă���
	std::map<std::wstring, std::vector<MorphOffsets>> _morphData;// ���[�ӂ̖��O������[�ӏ����Ƃ��Ă���

	std::vector<PMXVertexInfo> vertexInfo;
	std::vector<unsigned int> _verindex;
	std::vector<std::string>_texVec;
	std::vector<PMXMaterial> _materials;

	// �����e�N�X�`��
	ID3D12Resource* whiteTex = nullptr;
	ID3D12Resource* blackTex = nullptr;
	ID3D12Resource* gradTex = nullptr;

	ID3D12Resource* _boneBuffer;// �{�[���o�b�t�@
	std::vector<ID3D12Resource*> _textureBuffer;// �e�N�X�`���o�b�t�@
	std::vector<ID3D12Resource*> _sphBuffer;// ��Z�X�t�B�A�}�b�v
	std::vector<ID3D12Resource*> _spaBuffer;// ���Z�X�t�B�A�}�b�v
	std::vector<ID3D12Resource*> _toonResources;// �g�D�[��
	std::vector<ID3D12Resource*> _materialsBuff;// �}�e���A���o�b�t�@(��������̂Ńx�N�^�[�ɂ��Ă���)


	std::vector<int>_orderMoveIdx;// ���������ԂɃ{�[���C���f�b�N�X������i���j
	std::vector<BoneInfo>_bones;
	std::vector<BoneInfo>_boneOrders[4];
	std::map<std::wstring, PMXBoneNode> _boneMap;//�{�[���}�b�v
	std::vector<XMMATRIX>_boneMats;// �]���p
	XMMATRIX* _mappedBones = nullptr;

	float angle = 0;

	ID3D12DescriptorHeap* _boneHeap;// �{�[���q�[�v
	ID3D12DescriptorHeap* _matDescHeap;// �}�e���A���f�X�N���v�^�q�[�v

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

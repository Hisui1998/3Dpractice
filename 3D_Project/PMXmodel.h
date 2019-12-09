#pragma once
#include <string>
#include <vector>
#include <map>
#include "Dx12Wrapper.h"

class VMDMotion;

using namespace DirectX;

// PMX���f�����
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

// IK�����N���
struct IKLink
{
	int linkboneIdx;// �����N��̃{�[���C���f�b�N�X
	char isRadlim;// �p�x������t���邩�ǂ���

	XMFLOAT3 minRadlim;// �p�x��������
	XMFLOAT3 maxRadlim;// �p�x�������
};

// IK���
struct IKdata {
	int boneIdx;// �^�[�Q�b�g�{�[���̃C���f�b�N�X
	int loopCnt;// IK���[�v��
	float limrad;// IK���[�v�v�Z���̐����p�x
	int linkNum;// �㑱�̗v�f��

	std::vector<IKLink> ikLinks;// �㑱�̗v�f�z��
};


// ���
#pragma pack(1)
// PMX�t�@�C���̃w�b�_���
struct PMXHeader {
	unsigned char extension[4];//"PMX":4
	float version;// ver(2.0/2.1):4
	unsigned char bytesize;// �㑱����f�[�^��̃o�C�g�T�C�Y  *PMX2.0�� 8 �ŌŒ�
	unsigned char data[8];// �G���R�[�h�����Ƃ��������Ă���o�C�g������
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
	std::wstring name;// �{�[����
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

	IKdata ikdata;// IK�̃f�[�^
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

	// Z�������̕����Ɍ�������
	XMMATRIX LookAtMatrix(const XMVECTOR& lookat, XMFLOAT3& up, XMFLOAT3& right);

	// �C�ӂ̎���C�ӂ̕����֌�������
	XMMATRIX LookAtMatrix(const XMVECTOR origin, const XMVECTOR lookat, XMFLOAT3 up, XMFLOAT3 right);
	
	// �p�C�v���C���̐���
	HRESULT CreatePipeline();

	// ���[�g�V�O�l�`���̐���
	HRESULT CreateRootSignature();

	// ���e�N�X�`���̍쐬
	HRESULT CreateWhiteTexture();

	// ���e�N�X�`���̍쐬
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

	/* �V���h�E�}�b�v�p */
	// �V���h�E�}�b�v�p���[�g�V�O�l�`���̍쐬
	HRESULT CreateShadowRS();

	// �V���h�E�}�b�v�p�p�C�v���C������
	HRESULT CreateShadowPS();

	ID3DBlob* _shadowVertShader = nullptr;// �V���h�E�}�b�v�p���_�V�F�[�_
	ID3DBlob* _shadowPixShader = nullptr;// �V���h�E�}�b�v�p�s�N�Z���V�F�[�_

	ID3D12PipelineState* _shadowMapGPS;
	ID3D12RootSignature* _shadowMapRS;
	/// �V���h�E�}�b�v�����܂�

	PMXHeader header;// �w�b�_�[��񂪓����Ă��

	ID3D12PipelineState* _pmxPipeline=nullptr;// PMX�`��p�p�C�v���C��
	ID3D12RootSignature* _pmxSignature =nullptr;// PMX�`��p���[�g�V�O�l�`��

	D3D12_VIEWPORT _viewPort;// �r���[�|�[�g
	D3D12_RECT _scissor;// �V�U�[�͈�

	std::shared_ptr<VMDMotion>_vmdData;// VMD�̃f�[�^���i�[���Ă���|�C���^

	std::map<std::wstring, MorphHeader> _morphHeaders;// ���[�t�����烂�[�t�̃J�e�S�����̃w�b�_��񂪎擾�ł���(���v���)
	std::map<std::wstring, std::vector<MorphOffsets>> _morphData;// ���[�t�����烂�[�t��񂪎擾�ł���(�w�b�_��񂩂烂�[�t��ނ��擾���Ďg��)

	std::vector<PMXVertexInfo> vertexInfo;// ���_��񂪓����Ă���z��
	std::vector<PMXVertexInfo> firstVertexInfo;// ���_���̏�����񂪓����Ă���z��
	std::vector<unsigned int> _verindex;// ���_�C���f�b�N�X��񂪓����Ă���
	std::vector<std::string>_texturePaths;// �e�N�X�`���̃p�X���i�[���Ă���z��
	std::vector<PMXMaterial> _materials;// �}�e���A����񂪓����Ă���z��
	PMXColor* MapColor = nullptr;// �}�e���A���̃J���[�������ē]������p�̃|�C���^(���g�m�F�p����)

	// ���_�o�b�t�@
	ID3D12Resource* _vertexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW _vbView = {};// ���_�o�b�t�@�r���[

	// �C���f�b�N�X�o�b�t�@
	ID3D12Resource* _indexBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW _idxView = {};// �C���f�b�N�X�o�b�t�@�r���[

	// �����e�N�X�`��
	ID3D12Resource* whiteTex = nullptr;
	ID3D12Resource* blackTex = nullptr;
	ID3D12Resource* gradTex = nullptr;

	// �e�N�X�`���n�o�b�t�@�z��
	std::vector<ID3D12Resource*> _textureBuffer;// �e�N�X�`���o�b�t�@�z��
	static std::map<std::string, ID3D12Resource*>_texMap;// �e�N�X�`���l�[������o�b�t�@�����o���ϐ�
	std::vector<ID3D12Resource*> _sphBuffer;// ��Z�X�t�B�A�}�b�v�o�b�t�@�z��
	std::vector<ID3D12Resource*> _spaBuffer;// ���Z�X�t�B�A�}�b�v�o�b�t�@�z��
	std::vector<ID3D12Resource*> _toonResources;// �g�D�[���o�b�t�@�z��

	// �}�e���A��
	std::vector<ID3D12Resource*> _materialsBuff;// �}�e���A���o�b�t�@
	ID3D12DescriptorHeap* _matDescHeap;// �}�e���A���f�X�N���v�^�q�[�v

	// �{�[���n�̕ϐ�
	ID3D12Resource* _boneBuffer;// �{�[���o�b�t�@
	std::vector<BoneInfo>_bones;// �{�[���̊�{��񂪓����Ă�
	std::map<std::wstring, PMXBoneNode> _boneTree;//�{�[��������q�̃m�[�h���擾�ł���
	std::vector<XMMATRIX>_boneMats;// �{�[���s��(���g�̓{�[���C���f�b�N�X��)
	XMMATRIX* _sendBone = nullptr;// �{�[���s��̓]���p�|�C���^

	//std::map<IKdata>

	ID3D12DescriptorHeap* _boneHeap;// �{�[���q�[�v

	std::string FolderPath;// ���f���������Ă���t�H���_�܂ł̃p�X

	float _morphWeight;// ���[�ӂ̃E�F�C�g(�e�X�g�p)
	int frame = 0;
	std::string modelname[4];
public:
	PMXmodel(ID3D12Device* dev, const std::string modelPath,const std::string vmdPath = "");
	~PMXmodel();

	void UpDate(char key[256]);

	// �`��֐�(���X�g�Ɛ[�x�o�b�t�@�q�[�v�ʒu��WVP�萔�o�b�t�@)
	void Draw(ID3D12GraphicsCommandList* list,ID3D12DescriptorHeap* constant,ID3D12DescriptorHeap*shadow,unsigned int instanceNum=1);

	// �V���h�E�[�x�̕`��
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

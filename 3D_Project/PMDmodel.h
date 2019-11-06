#pragma once
#include <string>
#include "Dx12Wrapper.h"

// PMD�t�@�C���̃w�b�_���̍\����
struct PMDHeader {
	char signature[3];//pmd
	float version;// 00 00 80 3f==1.00
	char model_name[20];
	char comment[256];
};

/// �p�f�B���O���P�o�C�g�P�ʂɂ�����
#pragma pack (1)
// ���_���\����
struct PMDVertexInfo
{
	float pos[3];//12
	float normal_vec[3];//12
	float uv[2];//8
	unsigned short bone_num[2];//4 // �{�[���ԍ� 1�A�ԍ� 2
	unsigned char bone_weight;//1
	unsigned char edge_flag;//1
};

// PMD�t�@�C���̃}�e���A���\����
struct PMDMaterial {
	DirectX::XMFLOAT3 diffuse_color;// 12
	float alpha;// 4
	float specularity;// 4
	DirectX::XMFLOAT3 specular_color;// 12
	DirectX::XMFLOAT3 mirror_color;// 12
	unsigned char toon_index;// 1
	unsigned char edge_flag;// 1
	// �����łQ�o�C�g���܂�
	unsigned int faceVerCnt;// 4
	char texFileName[20];//20
};

// PMD���f���̃{�[�����\����
struct PMDBoneInfo {
	char bone_name[20];// �{�[����:20
	unsigned short parent_bone_index;// �e�̃{�[���ԍ�(�Ȃ��Ƃ���0xffff):2
	unsigned short tail_pos_bone_index;// �悭�킩��񂯂Ǒ����������ǂ����݂����ȁF2
	char bone_type; // �{�[���̎��:1
	unsigned short ik_parent_bone_index; // IK�{�[���ԍ�(�e��IK�{�[���B�Ȃ��ꍇ��0):2
	DirectX::XMFLOAT3 bone_head_pos; // �{�[���̃w�b�h�̍��W:12
};
#pragma pack()
/// �p�f�B���O�����ɖ߂�()�S�o�C�g

// �{�[���̃m�[�h���
struct BoneNode {
	int boneIdx;// �{�[���̍s��z��ƑΉ�
	DirectX::XMFLOAT3 startPos;// �{�[���n�_
	DirectX::XMFLOAT3 endPos;// �{�[���I�_
	std::vector<BoneNode*>children;// �q�������ւ̃����N
};

// �F���̍\����
struct PMDColor
{
	DirectX::XMFLOAT4 diffuse;
	DirectX::XMFLOAT4 specular;
	DirectX::XMFLOAT3 ambient;
};


class PMDmodel
{
private:
	/* �v���C�x�[�g�֐� */
	void LoadModel(ID3D12Device* _dev,const std::string modelPath);
	
	// ���_�o�b�t�@�̍쐬
	HRESULT CreateVertexBuffer(ID3D12Device* _dev);

	// �C���f�b�N�X�o�b�t�@�̍쐬
	HRESULT CreateIndexBuffer(ID3D12Device* _dev);

	// �g���q���擾����֐�
	std::string GetExtension(const std::string& path);

	// �e�N�X�`���̃p�X�����o���֐�
	std::string GetTexPath(const std::string& modelPath, const char* texPath);

	// �p�X�𕪊�����֐�
	std::pair<std::string, std::string>SplitFileName(const std::string&path, const char splitter = '*');

	// �t�@�C����ǂݍ���Ńe�N�X�`���o�b�t�@���쐬����֐�
	ID3D12Resource*	LoadTextureFromFile(std::string& texPath,ID3D12Device* _dev);

	// �}���`�o�C�g�����񂩂烏�C�h������ւ̕ϊ����s���֐�
	std::wstring GetWstringFromString(const std::string& str);

	// �{�[���c���[�쐬
	void CreateBoneTree();

	// �}�e���A���o�b�t�@�ƃ}�e���A���o�b�t�@�r���[�����֐�
	HRESULT CreateMaterialBuffer(ID3D12Device* _dev);

	// �{�[���o�b�t�@�̍쐬
	HRESULT CreateBoneBuffer(ID3D12Device* _dev);

	// ���e�N�X�`���̍쐬
	HRESULT CreateWhiteTexture(ID3D12Device* _dev);

	// ���e�N�X�`���̍쐬
	HRESULT CreateBlackTexture(ID3D12Device* _dev);

	// �g�D�[���̂��߂̃O���f�[�V�����e�N�X�`���̍쐬
	HRESULT	CreateGrayGradationTexture(ID3D12Device* _dev);

	// ���s�ړ����ĉ�]
	void RotationMatrix(std::string bonename, DirectX::XMFLOAT3 theta);

	// �q�̃m�[�h�܂ōċA�I�ɍs���Z����֐�
	void RecursiveMatrixMultiply(BoneNode& node, DirectX::XMMATRIX& MultiMat);

	/* �v���C�x�[�g�ϐ� */
	ID3D12Resource* _boneBuffer;// �{�[���o�b�t�@
	std::vector<ID3D12Resource*> _textureBuffer;// �e�N�X�`���o�b�t�@
	std::vector<ID3D12Resource*> _sphBuffer;// ��Z�X�t�B�A�}�b�v
	std::vector<ID3D12Resource*> _spaBuffer;// ���Z�X�t�B�A�}�b�v
	std::vector<ID3D12Resource*> _toonResources;// �g�D�[��
	std::vector<ID3D12Resource*> _materialsBuff;// �}�e���A���o�b�t�@(��������̂Ńx�N�^�[�ɂ��Ă���)

	ID3D12DescriptorHeap* _boneHeap;// �{�[���q�[�v
	ID3D12DescriptorHeap* _matDescHeap;// �}�e���A���f�X�N���v�^�q�[�v

	std::vector<PMDVertexInfo> _vivec;// ���_�����i�[���Ă���z��
	std::vector<unsigned short> _verindex;// ���_�ԍ����i�[����Ă���z��
	std::vector<PMDMaterial> _materials;// �}�e���A��

	// �{�[���֌W�̕ϐ�
	std::vector<PMDBoneInfo> _bones;
	std::vector<DirectX::XMMATRIX>_boneMats;
	std::map<std::string, BoneNode>_boneMap;
	DirectX::XMMATRIX* _mappedBones;

	// �e�N�X�`���̖��O�ƃo�b�t�@�������Ă���z��
	std::map<std::string, ID3D12Resource*>_resourceTable;

	// �����e�N�X�`��
	ID3D12Resource* whiteTex = nullptr;
	ID3D12Resource* blackTex = nullptr;
	ID3D12Resource* gradTex = nullptr;
	
	// ���_�o�b�t�@
	ID3D12Resource* _vertexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW _vbView = {};// ���_�o�b�t�@�r���[

	// �C���f�b�N�X�o�b�t�@
	ID3D12Resource* _indexBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW _idxView = {};// �C���f�b�N�X�o�b�t�@�r���[

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

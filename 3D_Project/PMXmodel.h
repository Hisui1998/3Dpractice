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
	// �R�����g�����邩��Fseek���邱��
	int faceVerCnt;
};
#pragma pack()

// �F���̍\����
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

	// ���e�N�X�`���̍쐬
	HRESULT CreateWhiteTexture(ID3D12Device* _dev);

	// ���e�N�X�`���̍쐬
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

	std::vector<DirectX::XMMATRIX>_boneMats;
	DirectX::XMMATRIX* _mappedBones;

	ID3D12DescriptorHeap* _boneHeap;// �{�[���q�[�v
	ID3D12DescriptorHeap* _matDescHeap;// �}�e���A���f�X�N���v�^�q�[�v

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


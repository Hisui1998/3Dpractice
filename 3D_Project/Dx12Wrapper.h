#pragma once
#include <windows.h>
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <vector>
#include <DirectXMath.h>

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
struct VertexInfo
{
	float pos[3];//12
	float normal_vec[3];//12
	float uv[2];//8
	unsigned short bone_num[2];//4
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
	unsigned int indexNum;// 4
	char texFileName[20];//20
};
#pragma pack()
/// �p�f�B���O�����ɖ߂�()�S�o�C�g

// �F���̍\����
struct PMDColor
{
	DirectX::XMFLOAT4 diffuse;
	DirectX::XMFLOAT4 specular;
	DirectX::XMFLOAT3 ambient;
};

// �s��̍\����
struct WVPMatrix {
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX view;
	DirectX::XMMATRIX projection;
	DirectX::XMMATRIX wvp;
	DirectX::XMMATRIX lvp;
};

class Dx12Wrapper
{
private:
	//  ---�f�o�C�X�n�ϐ�---  //
	HWND _hwnd;
	IDXGIFactory6* _dxgi = nullptr;
	IDXGISwapChain4* _swapchain = nullptr;
	ID3D12Device* _dev = nullptr;
	ID3D12CommandAllocator* _cmdAlloc = nullptr;
	ID3D12GraphicsCommandList* _cmdList = nullptr;
	ID3D12CommandQueue* _cmdQue = nullptr;

	D3D12_VERTEX_BUFFER_VIEW _vbView = {};
	D3D12_INDEX_BUFFER_VIEW _ibView = {};

	ID3D12Fence* _fence = nullptr;
	UINT64 _fenceValue = 0;

	ID3D12DescriptorHeap* _swcDescHeap = nullptr;// SWC(�X���b�v�`�F�C��)�f�X�N���v�^�q�[�v
	ID3D12DescriptorHeap* _rtvDescHeap = nullptr;// RTV(�����_�[�^�[�Q�b�g)�f�X�N���v�^�q�[�v
	ID3D12DescriptorHeap* _dsvDescHeap;// DSV(�[�x)�f�X�N���v�^�q�[�v
	ID3D12DescriptorHeap* _depthSrvHeap;// �[�x�V�F�[�_�[���\�[�X�r���[�q�[�v
	ID3D12DescriptorHeap* _srvDescHeap;// ���̑�(�e�N�X�`���A�萔)�f�X�N���v�^�q�[�v
	ID3D12DescriptorHeap* _rgstDescHeap;
	ID3D12DescriptorHeap* _matDescHeap;// �}�e���A���f�X�N���v�^�q�[�v

	std::vector<ID3D12Resource*>renderTargets;

	ID3D12Resource* _vertexBuffer = nullptr;// ���_�o�b�t�@
	ID3D12Resource* _indexBuffer = nullptr;// �C���f�b�N�X�o�b�t�@
	ID3D12Resource* _constBuff = nullptr;// �萔�o�b�t�@
	ID3D12Resource* _depthBuffer = nullptr;// �[�x�o�b�t�@
	std::vector<ID3D12Resource*> _materialsBuff;// �}�e���A���o�b�t�@(��������̂Ńx�N�^�[�ɂ��Ă���)
	std::vector<ID3D12Resource*> _textureBuffer;// �e�N�X�`���o�b�t�@
	ID3D12Resource* whiteBuff = nullptr;

	ID3DBlob* vertexShader = nullptr;
	ID3DBlob* pixelShader = nullptr;

	ID3D12RootSignature* _rootSignature = nullptr;
	ID3DBlob* signature = nullptr;
	ID3DBlob* error = nullptr;

	ID3D12PipelineState* _pipelineState = nullptr;

	//  ---�֐�---  //
	/// Init�n(?)

	// �f�o�C�X�̏�����
	HRESULT DeviceInit();
	/// Create�n(?)

	// �X���b�v�`�F�C���ƃR�}���h�L���[�̍쐬
	HRESULT CreateSwapChainAndCmdQue();

	// �R�}���h���X�g�ƃR�}���h�A���P�[�^�̍쐬
	HRESULT CreateCmdListAndAlloc();

	// ���[�g�V�O�l�`�������֐�(�����W�ƃp�����[�^�����̒�)
	HRESULT CreateRootSignature();

	// �t�F���X�̍쐬
	HRESULT CreateFence();

	// �p�C�v���C�������֐�(���_���C�A�E�g�̐ݒ�͂��̒�)
	HRESULT CreateGraphicsPipelineState();

	// �C���f�b�N�X�o�b�t�@�ƒ��_�o�b�t�@�̍쐬
	HRESULT CreateBuffersForIndexAndVertex();

	// �[�x�o�b�t�@�Ɛ[�x�o�b�t�@�r���[�����֐�
	HRESULT CreateDSV();

	// �萔�o�b�t�@�̍쐬
	HRESULT CreateConstantBuffer();

	// �}�e���A���o�b�t�@�ƃ}�e���A���o�b�t�@�r���[�����֐�
	HRESULT CreateMaterialBuffer();

	// �萔�o�b�t�@�̍쐬
	HRESULT CreateWhiteBuffer();
	// ---���̑��֐�--- //

	// �V�F�[�_�[�̂�݂��݂��s���֐�
	HRESULT LoadShader();
			
	// ���f���f�[�^�̓ǂݍ��݂��s���֐�
	HRESULT LoadPMD();

	// �e�N�X�`���̃p�X�����o���֐�
	std::string GetTexPath(const std::string& modelPath, const char* texPath);
	
	// �}���`�o�C�g�����񂩂烏�C�h������ւ̕ϊ����s���֐�
	std::wstring GetWstringFromString(const std::string& str);

	// �t�@�C����ǂݍ���Ńe�N�X�`���o�b�t�@���쐬����֐�
	ID3D12Resource*	LoadTextureFromFile(std::string& texPath);

	// �g���q���擾����֐�
	std::string GetExtension(const std::string& path);

	// �p�X�𕪊�����֐�
	std::pair<std::string, std::string>SplitFileName(const std::string&path,const char splitter='*');

	// �o�b�t�@�ɉ�������I���܂ŃE�F�C�g��������֐�
	void WaitWithFence();

	// �R�}���h���������I��������ǂ�����Ԃ��֐�
	void ExecuteCommand();

	//�@---�ȉ��ϐ�--- //
	WVPMatrix* _wvpMP;
	WVPMatrix _wvp;

	std::vector<VertexInfo> _vivec;
	std::vector<unsigned short> verindex;
	std::vector<PMDMaterial> _materials;

	PMDColor* MapColor = nullptr;

	float angle;// �p�x
public:
	Dx12Wrapper(HWND hwnd);
	~Dx12Wrapper();

	// ������
	int Init();

	// �X�V
	void UpDate();
};


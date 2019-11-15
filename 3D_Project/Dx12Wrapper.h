#pragma once
#include <windows.h>
#include <d3dx12.h>
#include <d3d12.h>
#include <dinput.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <vector>
#include <map>

class PMDmodel;
class PMXmodel;

// �s��̍\����
struct WVPMatrix {
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX view;
	DirectX::XMMATRIX projection;
	DirectX::XMMATRIX wvp;
	DirectX::XMMATRIX lvp;
};

// ���_���
struct Vertex
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT2 uv;
};

class Dx12Wrapper
{
private:
	//  ---�f�o�C�X�n�ϐ�---  //
	HWND _hwnd;// �E�B���h�E�n���h��

	IDXGIFactory6* _dxgi = nullptr;// �t�@�N�g��

	IDXGISwapChain4* _swapchain = nullptr;// �X���b�v�`�F�C��

	ID3D12Device* _dev = nullptr;// �f�o�C�X
	ID3D12Device* _keydev = nullptr;// �L�[�{�[�h�f�o�C�X

	// �R�}���h�n
	ID3D12CommandAllocator* _cmdAlloc = nullptr;// �R�}���h�A���P�[�^
	ID3D12GraphicsCommandList* _cmdList = nullptr;// �R�}���h���X�g
	ID3D12CommandQueue* _cmdQue = nullptr;// �R�}���h�L���[

	LPDIRECTINPUT8       _directInput;// DirectInput�{��
	LPDIRECTINPUTDEVICE8 _keyBoadDev;// �L�[�{�[�h�f�o�C�X

	ID3D12Fence* _fence = nullptr;// �t�F���X
	UINT64 _fenceValue = 0;// �t�F���X�l

	ID3D12DescriptorHeap* _dsvDescHeap;// DSV(�[�x)�f�X�N���v�^�q�[�v
	ID3D12Resource* _depthBuffer = nullptr;// �[�x�o�b�t�@

	ID3D12DescriptorHeap* _cameraDescHeap;// �J�����p�̒萔�o�b�t�@�p�f�X�N���v�^�q�[�v
	ID3D12Resource* _cameraBuff = nullptr;// �J�����p�̒萔�o�b�t�@

	D3D12_VIEWPORT _viewPort;// �r���[�|�[�g
	D3D12_RECT _scissor;// �V�U�[�͈�

	// �y���|���p
	ID3D12Resource* _peraBuffer = nullptr;// �y���|���{�̂̃o�b�t�@
	ID3D12Resource* _peraBuffer2 = nullptr;// �y���|��2�{�̂̃o�b�t�@

	ID3D12Resource* _peraVertBuff = nullptr;// �y���|���p���_�o�b�t�@
	D3D12_VERTEX_BUFFER_VIEW _peravbView = {};// �y���|���p���_�o�b�t�@�r���[

	ID3D12DescriptorHeap* _rtvDescHeap = nullptr;// �y���|���p�����_�[�^�[�Q�b�g�f�X�N���v�^�q�[�v	
	ID3D12DescriptorHeap* _rtvDescHeap2 = nullptr;// �y���|��2�p�����_�[�^�[�Q�b�g�f�X�N���v�^�q�[�v	
	ID3D12DescriptorHeap* _srvDescHeap = nullptr;// �y���|���p�V�F�[�_�[���\�[�X�f�X�N���v�^�q�[�v
	ID3D12DescriptorHeap* _srvDescHeap2 = nullptr;// �y���|��2�p�V�F�[�_�[���\�[�X�f�X�N���v�^�q�[�v

	ID3DBlob* peraVertShader = nullptr;// �y���|���p���_�V�F�[�_
	ID3DBlob* peraVertShader2 = nullptr;// �y���|��2�p���_�V�F�[�_
	ID3DBlob* peraPixShader = nullptr;// �y���|���p�s�N�Z���V�F�[�_
	ID3DBlob* peraPixShader2 = nullptr;// �y���|��2�p�s�N�Z���V�F�[�_

	ID3D12PipelineState* _peraPipeline = nullptr;// �y���|���p�p�C�v���C��
	ID3D12RootSignature* _peraSignature = nullptr;// �y���|���p���[�g�V�O�l�`��
	

	// �X���b�v�`�F�C���p
	ID3D12DescriptorHeap* _swcDescHeap = nullptr;// SWC(�X���b�v�`�F�C��)�f�X�N���v�^�q�[�v		

	ID3D12RootSignature* _rootSignature = nullptr;
	ID3DBlob* signature = nullptr;
	ID3DBlob* error = nullptr;

	ID3D12PipelineState* _pipelineState = nullptr;
	std::vector<ID3D12Resource*>renderTargets;

	ID3DBlob* vertexShader = nullptr;// ���_�V�F�[�_
	ID3DBlob* pixelShader = nullptr;// �s�N�Z���V�F�[�_

	/*�@�쐬�Ə������n�֐��@*/
	// �f�o�C�X�̏�����
	HRESULT DeviceInit();

	// �X���b�v�`�F�C���ƃR�}���h�L���[�̍쐬
	HRESULT CreateSwapChainAndCmdQue();

	// �R�}���h���X�g�ƃR�}���h�A���P�[�^�̍쐬
	HRESULT CreateCmdListAndAlloc();

	// ���[�g�V�O�l�`�������֐�(�����W�ƃp�����[�^�����̒�)
	HRESULT CreateRootSignature();

	HRESULT CreatePeraRootSignature();

	// �t�F���X�̍쐬
	HRESULT CreateFence();

	// �p�C�v���C�������֐�(���_���C�A�E�g�̐ݒ�͂��̒�)
	HRESULT CreateGraphicsPipelineState();

	HRESULT CreatePeraPopelineState();

	// �[�x�o�b�t�@�Ɛ[�x�o�b�t�@�r���[�����֐�
	HRESULT CreateDSV();

	// �萔�o�b�t�@�̍쐬
	HRESULT CreateConstantBuffer();

	// �q�[�v�ƃr���[�̍쐬
	HRESULT CreateHeapAndView();

	// �؂�ۂ�̍쐬
	HRESULT CreatePolygon();

	void DrawPolygon();

	// �V�F�[�_�[�̂�݂��݂��s���֐�
	HRESULT LoadShader();
	
	// �o�b�t�@�ɉ�������I���܂ŃE�F�C�g��������֐�
	void WaitWithFence();

	// �R�}���h���������I��������ǂ�����Ԃ��֐�
	void ExecuteCommand();

	//�@---�ȉ��ϐ�--- //
	
	WVPMatrix* _wvpMP;
	WVPMatrix _wvp;
	float angle;// �p�x
	int cnt = 0;
	char key[256] = {};

	// �J�����p
	DirectX::XMFLOAT3 eye;// �J�����̍��W
	DirectX::XMFLOAT3 target;// �^�[�Q�b�g�̍��W
	DirectX::XMFLOAT3 up;// ����

	std::shared_ptr<PMDmodel> pmdModel;
	std::vector<std::shared_ptr<PMXmodel>> pmxModels;
	bool isPMD = false;
public:
	Dx12Wrapper(HWND hwnd);
	~Dx12Wrapper();

	// ������
	int Init();

	// �X�V
	void UpDate();
};

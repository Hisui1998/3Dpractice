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

class Dx12Wrapper
{
private:
	//  ---�f�o�C�X�n�ϐ�---  //
	HWND _hwnd;
	IDXGIFactory6* _dxgi = nullptr;
	IDXGISwapChain4* _swapchain = nullptr;
	ID3D12Device* _dev = nullptr;
	ID3D12Device* _keydev = nullptr;
	ID3D12CommandAllocator* _cmdAlloc = nullptr;
	ID3D12GraphicsCommandList* _cmdList = nullptr;
	ID3D12CommandQueue* _cmdQue = nullptr;

	LPDIRECTINPUT8       _directInput;// DirectInput�{��
	LPDIRECTINPUTDEVICE8 _keyBoadDev;// �L�[�{�[�h�f�o�C�X

	ID3D12Fence* _fence = nullptr;
	UINT64 _fenceValue = 0;

	ID3D12DescriptorHeap* _swcDescHeap = nullptr;// SWC(�X���b�v�`�F�C��)�f�X�N���v�^�q�[�v
	ID3D12DescriptorHeap* _rtvDescHeap = nullptr;// RTV(�����_�[�^�[�Q�b�g)�f�X�N���v�^�q�[�v
	ID3D12DescriptorHeap* _dsvDescHeap;// DSV(�[�x)�f�X�N���v�^�q�[�v
	ID3D12DescriptorHeap* _depthSrvHeap;// �[�x�V�F�[�_�[���\�[�X�r���[�q�[�v
	ID3D12DescriptorHeap* _srvDescHeap;// ���̑�(�e�N�X�`���A�萔)�f�X�N���v�^�q�[�v
	ID3D12DescriptorHeap* _rgstDescHeap;

	std::vector<ID3D12Resource*>renderTargets;

	ID3D12Resource* _constBuff = nullptr;// �萔�o�b�t�@
	ID3D12Resource* _depthBuffer = nullptr;// �[�x�o�b�t�@

	ID3DBlob* vertexShader = nullptr;
	ID3DBlob* pixelShader = nullptr;

	ID3D12RootSignature* _rootSignature = nullptr;
	ID3DBlob* signature = nullptr;
	ID3DBlob* error = nullptr;

	ID3D12PipelineState* _pipelineState = nullptr;

	/*�@�쐬�Ə������n�֐��@*/
	// �f�o�C�X�̏�����
	HRESULT DeviceInit();

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

	// �[�x�o�b�t�@�Ɛ[�x�o�b�t�@�r���[�����֐�
	HRESULT CreateDSV();

	// �萔�o�b�t�@�̍쐬
	HRESULT CreateConstantBuffer();

	// ---���̑��֐�--- //
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

	DirectX::XMFLOAT3 eye;
	DirectX::XMFLOAT3 target;
	DirectX::XMFLOAT3 up;

	std::shared_ptr<PMDmodel> pmdModel;
	std::shared_ptr<PMXmodel> pmxModel;
	bool isPMD = false;
public:
	Dx12Wrapper(HWND hwnd);
	~Dx12Wrapper();

	// ������
	int Init();

	// �X�V
	void UpDate();
};


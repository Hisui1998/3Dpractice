#pragma once
#include <windows.h>
#include <d3dx12.h>
#include <d3d12.h>
#include <dinput.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <vector>
#include <map>
#include <Effekseer.h>
#include <EffekseerRendererDX12.h>
#pragma comment(lib,"LLGI.lib")
#pragma comment(lib,"Effekseer.lib")
#pragma comment(lib,"EffekseerRendererDX12.lib")

class PMDmodel;
class PMXmodel;
class Plane;

// �s��̍\����
struct WVPMatrix {
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX view;
	DirectX::XMMATRIX projection;
	DirectX::XMMATRIX wvp;
	DirectX::XMMATRIX lvp;
	DirectX::XMFLOAT3 lightPos;
};

// ���_���
struct Vertex
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT2 uv;
};

// ���C�g�̐F
struct LightColor
{
	float bloom[4];
};

// �e��t���O�Ȃ�
struct Flags
{
	bool GBuffers;
};

class Dx12Wrapper
{
private:
	// ���b�p�[�p�֐�
	void KeyUpDate();// �L�[���X�V
	void ScreenUpDate();// ��ʂ̍X�V

	// �f�o�C�X�̏�����
	HRESULT DeviceInit();

	// �X���b�v�`�F�C���ƃR�}���h�L���[�̍쐬
	HRESULT CreateSwapChainAndCmdQue();

	// �R�}���h���X�g�ƃR�}���h�A���P�[�^�̍쐬
	HRESULT CreateCmdListAndAlloc();

	// �t�F���X�̍쐬
	HRESULT CreateFence();

	// �[�x�o�b�t�@�Ɛ[�x�o�b�t�@�r���[�����֐�
	HRESULT CreateDepthStencilView();

	// WVP�p�萔�o�b�t�@�̍쐬
	HRESULT CreateWVPConstantBuffer();

	// �o�b�t�@�ɉ�������I���܂ŃE�F�C�g��������֐�
	void WaitWithFence();

	// �R�}���h���������I��������ǂ�����Ԃ��֐�
	void ExecuteCommand();

	// ���b�p�[�p�ϐ�
	HWND _hwnd;// �E�B���h�E�n���h��
	IDXGIFactory6* _dxgi = nullptr;// �t�@�N�g��
	IDXGISwapChain4* _swapchain = nullptr;// �X���b�v�`�F�C��
	ID3D12Device* _dev = nullptr;// �f�o�C�X

	LPDIRECTINPUT8       _directInput;// DirectInput�{��
	LPDIRECTINPUTDEVICE8 _keyBoadDev;// �L�[�{�[�h�f�o�C�X

	ID3D12DescriptorHeap* _imguiDescHeap;// imgui�p��de�X�N���v�^�q�[�v

	// �R�}���h�n
	ID3D12CommandAllocator* _cmdAlloc = nullptr;// �R�}���h�A���P�[�^
	ID3D12GraphicsCommandList* _cmdList = nullptr;// �R�}���h���X�g
	ID3D12CommandQueue* _cmdQue = nullptr;// �R�}���h�L���[

	ID3D12Fence* _fence = nullptr;// �t�F���X
	UINT64 _fenceValue = 0;// �t�F���X�l

	// �[�x�o�b�t�@�p
	ID3D12Resource* _depthBuffer = nullptr;// �[�x�o�b�t�@
	ID3D12DescriptorHeap* _dsvDescHeap;// �[�x�o�b�t�@�f�X�N���v�^�q�[�v
	ID3D12DescriptorHeap* _dsvSrvHeap;// �[�xSRV�f�X�N���v�^�q�[�v	

	// �V���h�E�o�b�t�@
	ID3D12Resource* _lightBuffer = nullptr;// �[�x�o�b�t�@
	ID3D12DescriptorHeap* _lightDescHeap;// �[�x�o�b�t�@�f�X�N���v�^�q�[�v
	ID3D12DescriptorHeap* _lightSrvHeap;// �[�xSRV�f�X�N���v�^�q�[�v	

	// ���点���[��
	ID3D12Resource* _shrinkBuffer = nullptr;// ���P�x�o�b�t�@
	ID3D12DescriptorHeap* _shrinkRtvHeap;// �[�x�o�b�t�@�f�X�N���v�^�q�[�v
	ID3D12DescriptorHeap* _shrinkSrvHeap;// �[�xSRV�f�X�N���v�^�q�[��

	HRESULT CreateShrinkBuffer();// �o�b�t�@�쐬
	void DrawToShrinkBuffer();// �o�b�t�@�ւ̏�������
	ID3D12PipelineState* _shrinkPipeline = nullptr;// �p�C�v���C��
	HRESULT CreateShrinkPipline();

	// �Ђ��Ⴉ�����[���
	ID3D12Resource* _sceneBuffer = nullptr;// ���P�x�o�b�t�@
	ID3D12DescriptorHeap* _sceneRtvHeap;// �[�x�o�b�t�@�f�X�N���v�^�q�[�v
	ID3D12DescriptorHeap* _sceneSrvHeap;// �[�xSRV�f�X�N���v�^�q�[��

	HRESULT CreateSceneBuffer();// �o�b�t�@�쐬
	void DrawToSceneBuffer();// �o�b�t�@�ւ̏�������
	ID3D12PipelineState* _scenePipeline = nullptr;// �p�C�v���C��
	HRESULT CreateScenePipline();

	// �X���b�v�`�F�C���p
	ID3D12DescriptorHeap* _swcDescHeap = nullptr;// SWC(�X���b�v�`�F�C��)�f�X�N���v�^�q�[�v
	std::vector<ID3D12Resource*>renderTargets;// �X���b�v�`�F�C���Ŏg���o�b�t�@��RTV	

	int _persLevel = 0;// �p�[�X�̃��x��
	char key[256] = {};

	D3D12_VIEWPORT _viewPort;// �r���[�|�[�g
	D3D12_RECT _scissor;// �V�U�[�͈�

	// WVP�p
	struct Axis
	{
		DirectX::XMFLOAT3 x;
		DirectX::XMFLOAT3 y;
		DirectX::XMFLOAT3 z;
		Axis() :x(), y(), z() {};
		Axis(DirectX::XMFLOAT3 inx, DirectX::XMFLOAT3 iny, DirectX::XMFLOAT3 inz) :x(inx), y(iny), z(inz) {};
	};

	ID3D12Resource* _wvpBuff = nullptr;// WVP�p�̒萔�o�b�t�@
	ID3D12DescriptorHeap* _wvpDescHeap;// WVP�p�̒萔�o�b�t�@�f�X�N���v�^�q�[�v
	WVPMatrix* _wvpMP;
	WVPMatrix _wvp;
	DirectX::XMFLOAT4 angle;// �p�x
	float lightangle = 0;// �p�x
	DirectX::XMFLOAT3 eye;// ���_�̍��W
	DirectX::XMFLOAT3 target;// �ǂ������Ă��邩�̍��W
	DirectX::XMFLOAT3 lightTag;// �ǂ������Ă��邩�̍��W
	DirectX::XMFLOAT3 up;// ��
	DirectX::XMFLOAT3 lightVec;// ���C�g�̕��s�����̕���
	DirectX::XMMATRIX lightproj;
	int instanceNum = 1;

	std::shared_ptr<PMDmodel> pmdModel;
	std::vector<std::shared_ptr<PMXmodel>> pmxModels;
	std::string name;
	std::shared_ptr<Plane> _plane;

	HRESULT CreateFlagsBuffer();
	Flags flags;
	Flags *MapFlags;
	ID3D12Resource* _flagsBuffer;
	ID3D12DescriptorHeap* _flagsHeap;

	HRESULT CreateColBuffer();
	LightColor bloomCol;
	LightColor *MapCol;
	ID3D12Resource* _colorBuffer;
	ID3D12DescriptorHeap* _colDescHeap;

	// �G�t�F�N�V�A�֌W�̊֐�
	void SetEfkRenderer();
	Effekseer::Handle _efkHandle;
	Effekseer::Effect* _effect = nullptr;
	Effekseer::Manager* _efkManager = nullptr;
	EffekseerRenderer::Renderer* _efkRenderer = nullptr;
	EffekseerRenderer::SingleFrameMemoryPool* _efkMemoryPool = nullptr;
	EffekseerRenderer::CommandList* _efkCmdList = nullptr;

	/*�|���p�֐�(���ƂŃN���X�ɕ�����\��)*/
	// �|���S���p�̒��_�o�b�t�@�̍쐬
	HRESULT CreatePolygonVertexBuffer();

	// �ꖇ�ڃ|���S���p���[�g�V�O�l�`���̍쐬
	HRESULT CreateFirstSignature();

	// �񖇖ڃ|���S���p���[�g�V�O�l�`���̍쐬
	HRESULT CreateSecondSignature();

	// �ꖇ�ڃ|���S���p�p�C�v���C������
	HRESULT CreateFirstPopelineState();

	// �񖇖ڃ|���S���p�p�C�v���C������
	HRESULT CreateSecondPopelineState();

	// �ꖇ�ڃ|���S���̍쐬
	HRESULT CreateFirstPolygon();

	// �ꖇ�ڃ|���S���̍쐬
	HRESULT CreateSecondPolygon();

	// �|���S���`��֐�
	void DrawFirstPolygon();
	void DrawSecondPolygon();

	// �y���|���p�ϐ�
	std::vector<ID3D12Resource*> _peraBuffers;// �y���|���{�̂̃o�b�t�@

	ID3D12Resource* _peraBuffer2 = nullptr;// �y���|��2�{�̂̃o�b�t�@

	ID3D12Resource* _peraVertBuff = nullptr;// �y���|���p���_�o�b�t�@
	D3D12_VERTEX_BUFFER_VIEW _peravbView = {};// �y���|���p���_�o�b�t�@�r���[
	
	ID3D12DescriptorHeap* _rtvDescHeap = nullptr;// �y���|���p�����_�[�^�[�Q�b�g�f�X�N���v�^�q�[�v	
	ID3D12DescriptorHeap* _srvDescHeap = nullptr;// �y���|���p�V�F�[�_�[���\�[�X�f�X�N���v�^�q�[�v

	ID3D12DescriptorHeap* _rtvDescHeap2 = nullptr;// �y���|��2�p�����_�[�^�[�Q�b�g�f�X�N���v�^�q�[�v	
	ID3D12DescriptorHeap* _srvDescHeap2 = nullptr;// �y���|��2�p�V�F�[�_�[���\�[�X�f�X�N���v�^�q�[�v

	ID3DBlob* peraVertShader = nullptr;// �y���|���p���_�V�F�[�_
	ID3DBlob* peraPixShader = nullptr;// �y���|���p�s�N�Z���V�F�[�_

	ID3DBlob* peraVertShader2 = nullptr;// �y���|��2�p���_�V�F�[�_
	ID3DBlob* peraPixShader2 = nullptr;// �y���|��2�p�s�N�Z���V�F�[�_

	ID3D12PipelineState* _peraPipeline = nullptr;// �y���|���p�p�C�v���C��
	ID3D12RootSignature* _peraSignature = nullptr;// �y���|���p���[�g�V�O�l�`��

	ID3D12PipelineState* _peraPipeline2 = nullptr;// �y���|��2�p�p�C�v���C��
	ID3D12RootSignature* _peraSignature2 = nullptr;// �y���|��2�p���[�g�V�O�l�`��

	// ���[�g�V�O�l�`���̍쐬���Ɏg���ϐ�
	ID3DBlob* signature = nullptr;
	ID3DBlob* error = nullptr;
	/// �|���p�����܂�
public:
	Dx12Wrapper(HWND hwnd);
	~Dx12Wrapper();

	// ������
	int Init();

	// �X�V
	void UpDate();

	ID3D12Device* Device() { return _dev; };
	ID3D12CommandQueue* CmdQue() { return _cmdQue; };
	ID3D12GraphicsCommandList* CmdList() { return _cmdList; };
	ID3D12CommandAllocator* CmdAlloc() { return _cmdAlloc; };
};

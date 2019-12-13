#include "Dx12Wrapper.h"
#include "Application.h"
#include <wrl.h>
#include <d3dcompiler.h>
#include <DirectXTex.h>
#include <algorithm>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx12.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_internal.h"

#include "PMDmodel.h"
#include "PMXmodel.h"
#include "Plane.h"

#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"DirectXTex.lib")

const FLOAT clearColor[] = { 0.01f,0.01f,0.01f,0.0f };//�N���A�J���[�ݒ�

// �f�o�C�X�̍쐬
HRESULT Dx12Wrapper::DeviceInit()
{
	D3D_FEATURE_LEVEL levels[] = {
		  D3D_FEATURE_LEVEL_12_1,
		  D3D_FEATURE_LEVEL_12_0,
		  D3D_FEATURE_LEVEL_11_1,
		  D3D_FEATURE_LEVEL_11_0,
	};

	// ���x���`�F�b�N
	D3D_FEATURE_LEVEL level = D3D_FEATURE_LEVEL::D3D_FEATURE_LEVEL_12_1;
	HRESULT result = S_OK;
	for (auto lv : levels) {
		// �쐬
		result = D3D12CreateDevice(nullptr, lv, IID_PPV_ARGS(&_dev));
		// �������Ă��邩�̃`�F�b�N
		if (SUCCEEDED(result)) {
			level = lv;
			break;
		}
	}

	result = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgi));

	// DirectInput�I�u�W�F�N�g
	result = DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8,
		(void**)&_directInput, NULL);

	// �L�[�f�o�C�X�̍쐬
	result = _directInput->CreateDevice(GUID_SysKeyboard, &_keyBoadDev, nullptr);
	result =  _keyBoadDev->SetDataFormat(&c_dfDIKeyboard);
	result = _keyBoadDev->SetCooperativeLevel(_hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);

	return result;
}

// �X���b�v�`�F�C���ƃR�}���h�L���[�̍쐬
HRESULT Dx12Wrapper::CreateSwapChainAndCmdQue()
{
	// �R�}���h�L���[�쐬�Ɏg�����
	D3D12_COMMAND_QUEUE_DESC cmdQDesc = {};
	cmdQDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmdQDesc.NodeMask = 0;
	cmdQDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmdQDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	// �R�}���h�L���[�̍쐬
	auto result = _dev->CreateCommandQueue(&cmdQDesc, IID_PPV_ARGS(&_cmdQue));

	// �E�B���h�E�T�C�Y�̎擾
	Size window = Application::Instance().GetWindowSize();

	// �X���b�v�`�F�C���p�̕ϐ��̍쐬
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = window.width; // �E�B���h�E�̕�
	swapChainDesc.Width = window.height; // �E�B���h�E�̍���
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = FALSE; // 3DS�I�ȕ\�������邩�ǂ����A��{��false
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0; 
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // �o�b�t�@�̎g�p�@(����͏o�͗p������Č����Ă�)
	swapChainDesc.BufferCount = 2; // �o�b�t�@�̐�(�\�Ɨ��œ񖇂Ȃ̂łQ)
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH; // �o�b�t�@�̃X�P�[�����O���@(����̂̓T�C�Y�𖳏����ň������΂����)
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // �o�b�t�@�̐؂�ւ����ɂǂ��؂�ւ��邩���w��A����̓h���C�o�ˑ��B
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED; // �A���t�@�l�̐ݒ�(����̓A���t�@�̎w��͂Ȃ�)
	swapChainDesc.Flags = 0; // �o�b�N�o�b�t�@����t�����g�o�b�t�@�ւ̈ڍs�̂Ƃ��̃I�v�V�������ݒ�ł���炵���B�悭�킩��񂩂�O

	// �X���b�v�`�F�C���{�̂̍쐬
	result = _dxgi->CreateSwapChainForHwnd(_cmdQue,
		_hwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)(&_swapchain));

	// �X���b�v�`�F�C�����\�[�X�Ё[��
	D3D12_DESCRIPTOR_HEAP_DESC swcHeapDesc = {};
	swcHeapDesc.NumDescriptors = 2;
	swcHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	swcHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	result = _dev->CreateDescriptorHeap(&swcHeapDesc, IID_PPV_ARGS(&_swcDescHeap));
	// �q�[�v�̐擪�n���h�����擾����
	D3D12_CPU_DESCRIPTOR_HANDLE swchandle = _swcDescHeap->GetCPUDescriptorHandleForHeapStart();

	// �X���b�v�`�F�C���̎擾
	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	_swapchain->GetDesc(&swcDesc);

	// �����_�[�^�[�Q�b�g�̐����擾
	int renderTargetsNum = swcDesc.BufferCount;

	// �擾���������_�[�^�[�Q�b�g�̐��������m�ۂ���
	renderTargets.resize(renderTargetsNum);

	// �f�X�N���v�^�̈������̃T�C�Y�̎擾
	int descriptorSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// �����_�[�^�[�Q�b�g�r���[�{�̂��o�b�t�@�̐����쐬����
	for (int i = 0; i < renderTargetsNum; i++)
	{
		result = _swapchain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
		if (FAILED(result))break;

		_dev->CreateRenderTargetView(renderTargets[i], nullptr, swchandle);
		swchandle.ptr += descriptorSize;
	}

	return result;
}

// �R�}���h���X�g�ƃR�}���h�A���P�[�^�̍쐬
HRESULT Dx12Wrapper::CreateCmdListAndAlloc()
{
	HRESULT result = S_OK;

	_dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAlloc));
	_dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAlloc, nullptr, IID_PPV_ARGS(&_cmdList));

	_cmdList->Close();

	return result;
}

// �t�F���X�̐���
HRESULT Dx12Wrapper::CreateFence()
{
	HRESULT result = S_OK;
	_fenceValue = 0;
	result = _dev->CreateFence(_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
	return result;
}

// WVP�p�̒萔�o�b�t�@�̍쐬
HRESULT Dx12Wrapper::CreateWVPConstantBuffer()
{
	// �f�X�N���v�^�q�[�v�쐬�p�̐ݒ�
	D3D12_DESCRIPTOR_HEAP_DESC wvpDesc = {};
	wvpDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	wvpDesc.NodeMask = 0;
	wvpDesc.NumDescriptors = 1;
	wvpDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	auto result = _dev->CreateDescriptorHeap(&wvpDesc,IID_PPV_ARGS(&_wvpDescHeap));
	auto wsize = Application::Instance().GetWindowSize();

	auto plane = XMFLOAT4(0, 1, 0, 0);//���ʂ̕�����
	_wvp.lightPos = XMFLOAT3(-100, 200, -100);// ���C�g�̍��W
	angle = {};

	lightTag = target;
	// ���C�g���璍���_�ւ̍s��̌v�Z
	XMMATRIX lightview = DirectX::XMMatrixLookAtLH(
		DirectX::XMLoadFloat3(&_wvp.lightPos),
		DirectX::XMLoadFloat3(&lightTag),
		DirectX::XMLoadFloat3(&up)
	);

	// ���ˉe�s��̎Z�o
	lightproj = XMMatrixOrthographicLH(100, 100, 1.f,1500.f);

	// ���[���h�s��̌v�Z
	_wvp.world = DirectX::XMMatrixRotationY(angle.y);

	// ���C�g����n�ʂւ̎ˉe�s��i�R�e�j�̌v�Z
	//_wvp.shadow = XMMatrixShadow(XMLoadFloat4(&plane), XMLoadFloat3(&_wvp.lightPos));

	// �r���[�s��̌v�Z�i���_���璍���_�ւ̍s��j
	_wvp.view = DirectX::XMMatrixLookAtLH(DirectX::XMLoadFloat3(&eye),DirectX::XMLoadFloat3(&target),DirectX::XMLoadFloat3(&up));

	// �v���W�F�N�V�����s��̌v�Z
	_wvp.projection = XMMatrixPerspectiveFovLH(XM_PIDIV4,static_cast<float>(wsize.width) / static_cast<float>(wsize.height),0.1f,1000.0f);

	// ���C�g�r���[�v���W�F�N�V�����̎Z�o
	_wvp.lvp = lightview * lightproj;

	// �T�C�Y�𒲐�
	size_t size = sizeof(_wvp);
	size = (size + 0xff)&~0xff;

	// �R���X�^���\�o�b�t�@�p�̃q�[�v�v���p�e�B
	D3D12_HEAP_PROPERTIES cbvHeapProp = {};
	cbvHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	cbvHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	cbvHeapProp.CreationNodeMask = 1;
	cbvHeapProp.VisibleNodeMask = 1;
	cbvHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

	// �R���X�^���g�o�b�t�@�̍쐬
	result = _dev->CreateCommittedResource(&cbvHeapProp,
		D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_wvpBuff));

	// Mapping
	result = _wvpBuff->Map(0,nullptr,(void**)&_wvpMP);
	std::memcpy(_wvpMP,&_wvp,sizeof(_wvp));

	// �R���X�^���g�o�b�t�@�p�̃f�X�N
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _wvpBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = static_cast<unsigned int>(size);

	auto handle = _wvpDescHeap->GetCPUDescriptorHandleForHeapStart();
	// �R���X�^���g�o�b�t�@�r���[�̍쐬
	_dev->CreateConstantBufferView(&cbvDesc, handle);

	return result;
}

// �[�x�X�e���V���r���[�̍쐬
HRESULT Dx12Wrapper::CreateDepthStencilView()
{
	Size wsize = Application::Instance().GetWindowSize();

	D3D12_DESCRIPTOR_HEAP_DESC _dsvHeapDesc = {};
	_dsvHeapDesc.NumDescriptors = 1;
	_dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	auto result = _dev->CreateDescriptorHeap(&_dsvHeapDesc, IID_PPV_ARGS(&_dsvDescHeap));
	result = _dev->CreateDescriptorHeap(&_dsvHeapDesc, IID_PPV_ARGS(&_lightDescHeap));

	_dsvHeapDesc.NumDescriptors = 1;
	_dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	_dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	// �V�F�[�_�[���\�[�X�f�X�N���v�^�q�[�v�̍쐬
	result = _dev->CreateDescriptorHeap(&_dsvHeapDesc, IID_PPV_ARGS(&_dsvSrvHeap));
	result = _dev->CreateDescriptorHeap(&_dsvHeapDesc, IID_PPV_ARGS(&_lightSrvHeap));

	// DSV�p�̃��\�[�X�f�X�N
	D3D12_RESOURCE_DESC depthResDesc = {};
	depthResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthResDesc.Width = wsize.width;
	depthResDesc.Height = wsize.height;
	depthResDesc.DepthOrArraySize = 1;
	depthResDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	depthResDesc.SampleDesc.Count = 1;
	depthResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	// DSV�p�̃q�[�v�v���b�v
	D3D12_HEAP_PROPERTIES depthHeapProp = {};
	depthHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	depthHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	depthHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	//�N���A�o�����[
	D3D12_CLEAR_VALUE _depthClearValue = {};
	_depthClearValue.DepthStencil.Depth = 1.0f;
	_depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;

	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&_depthClearValue,
		IID_PPV_ARGS(&_depthBuffer));
	
	depthResDesc.Width = wsize.width*2;
	depthResDesc.Height = wsize.height*2;

	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&_depthClearValue,
		IID_PPV_ARGS(&_lightBuffer));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvdec = {};
	dsvdec.Format = DXGI_FORMAT_D32_FLOAT;
	dsvdec.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvdec.Texture2D.MipSlice = 0;
	dsvdec.Flags = D3D12_DSV_FLAG_NONE;
	
	auto heapstart = _dsvDescHeap->GetCPUDescriptorHandleForHeapStart();

	_dev->CreateDepthStencilView(_depthBuffer, &dsvdec, heapstart);

	heapstart = _lightDescHeap->GetCPUDescriptorHandleForHeapStart();

	_dev->CreateDepthStencilView(_lightBuffer, &dsvdec, heapstart);// ���C�g���猩���[�x���������ރq�[�v

	
	// �V�F�[�_�[���\�[�X�r���[�̍쐬
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	auto srvstart = _dsvSrvHeap->GetCPUDescriptorHandleForHeapStart();

	_dev->CreateShaderResourceView(_depthBuffer, &srvDesc, srvstart);

	srvstart = _lightSrvHeap->GetCPUDescriptorHandleForHeapStart();

	_dev->CreateShaderResourceView(_lightBuffer, &srvDesc, srvstart);

	return result;
}

// �҂����s���֐�
void Dx12Wrapper::WaitWithFence()
{
	int cnt = 0;
	while (_fence->GetCompletedValue() != _fenceValue){
		//�т��[���
	};
}

// �R�}���h�̒ǉ��ƃV�O�i���̑��M���s��
void Dx12Wrapper::ExecuteCommand()
{
	_cmdQue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&_cmdList);
	_cmdQue->Signal(_fence, ++_fenceValue);
}

HRESULT Dx12Wrapper::CreateShrinkBuffer()
{
	Size wsize = Application::Instance().GetWindowSize();

	D3D12_RESOURCE_DESC ResDesc = {};
	ResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResDesc.Width = wsize.width;
	ResDesc.Height = wsize.height;
	ResDesc.DepthOrArraySize = 1;
	ResDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	ResDesc.SampleDesc.Count = 1;
	ResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE clear = { DXGI_FORMAT_R8G8B8A8_UNORM ,{0.1f,0.1f,0.1f,0.0f}, };

	// �o�b�t�@�쐬
	auto result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&ResDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clear,
		IID_PPV_ARGS(&_shrinkBuffer)
	);

	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = 1;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	// �����_�[�^�[�Q�b�g�f�X�N���v�^�q�[�v�̍쐬
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&_shrinkRtvHeap));

	descHeapDesc.NumDescriptors = 1;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;// �^�C�v�ύX
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	// �V�F�[�_�[���\�[�X�f�X�N���v�^�q�[�v�̍쐬
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&_shrinkSrvHeap));

	_dev->CreateRenderTargetView(_shrinkBuffer,nullptr, _shrinkRtvHeap->GetCPUDescriptorHandleForHeapStart());

	// �V�F�[�_�[���\�[�X�r���[�̍쐬
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	_dev->CreateShaderResourceView(_shrinkBuffer, &srvDesc, _shrinkSrvHeap->GetCPUDescriptorHandleForHeapStart());
	return result;
}

void Dx12Wrapper::DrawToShrinkBuffer()
{
	float col[4] = { 0.1f,0.1f,0.1f,0.0 };
	auto handle = _shrinkRtvHeap->GetCPUDescriptorHandleForHeapStart();

	_cmdAlloc->Reset();
	_cmdList->Reset(_cmdAlloc, nullptr);

	// �����_�[�^�[�Q�b�g�w��
	_cmdList->OMSetRenderTargets(1,&handle, false,nullptr);
	_cmdList->ClearRenderTargetView(handle, col,0,nullptr);// �N���A

	// �V�O�l�`���ƃp�C�v���C���̐ݒ�
	_cmdList->SetGraphicsRootSignature(_peraSignature);
	_cmdList->SetPipelineState(_shrinkPipeline);

	// ���_�o�b�t�@�ƃg�|���W�̐ݒ�
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	_cmdList->IASetVertexBuffers(0, 1, &_peravbView);

	auto start = _srvDescHeap->GetGPUDescriptorHandleForHeapStart();
	_cmdList->SetDescriptorHeaps(1, &_srvDescHeap);
	start.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)*2;
	_cmdList->SetGraphicsRootDescriptorTable(0, start);

	// �r���[�|�[�g�̐ݒ�
	D3D12_VIEWPORT vp = {};
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width = static_cast<float>(Application::Instance().GetWindowSize().width);
	vp.Height = static_cast<float>(Application::Instance().GetWindowSize().height)/2;
	vp.MaxDepth = 1.0f;
	vp.MinDepth = 0.0f;

	// �V�U�[�̐ݒ�
	D3D12_RECT sr = {};
	sr.left = 0;
	sr.top = 0;
	sr.right = vp.Width;
	sr.bottom = vp.Height;

	for (int i = 0; i < 8; ++i)
	{
		_cmdList->RSSetViewports(1,&vp);
		_cmdList->RSSetScissorRects(1,&sr);
		_cmdList->DrawInstanced(4, 1, 0, 0);

		vp.TopLeftY += vp.Height;
		sr.top = vp.TopLeftY;
		vp.Height /= 2;
		vp.Width /= 2;
		sr.bottom = sr.top + vp.Height;
	}

	_cmdList->Close();
}

HRESULT Dx12Wrapper::CreateShrinkPipline()
{
	auto result = D3DCompileFromFile(L"Shrink.hlsl", nullptr, nullptr, "shrinkVS", "vs_5_0", D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION, 0, &peraVertShader, nullptr);

	// �s�N�Z���V�F�[�_
	result = D3DCompileFromFile(L"Shrink.hlsl", nullptr, nullptr, "shrinkPS", "ps_5_0", D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION, 0, &peraPixShader, nullptr);

	D3D12_INPUT_ELEMENT_DESC peraLayoutDescs[] =
	{
		// Pos
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0, D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		// UV
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};

	// �p�C�v���C������邽�߂�GPS�̕ϐ��̍쐬
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};

	//���[�g�V�O�l�`���ƒ��_���C�A�E�g
	gpsDesc.pRootSignature = _peraSignature;
	gpsDesc.InputLayout.pInputElementDescs = peraLayoutDescs;
	gpsDesc.InputLayout.NumElements = _countof(peraLayoutDescs);

	//�V�F�[�_�̃Z�b�g
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(peraVertShader);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(peraPixShader);

	//�����_�[�^�[�Q�b�g
	gpsDesc.NumRenderTargets = 1;
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	//�[�x�X�e���V��
	gpsDesc.DepthStencilState.DepthEnable = false;
	gpsDesc.DepthStencilState.StencilEnable = false;

	//���X�^���C�U
	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	//���̑�
	gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.NodeMask = 0;
	gpsDesc.SampleDesc.Count = 1;
	gpsDesc.SampleDesc.Quality = 0;
	gpsDesc.SampleMask = 0xffffffff;
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//�O�p�`

	result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&_shrinkPipeline));

	return result;
}

HRESULT Dx12Wrapper::CreateSceneBuffer()
{
	Size wsize = Application::Instance().GetWindowSize();

	D3D12_RESOURCE_DESC ResDesc = {};
	ResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResDesc.Width = wsize.width;
	ResDesc.Height = wsize.height;
	ResDesc.DepthOrArraySize = 1;
	ResDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	ResDesc.SampleDesc.Count = 1;
	ResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE clear = { DXGI_FORMAT_R8G8B8A8_UNORM ,{0.1f,0.1f,0.1f,0.0f}, };

	// �o�b�t�@�쐬
	auto result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&ResDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clear,
		IID_PPV_ARGS(&_sceneBuffer)
	);

	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = 1;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	// �łՂ��f�X�N���v�^�q�[�v�̍쐬
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&_sceneRtvHeap));

	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;// �^�C�v�ύX
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	// �V�F�[�_�[���\�[�X�f�X�N���v�^�q�[�v�̍쐬
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&_sceneSrvHeap));

	_dev->CreateRenderTargetView(_sceneBuffer, nullptr, _sceneRtvHeap->GetCPUDescriptorHandleForHeapStart());

	// �V�F�[�_�[���\�[�X�r���[�̍쐬
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	_dev->CreateShaderResourceView(_sceneBuffer, &srvDesc, _sceneSrvHeap->GetCPUDescriptorHandleForHeapStart());
	return result;
}

void Dx12Wrapper::DrawToSceneBuffer()
{
	float col[4] = { 0.1f,0.1f,0.1f,0.0 };

	_cmdAlloc->Reset();
	_cmdList->Reset(_cmdAlloc, nullptr);
	auto rtvStart = _sceneRtvHeap->GetCPUDescriptorHandleForHeapStart();

	_cmdList->OMSetRenderTargets(1, &rtvStart,false,nullptr);
	_cmdList->ClearRenderTargetView(rtvStart,col,0,nullptr);

	// �V�O�l�`���ƃp�C�v���C���̐ݒ�
	_cmdList->SetGraphicsRootSignature(_peraSignature);
	_cmdList->SetPipelineState(_scenePipeline);

	// ���_�o�b�t�@�ƃg�|���W�̐ݒ�
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	_cmdList->IASetVertexBuffers(0, 1, &_peravbView);

	auto start = _srvDescHeap->GetGPUDescriptorHandleForHeapStart();
	_cmdList->SetDescriptorHeaps(1, &_srvDescHeap);
	_cmdList->SetGraphicsRootDescriptorTable(0, start);

	// �r���[�|�[�g�̐ݒ�
	D3D12_VIEWPORT vp = {};
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width = static_cast<float>(Application::Instance().GetWindowSize().width);
	vp.Height = static_cast<float>(Application::Instance().GetWindowSize().height) / 2;
	vp.MaxDepth = 1.0f;
	vp.MinDepth = 0.0f;

	// �V�U�[�̐ݒ�
	D3D12_RECT sr = {};
	sr.left = 0;
	sr.top = 0;
	sr.right = vp.Width;
	sr.bottom = vp.Height;

	for (int i = 0; i < 8; ++i)
	{
		_cmdList->RSSetViewports(1, &vp);
		_cmdList->RSSetScissorRects(1, &sr);
		_cmdList->DrawInstanced(4, 1, 0, 0);

		vp.TopLeftY += vp.Height;
		sr.top = vp.TopLeftY;
		vp.Height /= 2;
		vp.Width /= 2;
		sr.bottom = sr.top + vp.Height;
	}

	_cmdList->Close();
}

HRESULT Dx12Wrapper::CreateScenePipline()
{
	auto result = D3DCompileFromFile(L"scene.hlsl", nullptr, nullptr, "sceneVS", "vs_5_0", D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION, 0, &peraVertShader, nullptr);

	// �s�N�Z���V�F�[�_
	result = D3DCompileFromFile(L"scene.hlsl", nullptr, nullptr, "scenePS", "ps_5_0", D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION, 0, &peraPixShader, nullptr);

	D3D12_INPUT_ELEMENT_DESC peraLayoutDescs[] =
	{
		// Pos
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0, D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		// UV
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};

	// �p�C�v���C������邽�߂�GPS�̕ϐ��̍쐬
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};

	//���[�g�V�O�l�`���ƒ��_���C�A�E�g
	gpsDesc.pRootSignature = _peraSignature;
	gpsDesc.InputLayout.pInputElementDescs = peraLayoutDescs;
	gpsDesc.InputLayout.NumElements = _countof(peraLayoutDescs);

	//�V�F�[�_�̃Z�b�g
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(peraVertShader);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(peraPixShader);

	//�����_�[�^�[�Q�b�g
	gpsDesc.NumRenderTargets = 1;
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	//�[�x�X�e���V��
	gpsDesc.DepthStencilState.DepthEnable = false;
	gpsDesc.DepthStencilState.StencilEnable = false;

	//���X�^���C�U
	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	//���̑�
	gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.NodeMask = 0;
	gpsDesc.SampleDesc.Count = 1;
	gpsDesc.SampleDesc.Quality = 0;
	gpsDesc.SampleMask = 0xffffffff;
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//�O�p�`

	result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&_scenePipeline));

	return result;
}

HRESULT Dx12Wrapper::CreateSSAOPS()
{
	ID3DBlob* SSAOVS = nullptr;
	ID3DBlob* SSAOPS = nullptr;

	auto result = D3DCompileFromFile(L"PeraShader.hlsl", nullptr, nullptr, "peraVS", "vs_5_0", D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION, 0, &SSAOVS, nullptr);

	// �s�N�Z���V�F�[�_
	result = D3DCompileFromFile(L"SSAO.hlsl", nullptr, nullptr, "SSAOPS", "ps_5_0", D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION, 0, &SSAOPS, nullptr);

	D3D12_INPUT_ELEMENT_DESC peraLayoutDescs[] =
	{
		// Pos
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0, D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		// UV
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};

	// �p�C�v���C������邽�߂�GPS�̕ϐ��̍쐬
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};

	//���[�g�V�O�l�`���ƒ��_���C�A�E�g
	gpsDesc.pRootSignature = _peraSignature;
	gpsDesc.InputLayout.pInputElementDescs = peraLayoutDescs;
	gpsDesc.InputLayout.NumElements = _countof(peraLayoutDescs);

	//�V�F�[�_�̃Z�b�g
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(SSAOVS);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(SSAOPS);

	//�����_�[�^�[�Q�b�g
	gpsDesc.NumRenderTargets = 1;
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	//�[�x�X�e���V��
	gpsDesc.DepthStencilState.DepthEnable = false;
	gpsDesc.DepthStencilState.StencilEnable = false;

	//���X�^���C�U
	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	//���̑�
	gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.NodeMask = 0;
	gpsDesc.SampleDesc.Count = 1;
	gpsDesc.SampleDesc.Quality = 0;
	gpsDesc.SampleMask = 0xffffffff;
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//�O�p�`

	result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&_SSAOPipeline));

	return result;
}

HRESULT Dx12Wrapper::CreateSSAOBuffer()
{
	Size wsize = Application::Instance().GetWindowSize();

	D3D12_RESOURCE_DESC ResDesc = {};
	ResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	ResDesc.Width = wsize.width;
	ResDesc.Height = wsize.height;
	ResDesc.DepthOrArraySize = 1;
	ResDesc.Format = DXGI_FORMAT_R32_FLOAT;
	ResDesc.SampleDesc.Count = 1;
	ResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE clear = { DXGI_FORMAT_R32_FLOAT ,{1,1,1,1}, };

	// �o�b�t�@�쐬
	auto result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&ResDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clear,
		IID_PPV_ARGS(&_SSAOBuffer)
	);

	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = 1;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	// �����_�[�^�[�Q�b�g�f�X�N���v�^�q�[�v�̍쐬
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&_SSAORtv));

	descHeapDesc.NumDescriptors = 1;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;// �^�C�v�ύX
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	// �V�F�[�_�[���\�[�X�f�X�N���v�^�q�[�v�̍쐬
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&_SSAOSrv));

	_dev->CreateRenderTargetView(_SSAOBuffer, nullptr, _SSAORtv->GetCPUDescriptorHandleForHeapStart());

	// �V�F�[�_�[���\�[�X�r���[�̍쐬
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	_dev->CreateShaderResourceView(_SSAOBuffer, &srvDesc, _SSAOSrv->GetCPUDescriptorHandleForHeapStart());
	return result;
}

void Dx12Wrapper::DrawToSSAO()
{
	_cmdAlloc->Reset();
	_cmdList->Reset(_cmdAlloc, nullptr);
	auto rtvStart = _SSAORtv->GetCPUDescriptorHandleForHeapStart();

	_cmdList->OMSetRenderTargets(1, &rtvStart, false, nullptr);

	// �V�O�l�`���ƃp�C�v���C���̐ݒ�
	_cmdList->SetGraphicsRootSignature(_peraSignature);
	_cmdList->SetPipelineState(_SSAOPipeline);

	// ���_�o�b�t�@�ƃg�|���W�̐ݒ�
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	_cmdList->IASetVertexBuffers(0, 1, &_peravbView);

	// �[�x�l
	auto dstart = _dsvSrvHeap->GetGPUDescriptorHandleForHeapStart();
	_cmdList->SetDescriptorHeaps(1, &_dsvSrvHeap);
	_cmdList->SetGraphicsRootDescriptorTable(1, dstart);

	auto srvStart = _srvDescHeap->GetGPUDescriptorHandleForHeapStart();
	_cmdList->SetDescriptorHeaps(1, &_srvDescHeap);
	srvStart.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	_cmdList->SetGraphicsRootDescriptorTable(3, srvStart);

	// �r���[�|�[�g�̐ݒ�
	D3D12_VIEWPORT vp = {};
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width = static_cast<float>(Application::Instance().GetWindowSize().width);
	vp.Height = static_cast<float>(Application::Instance().GetWindowSize().height);
	vp.MaxDepth = 1.0f;
	vp.MinDepth = 0.0f;

	// �V�U�[�̐ݒ�
	D3D12_RECT sr = {};
	sr.left = 0;
	sr.top = 0;
	sr.right = vp.Width;
	sr.bottom = vp.Height;

	_cmdList->RSSetViewports(1, &vp);
	_cmdList->RSSetScissorRects(1, &sr);
	_cmdList->DrawInstanced(4, 1, 0, 0);

	_cmdList->Close();
}


HRESULT Dx12Wrapper::CreateFlagsBuffer()
{
	flags = {true,false};
	// �f�X�N���v�^�q�[�v�쐬�p�̐ݒ�
	D3D12_DESCRIPTOR_HEAP_DESC colDesc = {};
	colDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	colDesc.NodeMask = 0;
	colDesc.NumDescriptors = 1;
	colDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	auto result = _dev->CreateDescriptorHeap(&colDesc, IID_PPV_ARGS(&_flagsHeap));

	// �R���X�^���\�o�b�t�@�p�̃q�[�v�v���p�e�B
	D3D12_HEAP_PROPERTIES cbvHeapProp = {};
	cbvHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	cbvHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	cbvHeapProp.CreationNodeMask = 1;
	cbvHeapProp.VisibleNodeMask = 1;
	cbvHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

	auto size = sizeof(flags);
	size = (size + 0xff)&~0xff;

	// �R���X�^���g�o�b�t�@�̍쐬
	result = _dev->CreateCommittedResource(&cbvHeapProp,
		D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_flagsBuffer));

	// �R���X�^���g�o�b�t�@�p�̃f�X�N
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _flagsBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = static_cast<unsigned int>(size);

	auto handle = _flagsHeap->GetCPUDescriptorHandleForHeapStart();
	// �R���X�^���g�o�b�t�@�r���[�̍쐬
	_dev->CreateConstantBufferView(&cbvDesc, handle);

	// Mapping
	result = _flagsBuffer->Map(0, nullptr, (void**)&MapFlags);
	std::memcpy(MapFlags, &flags, sizeof(flags));

	return result;
}

HRESULT Dx12Wrapper::CreateColBuffer()
{
	// �f�X�N���v�^�q�[�v�쐬�p�̐ݒ�
	D3D12_DESCRIPTOR_HEAP_DESC colDesc = {};
	colDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	colDesc.NodeMask = 0;
	colDesc.NumDescriptors = 1;
	colDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	auto result = _dev->CreateDescriptorHeap(&colDesc, IID_PPV_ARGS(&_colDescHeap));

	// �R���X�^���\�o�b�t�@�p�̃q�[�v�v���p�e�B
	D3D12_HEAP_PROPERTIES cbvHeapProp = {};
	cbvHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	cbvHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	cbvHeapProp.CreationNodeMask = 1;
	cbvHeapProp.VisibleNodeMask = 1;
	cbvHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

	bloomCol.bloom[0]= bloomCol.bloom[1]=bloomCol.bloom[2] = 1;

	auto size = sizeof(bloomCol);
	size = (size + 0xff)&~0xff;

	// �R���X�^���g�o�b�t�@�̍쐬
	result = _dev->CreateCommittedResource(&cbvHeapProp,
		D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_colorBuffer));

	// �R���X�^���g�o�b�t�@�p�̃f�X�N
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _colorBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = static_cast<unsigned int>(size);

	auto handle = _colDescHeap->GetCPUDescriptorHandleForHeapStart();
	// �R���X�^���g�o�b�t�@�r���[�̍쐬
	_dev->CreateConstantBufferView(&cbvDesc, handle);
	
	// Mapping
	result = _colorBuffer->Map(0, nullptr, (void**)&MapCol);
	std::memcpy(MapCol, &bloomCol, sizeof(bloomCol));

	return result;
}

void Dx12Wrapper::SetEfkRenderer()
{
	auto wsize = Application::Instance().GetWindowSize();
	// �G�t�F�N�V�A�̃����_�������
	DXGI_FORMAT bbFormat = DXGI_FORMAT_R8G8B8A8_UNORM;// �o�b�N�o�b�t�@�̃t�H�[�}�b�g
	_efkRenderer = EffekseerRendererDX12::Create(
		_dev,// �f�o�C�X
		_cmdQue,// �R�}���h�L���[
		2,// �o�b�N�o�b�t�@�̐�
		&bbFormat,// �o�b�N�o�b�t�@�̃t�H�[�}�b�g
		1,// �����_�[�^�[�Q�b�g�̐�
		true,// �f�v�X���肩
		false,// ���΃f�v�X���肩
		2000// �p�[�e�B�N����
	);

	_efkManager = Effekseer::Manager::Create(2000);

	_efkManager->SetCoordinateSystem(Effekseer::CoordinateSystem::LH);

	// �`��p�C���X�^���X����`��@�\��ݒ�
	_efkManager->SetSpriteRenderer(_efkRenderer->CreateSpriteRenderer());// �X�v���C�g
	_efkManager->SetRibbonRenderer(_efkRenderer->CreateRibbonRenderer());// ���{���ۂ����
	_efkManager->SetRingRenderer(_efkRenderer->CreateRingRenderer());// �����O
	_efkManager->SetTrackRenderer(_efkRenderer->CreateTrackRenderer());// �O��(�g���b�L���O)
	_efkManager->SetModelRenderer(_efkRenderer->CreateModelRenderer());// ���f��

	// �`��p�C���X�^���X����e�N�X�`���̓Ǎ��@�\��ݒ�
	_efkManager->SetTextureLoader(_efkRenderer->CreateTextureLoader());
	_efkManager->SetModelLoader(_efkRenderer->CreateModelLoader());

	// �G�t�F�N�g�����ʒu��ݒ�
	auto efkPos = ::Effekseer::Vector3D(0.0f, 0.0f, 0.0f);

	//�������v�[��
	_efkMemoryPool = EffekseerRendererDX12::CreateSingleFrameMemoryPool(_efkRenderer);

	//�R�}���h���X�g�쐬
	_efkCmdList = EffekseerRendererDX12::CreateCommandList(_efkRenderer, _efkMemoryPool);

	//�R�}���h���X�g�Z�b�g
	_efkRenderer->SetCommandList(_efkCmdList);

	// ���e�s���ݒ�
	_efkRenderer->SetProjectionMatrix(
		Effekseer::Matrix44().PerspectiveFovRH(45.0f / 180.0f * 3.14f, wsize.width / wsize.height, 1.0f, 1000.0f)
	);

	// �J�����s���ݒ�
	_efkRenderer->SetCameraMatrix(
		Effekseer::Matrix44().LookAtRH(
			Effekseer::Vector3D(eye.x, eye.y, eye.z),
			Effekseer::Vector3D(target.x, target.y, target.z),
			Effekseer::Vector3D(up.x, up.y, up.z)
		)
	);

	// �G�t�F�N�g�̓Ǎ�
	_effect = Effekseer::Effect::Create(_efkManager, (const EFK_CHAR*)L"effect/�a��.efk");
}


/* �|���S���n�֐�(���ƂŃN���X�ɕ�����\��) */
// �|���p�̒��_�o�b�t�@�쐬
HRESULT Dx12Wrapper::CreatePolygonVertexBuffer()
{
	Vertex vertices[] = {
		XMFLOAT3(-1,-1,0),XMFLOAT2(0,1),//����
		XMFLOAT3(-1,1,0),XMFLOAT2(0,0),//����
		XMFLOAT3(1,-1,0),XMFLOAT2(1,1),//�E��
		XMFLOAT3(1,1,0),XMFLOAT2(1,0),//�E��
	};

	//�y���o�b�t�@����
	auto result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_peraVertBuff)
	);	Vertex* mapver = nullptr;	result = _peraVertBuff->Map(0, nullptr, (void**)&mapver);
	std::copy(std::begin(vertices), std::end(vertices), mapver);	_peraVertBuff->Unmap(0, nullptr);	_peravbView.BufferLocation = _peraVertBuff->GetGPUVirtualAddress();
	_peravbView.SizeInBytes = sizeof(vertices);
	_peravbView.StrideInBytes = sizeof(Vertex);

	return result;
}

// �ꖇ�ڃ|���S���̍쐬
HRESULT Dx12Wrapper::CreateFirstPolygon()
{
	_peraBuffers.resize(4);
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = _peraBuffers.size();
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	// �����_�[�^�[�Q�b�g�f�X�N���v�^�q�[�v�̍쐬
	auto result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&_rtvDescHeap));

	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;// �^�C�v�ύX

	// �V�F�[�_�[���\�[�X�f�X�N���v�^�q�[�v�̍쐬
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&_srvDescHeap));

	// ���\�[�X�̍쐬
	D3D12_RESOURCE_DESC desc = {};
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = Application::Instance().GetWindowSize().width;
	desc.Height = Application::Instance().GetWindowSize().height;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.DepthOrArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.MipLevels = 1;
	
	D3D12_CLEAR_VALUE clear = { DXGI_FORMAT_R8G8B8A8_UNORM ,{0.01f,0.01f,0.01f,0.0f}, };
	auto rtvstart = _rtvDescHeap->GetCPUDescriptorHandleForHeapStart();
	auto srvstart = _srvDescHeap->GetCPUDescriptorHandleForHeapStart();
	for (auto& pb: _peraBuffers)
	{
		result = _dev->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			&clear,
			IID_PPV_ARGS(&pb));
	
		// �����_�[�^�[�Q�b�g�r���[�̍쐬
		_dev->CreateRenderTargetView(pb, nullptr, rtvstart);
		rtvstart.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// �V�F�[�_�[���\�[�X�r���[�̍쐬
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = desc.Format;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		_dev->CreateShaderResourceView(pb, &srvDesc, srvstart);
		srvstart.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	return result;
}

// �񖇖ڃ|���S���̍쐬
HRESULT Dx12Wrapper::CreateSecondPolygon()
{
	// �f�X�N���v�^�q�[�v�쐬�p�̏��\����
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = 1;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	// �����_�[�^�[�Q�b�g�f�X�N���v�^�q�[�v�̍쐬
	auto result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&_rtvDescHeap2));

	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NumDescriptors = 1;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;// �^�C�v�ύX

	// �V�F�[�_�[���\�[�X�f�X�N���v�^�q�[�v�̍쐬
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&_srvDescHeap2));

	// ���\�[�X�̍쐬�p���\����
	D3D12_RESOURCE_DESC desc = {};
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = Application::Instance().GetWindowSize().width;
	desc.Height = Application::Instance().GetWindowSize().height;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.DepthOrArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.MipLevels = 1;

	// �N���A�o�����[
	D3D12_CLEAR_VALUE clear = { DXGI_FORMAT_R8G8B8A8_UNORM ,{0.01f,0.01f,0.01f,0.0f}, };

	// ���\�[�X�쐬
	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clear,
		IID_PPV_ARGS(&_peraBuffer2));

	// �����_�[�^�[�Q�b�g�r���[�̍쐬
	_dev->CreateRenderTargetView(_peraBuffer2, nullptr, _rtvDescHeap2->GetCPUDescriptorHandleForHeapStart());

	// �V�F�[�_�[���\�[�X�r���[�̍쐬�p���\����
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = desc.Format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	// �V�F�[�_�[���\�[�X�r���[�̍쐬
	_dev->CreateShaderResourceView(_peraBuffer2, &srvDesc, _srvDescHeap2->GetCPUDescriptorHandleForHeapStart());
	return result;
}

// �ꖇ�ڃ|���S���p�̃��[�g�V�O�l�`���̍쐬
HRESULT Dx12Wrapper::CreateFirstSignature()
{
	// �T���v���̐ݒ�
	D3D12_STATIC_SAMPLER_DESC SamplerDesc[1] = {};
	SamplerDesc[0].Filter = D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR;// ���`�⊮����
	SamplerDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;// �悪�J��Ԃ��`�悳���
	SamplerDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	SamplerDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	SamplerDesc[0].MaxLOD = D3D12_FLOAT32_MAX;// ����Ȃ�
	SamplerDesc[0].MinLOD = 0.0f;// �����Ȃ�
	SamplerDesc[0].MipLODBias = 0.0f;// MIPMAP�̃o�C�A�X
	SamplerDesc[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;// �G�b�W�̐F(��)
	SamplerDesc[0].ShaderRegister = 0;// �g�p���郌�W�X�^�X���b�g
	SamplerDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;// �ǂ̂��炢�̃f�[�^���V�F�[�_�Ɍ����邩(�S��)
	SamplerDesc[0].RegisterSpace = 0;// �킩���
	SamplerDesc[0].MaxAnisotropy = 0;// Filter��Anisotropy�̂Ƃ��̂ݗL��

	// �����W�̐ݒ�
	D3D12_DESCRIPTOR_RANGE descRange[11] = {};// �ꖇ�|���S��
	descRange[0].NumDescriptors = 1;
	descRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;// �V�F�[�_�[���\�[�X
	descRange[0].BaseShaderRegister = 0;//���W�X�^�ԍ�
	descRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// �[�x
	descRange[1].NumDescriptors = 1;
	descRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;// �V�F�[�_�[���\�[�X
	descRange[1].BaseShaderRegister = 1;//���W�X�^�ԍ�
	descRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ���C�g����̐[�x
	descRange[2].NumDescriptors = 1;
	descRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;// �V�F�[�_�[���\�[�X
	descRange[2].BaseShaderRegister = 2;//���W�X�^�ԍ�
	descRange[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// housenn
	descRange[3].NumDescriptors = 1;
	descRange[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;// �V�F�[�_�[���\�[�X
	descRange[3].BaseShaderRegister = 3;//���W�X�^�ԍ�
	descRange[3].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// bloom�p
	descRange[4].NumDescriptors = 1;
	descRange[4].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;// �V�F�[�_�[���\�[�X
	descRange[4].BaseShaderRegister = 4;//���W�X�^�ԍ�
	descRange[4].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// shrink�p
	descRange[5].NumDescriptors = 1;
	descRange[5].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;// �V�F�[�_�[���\�[�X
	descRange[5].BaseShaderRegister = 5;//���W�X�^�ԍ�
	descRange[5].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	
	descRange[6].NumDescriptors = 1;
	descRange[6].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;// �J���[�l
	descRange[6].BaseShaderRegister = 0;//���W�X�^�ԍ�
	descRange[6].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// �e��t���O
	descRange[7].NumDescriptors = 1;
	descRange[7].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;// �t���O
	descRange[7].BaseShaderRegister = 1;//���W�X�^�ԍ�
	descRange[7].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// 
	descRange[8].NumDescriptors = 1;
	descRange[8].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;// tex
	descRange[8].BaseShaderRegister = 6;//���W�X�^�ԍ�
	descRange[8].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// �k���e�N�X�`��
	descRange[9].NumDescriptors = 1;
	descRange[9].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;// tex
	descRange[9].BaseShaderRegister = 7;//���W�X�^�ԍ�
	descRange[9].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// SSAO
	descRange[10].NumDescriptors = 1;
	descRange[10].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;// SSAotex
	descRange[10].BaseShaderRegister = 8;//���W�X�^�ԍ�
	descRange[10].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ���[�g�p�����[�^�ϐ��̐ݒ�
	D3D12_ROOT_PARAMETER rootParam[11] = {};
	rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[0].DescriptorTable.pDescriptorRanges = &descRange[0];//�Ή����郌���W�ւ̃|�C���^
	rootParam[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//���ׂẴV�F�[�_����Q��

	// �J��������̐[�x
	rootParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[1].DescriptorTable.pDescriptorRanges = &descRange[1];//�Ή����郌���W�ւ̃|�C���^
	rootParam[1].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//���ׂẴV�F�[�_����Q��

	// ���C�g����̐[�x
	rootParam[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[2].DescriptorTable.pDescriptorRanges = &descRange[2];//�Ή����郌���W�ւ̃|�C���^
	rootParam[2].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//���ׂẴV�F�[�_����Q��

	// �@��
	rootParam[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[3].DescriptorTable.pDescriptorRanges = &descRange[3];//�Ή����郌���W�ւ̃|�C���^
	rootParam[3].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//���ׂẴV�F�[�_����Q��

	// buru-mu
	rootParam[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[4].DescriptorTable.pDescriptorRanges = &descRange[4];//�Ή����郌���W�ւ̃|�C���^
	rootParam[4].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//���ׂẴV�F�[�_����Q��

	// �V�������N
	rootParam[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[5].DescriptorTable.pDescriptorRanges = &descRange[5];//�Ή����郌���W�ւ̃|�C���^
	rootParam[5].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//���ׂẴV�F�[�_����Q��

	// �J���[
	rootParam[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[6].DescriptorTable.pDescriptorRanges = &descRange[6];//�Ή����郌���W�ւ̃|�C���^
	rootParam[6].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//���ׂẴV�F�[�_����Q��
																
	// �e��t���O
	rootParam[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[7].DescriptorTable.pDescriptorRanges = &descRange[7];//�Ή����郌���W�ւ̃|�C���^
	rootParam[7].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//���ׂẴV�F�[�_����Q��

	// �k���Ă�����
	rootParam[8].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[8].DescriptorTable.pDescriptorRanges = &descRange[8];//�Ή����郌���W�ւ̃|�C���^
	rootParam[8].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[8].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//���ׂẴV�F�[�_����Q��

	// �k���Ă�����
	rootParam[9].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[9].DescriptorTable.pDescriptorRanges = &descRange[9];//�Ή����郌���W�ւ̃|�C���^
	rootParam[9].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[9].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//���ׂẴV�F�[�_����Q��

	// SSAO
	rootParam[10].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[10].DescriptorTable.pDescriptorRanges = &descRange[10];//�Ή����郌���W�ւ̃|�C���^
	rootParam[10].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[10].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//���ׂẴV�F�[�_����Q��

	// ���[�g�V�O�l�`������邽�߂̕ϐ��̐ݒ�
	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.pStaticSamplers = SamplerDesc;
	rsd.pParameters = rootParam;
	rsd.NumParameters = 11;
	rsd.NumStaticSamplers = 1;

	auto result = D3D12SerializeRootSignature(
		&rsd,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&signature,
		&error);

	// ���[�g�V�O�l�`���{�̂̍쐬
	result = _dev->CreateRootSignature(
		0,
		signature->GetBufferPointer(),
		signature->GetBufferSize(),
		IID_PPV_ARGS(&_peraSignature));

	return result;
}

// �ꖇ�ڃ|���S���p�̃p�C�v���C���̍쐬
HRESULT Dx12Wrapper::CreateFirstPopelineState()
{
	auto result = D3DCompileFromFile(L"PeraShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "peraVS", "vs_5_0", D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION, 0, &peraVertShader, nullptr);

	// �s�N�Z���V�F�[�_
	result = D3DCompileFromFile(L"PeraShader.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "peraPS", "ps_5_0", D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION, 0, &peraPixShader, nullptr);

	D3D12_INPUT_ELEMENT_DESC peraLayoutDescs[] =
	{
		// Pos
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0, D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		// UV
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};

	// �p�C�v���C������邽�߂�GPS�̕ϐ��̍쐬
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};

	//���[�g�V�O�l�`���ƒ��_���C�A�E�g
	gpsDesc.pRootSignature = _peraSignature;
	gpsDesc.InputLayout.pInputElementDescs = peraLayoutDescs;
	gpsDesc.InputLayout.NumElements = _countof(peraLayoutDescs);

	//�V�F�[�_�̃Z�b�g
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(peraVertShader);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(peraPixShader);

	//�����_�[�^�[�Q�b�g
	gpsDesc.NumRenderTargets = 1;
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	//�[�x�X�e���V��
	gpsDesc.DepthStencilState.DepthEnable = false;
	gpsDesc.DepthStencilState.StencilEnable = false;

	//���X�^���C�U
	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	//���̑�
	gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.NodeMask = 0;
	gpsDesc.SampleDesc.Count = 1;
	gpsDesc.SampleDesc.Quality = 0;
	gpsDesc.SampleMask = 0xffffffff;
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//�O�p�`

	result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&_peraPipeline));

	return result;
}

// �񖇖ڃ|���S���p�̃��[�g�V�O�l�`���̍쐬
HRESULT Dx12Wrapper::CreateSecondSignature()
{
	// �T���v���̐ݒ�
	D3D12_STATIC_SAMPLER_DESC SamplerDesc[1] = {};
	SamplerDesc[0].Filter = D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR;// ���`�⊮����
	SamplerDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;// �悪�J��Ԃ��`�悳���
	SamplerDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	SamplerDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	SamplerDesc[0].MaxLOD = D3D12_FLOAT32_MAX;// ����Ȃ�
	SamplerDesc[0].MinLOD = 0.0f;// �����Ȃ�
	SamplerDesc[0].MipLODBias = 0.0f;// MIPMAP�̃o�C�A�X
	SamplerDesc[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;// �G�b�W�̐F(��)
	SamplerDesc[0].ShaderRegister = 0;// �g�p���郌�W�X�^�X���b�g
	SamplerDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;// �ǂ̂��炢�̃f�[�^���V�F�[�_�Ɍ����邩(�S��)
	SamplerDesc[0].RegisterSpace = 0;// �킩���
	SamplerDesc[0].MaxAnisotropy = 0;// Filter��Anisotropy�̂Ƃ��̂ݗL��

	// �����W�̐ݒ�
	D3D12_DESCRIPTOR_RANGE descRange[1] = {};// �ꖇ�|���S��
	descRange[0].NumDescriptors = 1;
	descRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;// �V�F�[�_�[���\�[�X
	descRange[0].BaseShaderRegister = 0;//���W�X�^�ԍ�
	descRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ���[�g�p�����[�^�ϐ��̐ݒ�
	D3D12_ROOT_PARAMETER rootParam[1] = {};
	rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[0].DescriptorTable.pDescriptorRanges = &descRange[0];//�Ή����郌���W�ւ̃|�C���^
	rootParam[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//���ׂẴV�F�[�_����Q��

	// ���[�g�V�O�l�`������邽�߂̕ϐ��̐ݒ�
	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.pStaticSamplers = SamplerDesc;
	rsd.pParameters = rootParam;
	rsd.NumParameters = 1;
	rsd.NumStaticSamplers = 1;

	auto result = D3D12SerializeRootSignature(
		&rsd,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&signature,
		&error);

	// ���[�g�V�O�l�`���{�̂̍쐬
	result = _dev->CreateRootSignature(
		0,
		signature->GetBufferPointer(),
		signature->GetBufferSize(),
		IID_PPV_ARGS(&_peraSignature2));

	return result;
}

// �񖇖ڃ|���S���p�̃p�C�v���C���̍쐬
HRESULT Dx12Wrapper::CreateSecondPopelineState()
{
	auto result = D3DCompileFromFile(L"pera2.hlsl", nullptr, nullptr, "pera2VS", "vs_5_0", D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION, 0, &peraVertShader2, nullptr);

	// �s�N�Z���V�F�[�_
	result = D3DCompileFromFile(L"pera2.hlsl", nullptr, nullptr, "pera2PS", "ps_5_0", D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION, 0, &peraPixShader2, nullptr);

	D3D12_INPUT_ELEMENT_DESC peraLayoutDescs[] =
	{
		// Pos
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0, D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		// UV
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};

	// �p�C�v���C������邽�߂�GPS�̕ϐ��̍쐬
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};

	//���[�g�V�O�l�`���ƒ��_���C�A�E�g
	gpsDesc.pRootSignature = _peraSignature2;
	gpsDesc.InputLayout.pInputElementDescs = peraLayoutDescs;
	gpsDesc.InputLayout.NumElements = _countof(peraLayoutDescs);

	//�V�F�[�_�̃Z�b�g
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(peraVertShader2);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(peraPixShader2);

	//�����_�[�^�[�Q�b�g
	gpsDesc.NumRenderTargets = 1;
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	//�[�x�X�e���V��
	gpsDesc.DepthStencilState.DepthEnable = false;
	gpsDesc.DepthStencilState.StencilEnable = false;

	//���X�^���C�U
	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	//���̑�
	gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.NodeMask = 0;
	gpsDesc.SampleDesc.Count = 1;
	gpsDesc.SampleDesc.Quality = 0;
	gpsDesc.SampleMask = 0xffffffff;
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//�O�p�`

	result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&_peraPipeline2));

	return result;
}

// �ꖇ�ڃ|���S���̕`��
void Dx12Wrapper::DrawFirstPolygon()
{
	// �q�[�v�̊J�n�ʒu���擾
	auto heapStart = _rtvDescHeap2->GetCPUDescriptorHandleForHeapStart();// �񖇖ڂ̃|���S���ɕ`��
	auto depthStart = _dsvDescHeap->GetCPUDescriptorHandleForHeapStart();
	auto lightStart = _lightDescHeap->GetCPUDescriptorHandleForHeapStart();

	// �o���A�̏��ݒ�
	D3D12_RESOURCE_BARRIER BarrierDesc = {};
	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.Transition.pResource = _peraBuffer2;// �񖇖ڂ̃|���S���ɕ`��
	BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	
	// �R�}���h�A���P�[�^�ƃ��X�g�̃��Z�b�g
	_cmdAlloc->Reset();
	_cmdList->Reset(_cmdAlloc, nullptr);

	// �p�C�v���C���̃Z�b�g
	_cmdList->SetPipelineState(_peraPipeline);

	// ���[�g�V�O�l�`�����Z�b�g
	_cmdList->SetGraphicsRootSignature(_peraSignature);

	//�r���[�|�[�g�ƃV�U�[�ݒ�
	_cmdList->RSSetViewports(1, &_viewPort);
	_cmdList->RSSetScissorRects(1, &_scissor);

	// ���\�[�X�o���A
	_cmdList->ResourceBarrier(1, &BarrierDesc);

	// �����_�[�^�[�Q�b�g�̐ݒ�
	_cmdList->OMSetRenderTargets(1, &heapStart, false, nullptr);
	_cmdList->ClearRenderTargetView(heapStart, clearColor, 0, nullptr);

	// ���_�o�b�t�@�r���[�̐ݒ�
	_cmdList->IASetVertexBuffers(0, 1, &_peravbView);

	// �g�|���W�[�̃Z�b�g
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// �f�X�N���v�^�̃Z�b�g
	auto srvStart = _srvDescHeap->GetGPUDescriptorHandleForHeapStart();
	_cmdList->SetDescriptorHeaps(1,&_srvDescHeap);
	_cmdList->SetGraphicsRootDescriptorTable(0, srvStart);

	// �J�����[�x�̃Z�b�g
	auto dstart = _dsvSrvHeap->GetGPUDescriptorHandleForHeapStart();
	_cmdList->SetDescriptorHeaps(1, &_dsvSrvHeap);
	_cmdList->SetGraphicsRootDescriptorTable(1, dstart);

	// ���C�g�[�x�̃Z�b�g
	auto lstart = _lightSrvHeap->GetGPUDescriptorHandleForHeapStart();
	_cmdList->SetDescriptorHeaps(1, &_lightSrvHeap);
	_cmdList->SetGraphicsRootDescriptorTable(2, lstart);


	_cmdList->SetDescriptorHeaps(1, &_srvDescHeap);
	srvStart.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	_cmdList->SetGraphicsRootDescriptorTable(3, srvStart);

	srvStart.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	_cmdList->SetGraphicsRootDescriptorTable(4, srvStart);

	auto ssh = _shrinkSrvHeap->GetGPUDescriptorHandleForHeapStart();
	_cmdList->SetDescriptorHeaps(1, &_shrinkSrvHeap);
	_cmdList->SetGraphicsRootDescriptorTable(5, ssh);

	// �F�̂�����
	_cmdList->SetDescriptorHeaps(1, &_colDescHeap);
	auto colStart = _colDescHeap->GetGPUDescriptorHandleForHeapStart();
	_cmdList->SetGraphicsRootDescriptorTable(6, colStart);

	// �t���O�̂�����
	_cmdList->SetDescriptorHeaps(1, &_flagsHeap);
	auto flagStart = _flagsHeap->GetGPUDescriptorHandleForHeapStart();
	_cmdList->SetGraphicsRootDescriptorTable(7, flagStart);
	
	_cmdList->SetDescriptorHeaps(1, &_srvDescHeap);
	srvStart.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	_cmdList->SetGraphicsRootDescriptorTable(8, srvStart);

	_cmdList->SetDescriptorHeaps(1, &_sceneSrvHeap);
	auto scene = _sceneSrvHeap->GetGPUDescriptorHandleForHeapStart();
	_cmdList->SetGraphicsRootDescriptorTable(9, scene);

	// SSAO�̂�����
	_cmdList->SetDescriptorHeaps(1, &_SSAOSrv);
	auto SSAOStart = _SSAOSrv->GetGPUDescriptorHandleForHeapStart();
	_cmdList->SetGraphicsRootDescriptorTable(10, SSAOStart);

	// �|���S���̕`��
	_cmdList->DrawInstanced(4, 1, 0, 0);

	// �o���A�����Ƃɖ߂�
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	_cmdList->ResourceBarrier(1, &BarrierDesc);

	// ���X�g�̃N���[�Y
	_cmdList->Close();
}

// �񖇖ڃ|���S���̕`��
void Dx12Wrapper::DrawSecondPolygon()
{
	// �q�[�v�̊J�n�ʒu���擾
	auto heapStart = _swcDescHeap->GetCPUDescriptorHandleForHeapStart();
	auto depthStart = _dsvDescHeap->GetCPUDescriptorHandleForHeapStart();

	// �o�b�N�o�b�t�@�̃C���f�b�N�X���Ƃ��Ă��ē���ւ���
	auto bbIndex = _swapchain->GetCurrentBackBufferIndex();
	int DescriptorSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	heapStart.ptr += bbIndex * DescriptorSize;// �|�C���^���o�b�t�@�T�C�Y�����炷

	// �o���A�̏��ݒ�
	D3D12_RESOURCE_BARRIER BarrierDesc = {};
	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.Transition.pResource = renderTargets[bbIndex];
	BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	
	// �R�}���h�A���P�[�^�ƃ��X�g�̃��Z�b�g
	_cmdAlloc->Reset();
	_cmdList->Reset(_cmdAlloc, nullptr);

	// �p�C�v���C���ƃ��[�g�V�O�l�`�����Z�b�g
	_cmdList->SetPipelineState(_peraPipeline2);
	_cmdList->SetGraphicsRootSignature(_peraSignature2);

	//�r���[�|�[�g�ƃV�U�[��ݒ�
	_cmdList->RSSetViewports(1, &_viewPort);
	_cmdList->RSSetScissorRects(1, &_scissor);

	// ���\�[�X�o���A�𒣂�
	_cmdList->ResourceBarrier(1, &BarrierDesc);

	// �����_�[�^�[�Q�b�g�̐ݒ�
	_cmdList->OMSetRenderTargets(1, &heapStart, false, &depthStart);
	_cmdList->ClearRenderTargetView(heapStart, clearColor, 0, nullptr);// �N���A

	// ���_�o�b�t�@�r���[�̐ݒ�
	_cmdList->IASetVertexBuffers(0, 1, &_peravbView);

	// �g�|���W�[�̃Z�b�g
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// �f�X�N���v�^�̃Z�b�g
	_cmdList->SetDescriptorHeaps(1, &_srvDescHeap2);
	_cmdList->SetGraphicsRootDescriptorTable(0, _srvDescHeap2->GetGPUDescriptorHandleForHeapStart());
	
	// �|���S���̕`��
	_cmdList->DrawInstanced(4, 1, 0, 0);	  

	// GUI
	ImGui::SetNextWindowSize(ImVec2(400,600));
	ImGui::Begin("gui");
	ImGui::BulletText("light");
	ImGui::SliderAngle("���C�g�p�x", &lightangle,-180,180,"%.02f");
	ImGui::BulletText("instanceNum");
	ImGui::SliderInt("�C���X�^���X��", &instanceNum,1,25);
	ImGui::BulletText("blooomColor");
	ImGui::ColorPicker4("�u���[���̐F", bloomCol.bloom);
	ImGui::CheckboxFlags("GBuffer", &(flags.GBuffers),1);
	ImGui::CheckboxFlags("CenterLine",&(flags.CenterLine),1);
	ImGui::End();

	// ��ȏ�o���Ƃ��͂����ł�����xBegin����End����
	ImGui::Render();
	_cmdList->SetDescriptorHeaps(1, &_imguiDescHeap);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), _cmdList);

	// �o���A�����Ƃɖ߂�
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	_cmdList->ResourceBarrier(1, &BarrierDesc);

	// ���X�g�̃N���[�Y
	_cmdList->Close();
}

/// �|���S���֐������܂�


// �R���X�g���N�^
Dx12Wrapper::Dx12Wrapper(HWND hwnd) :_hwnd(hwnd)
{
	// ���W�̏����l
	eye = XMFLOAT3(0, 10, -25);
	target = XMFLOAT3(0, 10, 0);
	up = XMFLOAT3(0, 1, 0);
	// �C�j�V�����C�Y
	auto result = CoInitializeEx(0, COINIT_MULTITHREADED);
}
// �f�X�g���N�^
Dx12Wrapper::~Dx12Wrapper()
{
	// �L�[�{�[�h�f�o�C�X�̉��
	_keyBoadDev->Unacquire();

	if (_keyBoadDev != NULL)
		_keyBoadDev->Release();

	if (_directInput != NULL)
		_directInput->Release();
}

// �L�[���͍X�V�֐�
void Dx12Wrapper::KeyUpDate()
{
	// GUI�̖��ڰя�����
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	auto wsize = Application::Instance().GetWindowSize();
	// �L�[�̓���
	char Oldkey[256];
	int i = 0;
	for (auto& k : key)
	{
		Oldkey[i++] = k;
	}

	auto isOk = _keyBoadDev->GetDeviceState(sizeof(key), key);
	if (FAILED(isOk)) {
		// ���s�Ȃ�ĊJ�����Ă�����x�擾
		_keyBoadDev->Acquire();
		_keyBoadDev->GetDeviceState(sizeof(key), key);
	}

	float addsize = 0.5f;
	if (key[DIK_ESCAPE])
	{
		PostQuitMessage(0);
	}

	if (key[DIK_SPACE] && (Oldkey[DIK_SPACE] == 0))
	{
		if (_efkManager->Exists(_efkHandle)) {
			_efkManager->StopEffect(_efkHandle);
		}
		// �J�����s���ݒ�
		_efkRenderer->SetCameraMatrix(
			Effekseer::Matrix44().LookAtRH(
				Effekseer::Vector3D(eye.x, eye.y, eye.z),
				Effekseer::Vector3D(target.x, target.y, target.z),
				Effekseer::Vector3D(up.x, up.y, up.z)
			)
		);
		_efkHandle = _efkManager->Play(_effect, Effekseer::Vector3D(0, 0, 0));
		_efkManager->SetScale(_efkHandle, 3, 3, 3);
	}

	if (key[DIK_UP])
	{
		eye.y += addsize;
		target.y += addsize;
	}
	else if (key[DIK_DOWN])
	{
		eye.y -= addsize;
		target.y -= addsize;
	}

	if (key[DIK_Q] || key[DIK_LEFT])
	{
		angle.y += addsize / 10;
	}
	else if (key[DIK_E] || key[DIK_RIGHT])
	{
		angle.y -= addsize / 10;
	}
	if (key[DIK_Z])
	{
		angle.x += addsize / 10;
	}
	else if (key[DIK_X])
	{
		angle.x -= addsize / 10;
	}
	if (key[DIK_F])
	{
		angle.z += addsize / 10;
	}
	else if (key[DIK_G])
	{
		angle.z -= addsize / 10;
	}


	if (key[DIK_W])
	{
		eye.z += addsize;
		target.z += addsize;
	}
	else if (key[DIK_S])
	{
		eye.z -= addsize;
		target.z -= addsize;
	}

	if (key[DIK_D])
	{
		eye.x += addsize;
		target.x += addsize;
	}
	if (key[DIK_A])
	{
		eye.x -= addsize;
		target.x -= addsize;
	}

	if (key[DIK_R])
	{
		eye = XMFLOAT3(0, 10, -25);
		target = XMFLOAT3(0, 10, 0);
		up = XMFLOAT3(0, 1, 0);
		angle = {};
	}
	if (key[DIK_P])
	{
		isOk = 0;
	}
	if (key[DIK_P] && (Oldkey[DIK_P] == 0))
	{
		float rota[3] = { XM_PIDIV4 , XM_PIDIV2 ,XM_PI / 30 * XM_PI };
		_persLevel++;
		_wvp.projection = XMMatrixPerspectiveFovLH(
			rota[_persLevel % 3],
			static_cast<float>(wsize.width) / static_cast<float>(wsize.height),
			0.1f,
			1000.0f
		);
	}

	pmdModel->UpDate();// PMD���f���̍X�V

	// PMX���f���̍X�V
	for (auto& model : pmxModels)
	{
		model->UpDate(key);
	}

	// ���W�����_�ɋ߂����ɕ`�揇����ёւ���
	std::sort(pmxModels.begin(), pmxModels.end(), [](std::shared_ptr<PMXmodel>& a, std::shared_ptr<PMXmodel>& b)
	{
		return (a->GetPos().z > b->GetPos().z);
	});

	// ���݂̃J�����r���[�s���ۑ�

	_wvp.view = DirectX::XMMatrixLookAtLH(
		DirectX::XMLoadFloat3(&eye),
		DirectX::XMLoadFloat3(&target),
		DirectX::XMLoadFloat3(&up));
/*
	Axis axis(XMFLOAT3(1, 0, 0), XMFLOAT3(0, 1, 0), XMFLOAT3(0, 0, 1));
	_wvp.view *= XMMatrixRotationQuaternion(XMQuaternionRotationAxis(XMLoadFloat3(&axis.x),angle.x))*
				 XMMatrixRotationQuaternion(XMQuaternionRotationAxis(XMLoadFloat3(&axis.y),angle.y))*
				 XMMatrixRotationQuaternion(XMQuaternionRotationAxis(XMLoadFloat3(&axis.z),angle.z));*/

	_wvp.view = XMMatrixRotationY(angle.y) *_wvp.view;// �J�����̉�]
	_wvp.view = XMMatrixRotationX(angle.x) *_wvp.view;// �J�����̉�]
	_wvp.view = XMMatrixRotationZ(angle.z) *_wvp.view;// �J�����̉�]


	_wvp.wvp = _wvp.world * _wvp.view *_wvp.projection;

	// ���݂̃��C�g�r���[�s���ۑ�
	XMMATRIX lightview = DirectX::XMMatrixLookAtLH(
		DirectX::XMLoadFloat3(&_wvp.lightPos),
		DirectX::XMLoadFloat3(&lightTag),
		DirectX::XMLoadFloat3(&up)
	);
	lightview = DirectX::XMMatrixRotationY(lightangle)*lightview;// ���C�g�̉�]
	_wvp.lvp = lightview * lightproj;// ���C�g���W�̍X�V

	*_wvpMP = _wvp;// �]���p�ɏ�������

	*MapCol = bloomCol;
	*MapFlags = flags;
}

// ��ʕ`��֐�
void Dx12Wrapper::ScreenUpDate()
{
	auto wsize = Application::Instance().GetWindowSize();

	// �[�x�o�b�t�@�r���[�q�[�v�̊J�n�ʒu���Ƃ��Ă���
	auto depthStart = _dsvDescHeap->GetCPUDescriptorHandleForHeapStart();
	auto lightStart = _lightDescHeap->GetCPUDescriptorHandleForHeapStart();

	auto peraStart = _rtvDescHeap->GetCPUDescriptorHandleForHeapStart();// �ꖇ�ڂ̃|���S���ɕ`��
	
	_cmdAlloc->Reset();
	_cmdList->Reset(_cmdAlloc, nullptr);//�R�}���h���X�g���Z�b�g

	// �V���h�E�̕`��
	//�����_�[�^�[�Q�b�g�ݒ�
	_cmdList->OMSetRenderTargets(0, nullptr, false, &lightStart);

	// ���C�g�[�x�̃N���A
	_cmdList->ClearDepthStencilView(lightStart, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	for (auto &model : pmxModels)
	{
		model->PreDrawShadow(_cmdList, _wvpDescHeap, instanceNum);
	}

	// ���f���̕`��
	// �J�����[�x�̃N���A
	_cmdList->ClearDepthStencilView(depthStart, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// ���\�[�X�o���A
	for (int i= 0;i< _peraBuffers.size();++i)
	{
		_cmdList->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(
				_peraBuffers[i],
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET)
		);
	}	

	//�����_�[�^�[�Q�b�g�ݒ�
	_cmdList->OMSetRenderTargets(_peraBuffers.size(), &peraStart, true, &depthStart);

	//�����_�[�^�[�Q�b�g�̃N���A
	for (int i = 0; i < _peraBuffers.size(); ++i)
	{
		_cmdList->ClearRenderTargetView(peraStart, clearColor, 0, nullptr);
		peraStart.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// ���̂т傤��
	_plane->Draw(_cmdList, _wvpDescHeap, _lightSrvHeap);

	// ���f���̕`��
	for (auto &model : pmxModels)
	{
		model->Draw(_cmdList, _wvpDescHeap, _lightSrvHeap, instanceNum);
	}

	// �G�t�F�N�g�̕`��
	_efkManager->Update();
	_efkMemoryPool->NewFrame();

	EffekseerRendererDX12::BeginCommandList(_efkCmdList, _cmdList);
	_efkRenderer->BeginRendering();
	_efkManager->Draw();
	_efkRenderer->EndRendering();
	EffekseerRendererDX12::EndCommandList(_efkCmdList);

	// ���\�[�X�o���A
	for (int i = 0; i < _peraBuffers.size(); ++i)
	{
		_cmdList->ResourceBarrier(1,
			&CD3DX12_RESOURCE_BARRIER::Transition(
				_peraBuffers[i],
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
		);
	}

	//�R�}���h�̃N���[�Y
	_cmdList->Close();

	// �R�}���h�ǉ��ƃt�F���X
	ExecuteCommand();

	// �҂�
	WaitWithFence();

	// ���P�x
	DrawToShrinkBuffer();

	// �R�}���h�ǉ��ƃt�F���X
	ExecuteCommand();

	// �҂�
	WaitWithFence();
	
	// ��ʊE�[�x
	DrawToSceneBuffer();

	// �R�}���h�ǉ��ƃt�F���X
	ExecuteCommand();

	// �҂�
	WaitWithFence();

	// SSAO
	DrawToSSAO();

	// �R�}���h�ǉ��ƃt�F���X
	ExecuteCommand();

	// �҂�
	WaitWithFence();

	// �|���S���̕`��
	DrawFirstPolygon();

	// �R�}���h�ǉ��ƃt�F���X
	ExecuteCommand();

	// �҂�
	WaitWithFence();


	// �|���S���̕`��
	DrawSecondPolygon();

	// �R�}���h�ǉ��ƃt�F���X
	ExecuteCommand();

	// �҂�
	WaitWithFence();

	// ��ʂ̍X�V
	_swapchain->Present(1, 0);
}

// ������
int Dx12Wrapper::Init()
{
	// ���ޯ��ڲ԰��ON
#if defined(_DEBUG)
	ID3D12Debug* debug;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
		debug->EnableDebugLayer();
		debug->Release();
	}
#endif
	// �r���[�|�[�g�̐ݒ�
	_viewPort.TopLeftX = 0;
	_viewPort.TopLeftY = 0;
	_viewPort.Width = static_cast<float>(Application::Instance().GetWindowSize().width);
	_viewPort.Height = static_cast<float>(Application::Instance().GetWindowSize().height);
	_viewPort.MaxDepth = 1.0f;
	_viewPort.MinDepth = 0.0f;

	// �V�U�[�̐ݒ�
	_scissor.left = 0;
	_scissor.top = 0;
	_scissor.right = Application::Instance().GetWindowSize().width;
	_scissor.bottom = Application::Instance().GetWindowSize().height;

	// �f�o�C�X�̐���
	if (FAILED(DeviceInit()))
		return 1;

	_plane = std::make_shared<Plane>(_dev, _viewPort, _scissor);

	// ���f���ǂݍ���
	pmdModel = std::make_shared<PMDmodel>(_dev, "model/PMD/�����~�N.pmd");
	//pmxModels.emplace_back(std::make_shared<PMXmodel>(_dev, "model/PMX/���уt����/���уt����.pmx", "VMD/45�bMIKU.vmd"));
	//pmxModels.emplace_back(std::make_shared<PMXmodel>(_dev, "model/PMX/2B/na_2b_0407.pmx", "VMD/45�bGUMI.vmd"));
	//pmxModels.emplace_back(std::make_shared<PMXmodel>(_dev, "model/PMX/GUMI/GUMI��_V3.pmx", "VMD/DanceRobotDance_Motion.vmd"));
	//pmxModels.emplace_back(std::make_shared<PMXmodel>(_dev, "model/PMX/���у��[�~�A/���у��[�~�A�W���{�[��.pmx", "VMD/45�bGUMI.vmd"));
	//pmxModels.emplace_back(std::make_shared<PMXmodel>(_dev, "model/PMX/���у��[�~�A/���у��[�~�A.pmx", "VMD/45�bGUMI.vmd"));
	pmxModels.emplace_back(std::make_shared<PMXmodel>(_dev, "model/PMX/���у��[�~�A/���у��[�~�A�W���{�[��.pmx", "VMD/���S�R���_���X.vmd"));
	//pmxModels.emplace_back(std::make_shared<PMXmodel>(_dev, "model/PMX/���уt����/���уt�����W���{�[��.pmx", "VMD/���S�R���_���X.vmd"));

	/* A�|�[�Y(�e�X�g�p */
	//pmxModels.emplace_back(std::make_shared<PMXmodel>(_dev, "model/PMX/���у��[�~�A/���у��[�~�A.pmx"));


	SetEfkRenderer();
	_efkHandle = _efkManager->Play(_effect, Effekseer::Vector3D(0, 0, 0));
	_efkManager->SetScale(_efkHandle, 3, 3, 3);

	if (FAILED(CreateSwapChainAndCmdQue()))
		return 2;

	if (FAILED(CreateCmdListAndAlloc()))
		return 3;

	if (FAILED(CreateFence()))
		return 4;

	if (FAILED(CreateDepthStencilView()))
		return 5;

	if (FAILED(CreateWVPConstantBuffer()))
		return 6;

	if (FAILED(CreateFirstPolygon()))
		return 7;

	if (FAILED(CreateFirstSignature()))
		return 8;

	if (FAILED(CreateFirstPopelineState()))
		return 9;

	if (FAILED(CreateSecondPolygon()))
		return 10;

	if (FAILED(CreateSecondSignature()))
		return 11;

	if (FAILED(CreateSecondPopelineState()))
		return 12;

	if (FAILED(CreatePolygonVertexBuffer()))
		return 13;
	
	if (FAILED(CreateShrinkBuffer()))
		return 14;

	if (FAILED(CreateShrinkPipline()))
		return 15;
	
	if (FAILED(CreateSceneBuffer()))
		return 16;

	if (FAILED(CreateScenePipline()))
		return 17;

	if (FAILED(CreateColBuffer()))
		return 18;

	if (FAILED(CreateFlagsBuffer()))
		return 19;

	if (FAILED(CreateSSAOBuffer()))
		return 20;

	if (FAILED(CreateSSAOPS()))
		return 21;
	


	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = 1;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	auto imguiresult = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&_imguiDescHeap));

	ImGui::CreateContext();
	imguiresult = ImGui_ImplWin32_Init(_hwnd);
	assert(imguiresult);
	imguiresult = ImGui_ImplDX12_Init(_dev, 2,
		DXGI_FORMAT_R8G8B8A8_UNORM, 
		_imguiDescHeap,
		_imguiDescHeap->GetCPUDescriptorHandleForHeapStart(),
		_imguiDescHeap->GetGPUDescriptorHandleForHeapStart());
	assert(imguiresult);

	return 0;
}

// ���b�p�[�̍X�V
void Dx12Wrapper::UpDate()
{
	KeyUpDate();
	ScreenUpDate();
}
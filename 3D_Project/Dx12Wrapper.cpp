#include "Dx12Wrapper.h"
#include "Application.h"
#include <wrl.h>
#include <d3dcompiler.h>
#include <DirectXTex.h>
#include <algorithm>
#include "PMDmodel.h"
#include "PMXmodel.h"

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"DirectXTex.lib")


const FLOAT clearColor[] = { 0.5f,0.5f,0.5f,0.0f };//�N���A�J���[�ݒ�

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

HRESULT Dx12Wrapper::CreateSwapChainAndCmdQue()
{
	HRESULT result = S_OK;
	/* --�R�}���h�L���[�̍쐬-- */
	{
	D3D12_COMMAND_QUEUE_DESC cmdQDesc = {};
	cmdQDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmdQDesc.NodeMask = 0;
	cmdQDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmdQDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	// �R�}���h�L���[�̍쐬�ƃ`�F�b�N
	if (FAILED(result = _dev->CreateCommandQueue(&cmdQDesc, IID_PPV_ARGS(&_cmdQue))))
	{
		return result;
	};
	}
	/* -- �����܂� -- */

	/* --�X���b�v�`�F�C���̍쐬--�@*/
	{
	// �E�B���h�E�T�C�Y�̎擾
	Size window = Application::Instance().GetWindowSize();

	// �X���b�v�`�F�C���p�̕ϐ��̍쐬
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	{
		swapChainDesc.Width = window.width; // �E�B���h�E�̕�
		swapChainDesc.Width = window.height; // �E�B���h�E�̍���
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // �t�H�[�}�b�g�̎w��(�����RGBA�e8�r�b�g�̃m�[�}��)
		swapChainDesc.Stereo = FALSE; // 3DS�I�ȕ\�������邩�ǂ����A��{��false
		swapChainDesc.SampleDesc.Count = 1; // �悭�킩���
		swapChainDesc.SampleDesc.Quality = 0; // �悭�킩���
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // �o�b�t�@�̎g�p�@(����͏o�͗p������Č����Ă�)
		swapChainDesc.BufferCount = 2; // �o�b�t�@�̐�(�\�Ɨ��œ񖇂Ȃ̂łQ)
		swapChainDesc.Scaling = DXGI_SCALING_STRETCH; // �o�b�t�@�̃X�P�[�����O���@(����̂̓T�C�Y�𖳏����ň������΂����)
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // �o�b�t�@�̐؂�ւ����ɂǂ��؂�ւ��邩���w��A����̓h���C�o�ˑ��B
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED; // �A���t�@�l�̐ݒ�(����̓A���t�@�̎w��͂Ȃ�)
		swapChainDesc.Flags = 0; // �o�b�N�o�b�t�@����t�����g�o�b�t�@�ւ̈ڍs�̂Ƃ��̃I�v�V�������ݒ�ł���炵���B�悭�킩��񂩂�O
	}

	// �X���b�v�`�F�C���{�̂̍쐬
	result = _dxgi->CreateSwapChainForHwnd(_cmdQue,
		_hwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)(&_swapchain));
	}
	/* --�����܂�-- */

	/* --�X���b�v�`�F�C���Ɏg�������_�[�^�[�Q�b�g�r���[�̍쐬-- */
	{
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

		// �����_�[�^�[�Q�b�g�r���[�{�̂̍쐬
		for (int i = 0; i < renderTargetsNum; i++)
		{
			result = _swapchain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
			if (FAILED(result))break;

			_dev->CreateRenderTargetView(renderTargets[i], nullptr, swchandle);
			swchandle.ptr += descriptorSize;
		}
	}
	return result;
}

HRESULT Dx12Wrapper::CreateCmdListAndAlloc()
{
	HRESULT result = S_OK;

	_dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAlloc));
	_dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAlloc, nullptr, IID_PPV_ARGS(&_cmdList));

	_cmdList->Close();

	return result;
}

HRESULT Dx12Wrapper::CreateFence()
{
	HRESULT result = S_OK;
	_fenceValue = 0;
	result = _dev->CreateFence(_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
	return result;
}


HRESULT Dx12Wrapper::CreateFirstSignature()
{
	// �T���v���̐ݒ�
	D3D12_STATIC_SAMPLER_DESC SamplerDesc[1] = {};
	SamplerDesc[0].Filter = D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT;// ���ʂȃt�B���^���g�p���Ȃ�
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
	SamplerDesc[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;// ��ɔے�

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
		IID_PPV_ARGS(&_peraSignature));

	return result;
}

HRESULT Dx12Wrapper::CreateSecondSignature()
{
	// �T���v���̐ݒ�
	D3D12_STATIC_SAMPLER_DESC SamplerDesc[1] = {};
	SamplerDesc[0].Filter = D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT;// ���ʂȃt�B���^���g�p���Ȃ�
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
	SamplerDesc[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;// ��ɔے�

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

HRESULT Dx12Wrapper::CreateFirstPopelineState()
{
	auto result = D3DCompileFromFile(L"PeraShader.hlsl", nullptr, nullptr, "peraVS", "vs_5_0", D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION, 0, &peraVertShader, nullptr);

	// �s�N�Z���V�F�[�_
	result = D3DCompileFromFile(L"PeraShader.hlsl", nullptr, nullptr, "peraPS", "ps_5_0", D3DCOMPILE_DEBUG |
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

HRESULT Dx12Wrapper::CreateConstantBuffer()
{
	// �R���X�^���\�o�b�t�@�p�̃q�[�v�v���b�v
	D3D12_HEAP_PROPERTIES cbvHeapProp = {};
	cbvHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	cbvHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	cbvHeapProp.CreationNodeMask = 1;
	cbvHeapProp.VisibleNodeMask = 1;
	cbvHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_DESCRIPTOR_HEAP_DESC rgstDesc = {};
	rgstDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	rgstDesc.NodeMask = 0;
	rgstDesc.NumDescriptors = 1;
	rgstDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	auto result = _dev->CreateDescriptorHeap(&rgstDesc,IID_PPV_ARGS(&_cameraDescHeap));
	auto wsize = Application::Instance().GetWindowSize();

	// ���W�̏����l
	eye = XMFLOAT3(0,10,-25);
	target = XMFLOAT3(0, 10,0);
	up = XMFLOAT3(0,1,0);

	angle = 0.f;

	_wvp.world = DirectX::XMMatrixRotationY(angle);

	_wvp.view = DirectX::XMMatrixLookAtLH(
		DirectX::XMLoadFloat3(&eye),
		DirectX::XMLoadFloat3(&target),
		DirectX::XMLoadFloat3(&up)
	);

	_wvp.projection = XMMatrixPerspectiveFovLH(
		XM_PIDIV4,
		static_cast<float>(wsize.width) / static_cast<float>(wsize.height),
		0.1f,
		300.0f
	);

	// �T�C�Y�𒲐�
	size_t size = sizeof(_wvp);
	size = (size + 0xff)&~0xff;

	// �R���X�^���g�o�b�t�@�̍쐬
	result = _dev->CreateCommittedResource(&cbvHeapProp,
		D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_cameraBuff));

	// Mapping
	result = _cameraBuff->Map(0,nullptr,(void**)&_wvpMP);
	std::memcpy(_wvpMP,&_wvp,sizeof(_wvp));

	// �R���X�^���g�o�b�t�@�p�̃f�X�N
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _cameraBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = static_cast<unsigned int>(size);

	auto handle = _cameraDescHeap->GetCPUDescriptorHandleForHeapStart();
	// �R���X�^���g�o�b�t�@�r���[�̍쐬
	_dev->CreateConstantBufferView(&cbvDesc, handle);

	return result;
}

HRESULT Dx12Wrapper::CreateFirstPolygon()
{
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = 1;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	// �����_�[�^�[�Q�b�g�f�X�N���v�^�q�[�v�̍쐬
	auto result = _dev->CreateDescriptorHeap(&descHeapDesc,IID_PPV_ARGS(&_rtvDescHeap));

	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NumDescriptors = 1;
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


	D3D12_CLEAR_VALUE clear = { DXGI_FORMAT_R8G8B8A8_UNORM ,{0.5f,0.5f,0.5f,0.0f}, };

	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		&clear,
		IID_PPV_ARGS(&_peraBuffer));

	// �����_�[�^�[�Q�b�g�r���[�̍쐬
	_dev->CreateRenderTargetView(_peraBuffer, nullptr, _rtvDescHeap->GetCPUDescriptorHandleForHeapStart());

	// �V�F�[�_�[���\�[�X�r���[�̍쐬
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = desc.Format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	_dev->CreateShaderResourceView(_peraBuffer, &srvDesc, _srvDescHeap->GetCPUDescriptorHandleForHeapStart());
	return result;
}

HRESULT Dx12Wrapper::CreatePolygonVB()
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
	);	Vertex* mapver = nullptr;	result = _peraVertBuff->Map(0,nullptr, (void**)&mapver);
	std::copy(std::begin(vertices), std::end(vertices), mapver);	_peraVertBuff->Unmap(0, nullptr);	_peravbView.BufferLocation = _peraVertBuff->GetGPUVirtualAddress();
	_peravbView.SizeInBytes = sizeof(vertices);
	_peravbView.StrideInBytes = sizeof(Vertex);

	return result;
}

HRESULT Dx12Wrapper::CreateDSV()
{
	Size wsize = Application::Instance().GetWindowSize();

	D3D12_DESCRIPTOR_HEAP_DESC _dsvHeapDesc = {};
	_dsvHeapDesc.NumDescriptors = 1;
	_dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	auto result = _dev->CreateDescriptorHeap(&_dsvHeapDesc, IID_PPV_ARGS(&_dsvDescHeap));
	
	// DSV�p�̃��\�[�X�f�X�N
	D3D12_RESOURCE_DESC depthResDesc = {};
	depthResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthResDesc.Width = wsize.width;
	depthResDesc.Height = wsize.height;
	depthResDesc.DepthOrArraySize = 1;
	depthResDesc.Format = DXGI_FORMAT_D32_FLOAT;
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

	result = _dev->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&_depthClearValue,
		IID_PPV_ARGS(&_depthBuffer));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvdec = {};
	dsvdec.Format = DXGI_FORMAT_D32_FLOAT;
	dsvdec.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvdec.Texture2D.MipSlice = 0;
	dsvdec.Flags = D3D12_DSV_FLAG_NONE;
	
	_dev->CreateDepthStencilView(_depthBuffer, &dsvdec, _dsvDescHeap->GetCPUDescriptorHandleForHeapStart());

	return result;
}

void Dx12Wrapper::WaitWithFence()
{
	int cnt = 0;
	while (_fence->GetCompletedValue() != _fenceValue){
		//�т��[���
	};
}

void Dx12Wrapper::ExecuteCommand()
{
	_cmdQue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&_cmdList);
	_cmdQue->Signal(_fence, ++_fenceValue);
}

Dx12Wrapper::Dx12Wrapper(HWND hwnd):_hwnd(hwnd)
{
	// �C�j�V�����C�Y
	auto result = CoInitializeEx(0, COINIT_MULTITHREADED);
}

Dx12Wrapper::~Dx12Wrapper()
{
	// �L�[�{�[�h�f�o�C�X�̉��
	_keyBoadDev->Unacquire();

	if (_keyBoadDev != NULL)
		_keyBoadDev->Release();

	if (_directInput != NULL)
		_directInput->Release();
}

// �����ȏ������`�F�b�N
int Dx12Wrapper::Init()
{
	// ���ޯ��ڲ԰��ON
#if defined(_DEBUG)
	ID3D12Debug* debug;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))){
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
	isPMD = false;
	// ���f���ǂݍ���
	pmdModel = std::make_shared<PMDmodel>(_dev, "model/PMD/�����~�N.pmd");
	//pmxModels.emplace_back(std::make_shared<PMXmodel>(_dev, "model/PMX/���уt����/���уt����.pmx", "VMD/45�bMIKU.vmd"));
	//pmxModels.emplace_back(std::make_shared<PMXmodel>(_dev, "model/PMX/2B/na_2b_0407.pmx", "VMD/���S�R���_���X.vmd"));
	//pmxModels.emplace_back(std::make_shared<PMXmodel>(_dev, "model/PMX/GUMI/GUMI��_V3.pmx", "VMD/DanceRobotDance_Motion.vmd"));
	//pmxModels.emplace_back(std::make_shared<PMXmodel>(_dev, "model/PMX/���у��[�~�A/���у��[�~�A�W���{�[��.pmx", "VMD/45�bGUMI.vmd"));
	pmxModels.emplace_back(std::make_shared<PMXmodel>(_dev, "model/PMX/���у��[�~�A/���у��[�~�A.pmx", "VMD/45�bMIKU.vmd"));


	// �X���b�v�`�F�C���̐���
	if (FAILED(CreateSwapChainAndCmdQue())) 
		return 2;

	// �R�}���h�n�̐���
	if (FAILED(CreateCmdListAndAlloc())) 
		return 3;

	// �t�F���X�̐���
	if (FAILED(CreateFence())) 
		return 4;

	// �[�x�o�b�t�@�̍쐬
	if (FAILED(CreateDSV())) 
		return 7;

	// �J�����̍쐬
	if (FAILED(CreateConstantBuffer())) 
		return 8;

	// �ꖇ�ڃ|���S���̐���
	if (FAILED(CreateFirstPolygon()))
		return 9;

	if (FAILED(CreateFirstSignature()))
		return 10;

	if (FAILED(CreateFirstPopelineState()))
		return 11;
	
	// �񖇂߃|���S��
	if (FAILED(CreateSecondPolygon()))
		return 12;

	if (FAILED(CreateSecondSignature()))
		return 13;

	if (FAILED(CreateSecondPopelineState()))
		return 14;



	if (FAILED(CreatePolygonVB()))
		return 15;


	return 0;
}


HRESULT Dx12Wrapper::CreateSecondPolygon()
{
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


	D3D12_CLEAR_VALUE clear = { DXGI_FORMAT_R8G8B8A8_UNORM ,{0.5f,0.5f,0.5f,0.0f}, };

	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		&clear,
		IID_PPV_ARGS(&_peraBuffer2));

	// �����_�[�^�[�Q�b�g�r���[�̍쐬
	_dev->CreateRenderTargetView(_peraBuffer2, nullptr, _rtvDescHeap2->GetCPUDescriptorHandleForHeapStart());

	// �V�F�[�_�[���\�[�X�r���[�̍쐬
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = desc.Format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	_dev->CreateShaderResourceView(_peraBuffer2, &srvDesc, _srvDescHeap2->GetCPUDescriptorHandleForHeapStart());
	return result;
}

// ���t���[���Ăяo���X�V����
void Dx12Wrapper::UpDate()
{
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
		angle += addsize / 10;
	}
	else if (key[DIK_E] || key[DIK_RIGHT])
	{
		angle -= addsize / 10;
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

		angle = 0.f;
	}
	if (key[DIK_P])
	{
		isOk = 0;
	}
	if (key[DIK_P] && (Oldkey[DIK_P] == 0))
	{
		float rota[3] = { XM_PIDIV4 , XM_PIDIV2 ,XM_PI / 30 * XM_PI };
		cnt++;
		_wvp.projection = XMMatrixPerspectiveFovLH(
			rota[cnt % 3],
			static_cast<float>(wsize.width) / static_cast<float>(wsize.height),
			0.1f,
			300.0f
		);
	}

	_wvp.view = DirectX::XMMatrixLookAtLH(
		DirectX::XMLoadFloat3(&eye),
		DirectX::XMLoadFloat3(&target),
		DirectX::XMLoadFloat3(&up)
	);
	pmdModel->UpDate();

	for (auto& model : pmxModels)
	{
		model->UpDate(key);
	}
	// �`�揇�ɕ��ёւ���
	std::sort(pmxModels.begin(), pmxModels.end(), [](std::shared_ptr<PMXmodel>& a, std::shared_ptr<PMXmodel>& b)
	{
		return (a->GetPos().z > b->GetPos().z);
	});

	_wvp.view = DirectX::XMMatrixRotationY(angle)*_wvp.view;

	_wvp.wvp = _wvp.world;
	_wvp.wvp *= _wvp.view;
	_wvp.wvp *= _wvp.projection;
	*_wvpMP = _wvp;

	D3D12_RESOURCE_BARRIER BarrierDesc = {};
	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.Transition.pResource = _peraBuffer;
	BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	// �[�x�o�b�t�@�r���[�q�[�v�̊J�n�ʒu���Ƃ��Ă���
	auto depthStart = _dsvDescHeap->GetCPUDescriptorHandleForHeapStart();
	auto peraStart = _rtvDescHeap->GetCPUDescriptorHandleForHeapStart();
	_cmdAlloc->Reset();
	_cmdList->Reset(_cmdAlloc, nullptr);//�R�}���h���X�g���Z�b�g

	// ���\�[�X�o���A
	_cmdList->ResourceBarrier(1, &BarrierDesc);

	//�����_�[�^�[�Q�b�g�ݒ�
	_cmdList->OMSetRenderTargets(1, &peraStart, false, &depthStart);

	//�����_�[�^�[�Q�b�g�̃N���A
	_cmdList->ClearRenderTargetView(peraStart, clearColor, 0, nullptr);

	for (auto &model : pmxModels)
	{
		model->Draw(_cmdList, depthStart, _cameraDescHeap);
	}

	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

	// ���\�[�X�o���A
	_cmdList->ResourceBarrier(1, &BarrierDesc);

	//�R�}���h�̃N���[�Y
	_cmdList->Close();

	// �R�}���h�ǉ��ƃt�F���X
	ExecuteCommand();

	// �҂�
	WaitWithFence();

	// �|���S���̕`��
	//DrawFirstPolygon();

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
// �؂�|����`�悷��
void Dx12Wrapper::DrawFirstPolygon()
{
	// �q�[�v�̊J�n�ʒu���擾
	auto heapStart = _rtvDescHeap->GetCPUDescriptorHandleForHeapStart();
	auto depthStart = _dsvDescHeap->GetCPUDescriptorHandleForHeapStart();

	// �o���A�̍쐬------------//
	D3D12_RESOURCE_BARRIER BarrierDesc = {};
	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.Transition.pResource = _peraBuffer;
	BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

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
	_cmdList->OMSetRenderTargets(1, &heapStart, false, &depthStart);

	// �����_�[�^�[�Q�b�g�̃N���A
	_cmdList->ClearRenderTargetView(heapStart, clearColor, 0, nullptr);

	// ���_�o�b�t�@�r���[�̐ݒ�
	_cmdList->IASetVertexBuffers(0, 1, &_peravbView);

	// �g�|���W�[�̃Z�b�g
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// �f�X�N���v�^�q�[�v�̃Z�b�g
	_cmdList->SetDescriptorHeaps(1,&_srvDescHeap2);

	// �f�X�N���v�^�e�[�u���̃Z�b�g
	_cmdList->SetGraphicsRootDescriptorTable(0, _srvDescHeap2->GetGPUDescriptorHandleForHeapStart());

	// �|���S���̕`��
	_cmdList->DrawInstanced(4, 1, 0, 0);

	// �o���A�̐؂�ւ�
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

	// ���\�[�X�o���A
	_cmdList->ResourceBarrier(1, &BarrierDesc);

	//�R�}���h�̃N���[�Y
	_cmdList->Close();
}

void Dx12Wrapper::DrawSecondPolygon()
{
	// �q�[�v�̊J�n�ʒu���擾
	auto heapStart = _swcDescHeap->GetCPUDescriptorHandleForHeapStart();

	// �o�b�N�o�b�t�@�̃C���f�b�N�X���Ƃ��Ă��ē���ւ���
	auto bbIndex = _swapchain->GetCurrentBackBufferIndex();
	int DescriptorSize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	heapStart.ptr += bbIndex * DescriptorSize;

	// �o���A�̍쐬------------//
	D3D12_RESOURCE_BARRIER BarrierDesc = {};
	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.Transition.pResource = renderTargets[bbIndex];
	BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	_cmdAlloc->Reset();
	_cmdList->Reset(_cmdAlloc, nullptr);

	// �p�C�v���C���̃Z�b�g
	_cmdList->SetPipelineState(_peraPipeline2);

	// ���[�g�V�O�l�`�����Z�b�g
	_cmdList->SetGraphicsRootSignature(_peraSignature2);

	//�r���[�|�[�g�ƃV�U�[�ݒ�
	_cmdList->RSSetViewports(1, &_viewPort);
	_cmdList->RSSetScissorRects(1, &_scissor);

	// ���\�[�X�o���A
	_cmdList->ResourceBarrier(1, &BarrierDesc);

	// �����_�[�^�[�Q�b�g�̐ݒ�
	_cmdList->OMSetRenderTargets(1, &heapStart, false, &_dsvDescHeap->GetCPUDescriptorHandleForHeapStart());

	// �����_�[�^�[�Q�b�g�̃N���A
	_cmdList->ClearRenderTargetView(heapStart, clearColor, 0, nullptr);

	// ���_�o�b�t�@�r���[�̐ݒ�
	_cmdList->IASetVertexBuffers(0, 1, &_peravbView);

	// �g�|���W�[�̃Z�b�g
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// �f�X�N���v�^�q�[�v�̃Z�b�g
	_cmdList->SetDescriptorHeaps(1, &_srvDescHeap);

	// �f�X�N���v�^�e�[�u���̃Z�b�g
	_cmdList->SetGraphicsRootDescriptorTable(0, _srvDescHeap->GetGPUDescriptorHandleForHeapStart());

	// �|���S���̕`��
	_cmdList->DrawInstanced(4, 1, 0, 0);

	// �o���A�̐؂�ւ�
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

	// ���\�[�X�o���A
	_cmdList->ResourceBarrier(1, &BarrierDesc);

	//�R�}���h�̃N���[�Y
	_cmdList->Close();
}

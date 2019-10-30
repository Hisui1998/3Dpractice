#include "Dx12Wrapper.h"
#include "Application.h"
#include <wrl.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXTex.h>
#include "PMDmodel.h"
#include "PMXmodel.h"

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"DirectXTex.lib")

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
			//pmdModel = std::make_shared<PMDmodel>(_dev, "model/PMD/�����~�Nmetal.pmd");
			//pmxModel = std::make_shared<PMXmodel>(_dev, "model/PMX/GUMI/GUMI��_V3.pmx");
			//pmxModel = std::make_shared<PMXmodel>(_dev, "model/PMX/���уt����/���уt�����W���{�[��.pmx");
			pmxModel = std::make_shared<PMXmodel>(_dev, "model/PMX/���у��[�~�A/���у��[�~�A�W���{�[��.pmx");
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
		for (unsigned int i = 0; i < renderTargetsNum; i++)
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

HRESULT Dx12Wrapper::LoadShader()
{
	HRESULT result;

	// ���_�V�F�[�_
	result = D3DCompileFromFile(L"Shader.hlsl", nullptr, nullptr, "vs", "vs_5_0", D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION, 0, &vertexShader, nullptr);

	// �s�N�Z���V�F�[�_
	result = D3DCompileFromFile(L"Shader.hlsl", nullptr, nullptr, "ps", "ps_5_0", D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pixelShader, nullptr);

	return result;
}

// �V�O�l�`���̐ݒ�
HRESULT Dx12Wrapper::CreateRootSignature()
{
	HRESULT result;

	// �T���v���̐ݒ�
	D3D12_STATIC_SAMPLER_DESC SamplerDesc[2] = {};
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

	SamplerDesc[1] = SamplerDesc[0];//�ύX�_�ȊO���R�s�[
	SamplerDesc[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;//�J��Ԃ��Ȃ�
	SamplerDesc[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;//�J��Ԃ��Ȃ�
	SamplerDesc[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;//�J��Ԃ��Ȃ�
	SamplerDesc[1].ShaderRegister = 1; //�V�F�[�_�X���b�g�ԍ���Y��Ȃ��悤��

	// �����W�̐ݒ�
	D3D12_DESCRIPTOR_RANGE descRange[4] = {};// �e�N�X�`���ƒ萔��Ƃځ[��
	descRange[0].NumDescriptors = 1;
	descRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;//�萔
	descRange[0].BaseShaderRegister = 0;//���W�X�^�ԍ�
	descRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	descRange[1].NumDescriptors = 1;
	descRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;//�萔
	descRange[1].BaseShaderRegister = 1;//���W�X�^�ԍ�
	descRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// �e�N�X�`���[
	descRange[2].NumDescriptors = 4;// �e�N�X�`���ƃX�t�B�A�̓�ƃg�D�[��
	descRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//�Ă�������
	descRange[2].BaseShaderRegister = 0;//���W�X�^�ԍ�
	descRange[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// �{�[��
	descRange[3].NumDescriptors = 1;
	descRange[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descRange[3].BaseShaderRegister = 2;
	descRange[3].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ���[�g�p�����[�^�ϐ��̐ݒ�
	D3D12_ROOT_PARAMETER rootParam[3] = {};
	rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[0].DescriptorTable.pDescriptorRanges = &descRange[0];//�Ή����郌���W�ւ̃|�C���^
	rootParam[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//���ׂẴV�F�[�_����Q��

	// �e�N�X�`��
	rootParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[1].DescriptorTable.pDescriptorRanges = &descRange[1];//�Ή����郌���W�ւ̃|�C���^
	rootParam[1].DescriptorTable.NumDescriptorRanges = 2;
	rootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//�s�N�Z���V�F�[�_����Q��

	// �{�[��
	rootParam[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[2].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[2].DescriptorTable.pDescriptorRanges = &descRange[3];
	rootParam[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	// ���[�g�V�O�l�`������邽�߂̕ϐ��̐ݒ�
	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.pStaticSamplers = SamplerDesc;
	rsd.pParameters = rootParam;
	rsd.NumParameters = 3;
	rsd.NumStaticSamplers = 2;

	result = D3D12SerializeRootSignature(
		&rsd, 
		D3D_ROOT_SIGNATURE_VERSION_1, 
		&signature, 
		&error);

	// ���[�g�V�O�l�`���{�̂̍쐬
	result = _dev->CreateRootSignature(
		0,
		signature->GetBufferPointer(),
		signature->GetBufferSize(),
		IID_PPV_ARGS(&_rootSignature));	

	return result;
}

// GraphicsPipelineState(�`��p�p�C�v���C��)�̍쐬
HRESULT Dx12Wrapper::CreateGraphicsPipelineState()
{
	HRESULT result;	
	// ���_���C�A�E�g (�\���̂Ə��Ԃ����킹�邱��)
	D3D12_INPUT_ELEMENT_DESC inputLayoutDescs[] = {
		// ���W
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0, D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		// �@��
		{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		// UV
		{"TEXCOORD0",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		// �ǉ�UV
		{"TEXCOORD1",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TEXCOORD2",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TEXCOORD3",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TEXCOORD4",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		// wighttype
		{"WEIGHTTYPE",0,DXGI_FORMAT_R8_SINT,0,D3D12_APPEND_ALIGNED_ELEMENT ,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		// �{�[���C���f�b�N�X
		{"BONENO1",0,DXGI_FORMAT_R16G16_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"BONENO2",0,DXGI_FORMAT_R16G16_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"BONENO3",0,DXGI_FORMAT_R16G16_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"BONENO4",0,DXGI_FORMAT_R16G16_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		// weight
		{"WEIGHT1",0,DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"WEIGHT2",0,DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"WEIGHT3",0,DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"WEIGHT4",0,DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};

	// �p�C�v���C������邽�߂�GPS�̕ϐ��̍쐬
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};

	//���[�g�V�O�l�`���ƒ��_���C�A�E�g
	gpsDesc.pRootSignature = _rootSignature;
	gpsDesc.InputLayout.pInputElementDescs = inputLayoutDescs;
	gpsDesc.InputLayout.NumElements = _countof(inputLayoutDescs);

	//�V�F�[�_�n
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader);

	//�����_�[�^�[�Q�b�g
	gpsDesc.NumRenderTargets = 1;
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	//�[�x�X�e���V��
	gpsDesc.DepthStencilState.DepthEnable = true;
	gpsDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;// �K�{
	gpsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;// �������ق���ʂ�

	gpsDesc.DepthStencilState.StencilEnable = false;
	gpsDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	//���X�^���C�U
	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	//�����_�[�^�[�Q�b�g�u�����h�ݒ�p�\����
	D3D12_RENDER_TARGET_BLEND_DESC renderBlend = {};
	renderBlend.BlendEnable = true;
	renderBlend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	renderBlend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	renderBlend.BlendOp = D3D12_BLEND_OP_ADD;
	renderBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
	renderBlend.DestBlendAlpha = D3D12_BLEND_ZERO;
	renderBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	renderBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	//�u�����h�X�e�[�g�ݒ�p�\����
	D3D12_BLEND_DESC blend = {};
	blend.AlphaToCoverageEnable = false;
	blend.IndependentBlendEnable = false;
	blend.RenderTarget[0] = renderBlend;

	//���̑�
	gpsDesc.BlendState = blend;
	gpsDesc.NodeMask = 0;
	gpsDesc.SampleDesc.Count = 1;
	gpsDesc.SampleDesc.Quality = 0;
	gpsDesc.SampleMask = 0xffffffff;
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//�O�p�`

	result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&_pipelineState));
	return result;
}

HRESULT Dx12Wrapper::CreateBuffersForIndexAndVertex()
{
	// �q�[�v�̏��ݒ�
	D3D12_HEAP_PROPERTIES heapprop = {};
	heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;//CPU����GPU�֓]������p
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	auto vertexinfo = pmxModel->GetVertexInfo();
	auto vertexindex = pmxModel->GetVertexIndex();

	// create
	auto result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,//���ʂȎw��Ȃ�
		&CD3DX12_RESOURCE_DESC::Buffer(vertexinfo.size() * sizeof(vertexinfo[0])),
		D3D12_RESOURCE_STATE_GENERIC_READ,//��݂���
		nullptr,//nullptr�ł���
		IID_PPV_ARGS(&_vertexBuffer));//������

	result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vertexindex.size() * sizeof(vertexindex[0])),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_indexBuffer));
	
	// ���_�o�b�t�@�̃}�b�s���O
	PMXVertexInfo* vertMap = nullptr;
	result = _vertexBuffer->Map(0, nullptr, (void**)&vertMap);
	std::copy(vertexinfo.begin(), vertexinfo.end(), vertMap);

	_vertexBuffer->Unmap(0, nullptr);

	_vbView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
	_vbView.StrideInBytes = sizeof(PMXVertexInfo);
	_vbView.SizeInBytes = pmxModel->GetVertexInfo().size()* sizeof(PMXVertexInfo);

	// �C���f�b�N�X�o�b�t�@�̃}�b�s���O
	unsigned short* idxMap = nullptr;
	result = _indexBuffer->Map(0, nullptr, (void**)&idxMap);
	std::copy(std::begin(vertexindex), std::end(vertexindex), idxMap);
	_indexBuffer->Unmap(0, nullptr);

	_ibView.BufferLocation = _indexBuffer->GetGPUVirtualAddress();//�o�b�t�@�̏ꏊ
	_ibView.Format = DXGI_FORMAT_R16_UINT;//�t�H�[�}�b�g(short������R16)
	_ibView.SizeInBytes = vertexindex.size() * sizeof(vertexindex[0]);//���T�C�Y

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

	auto result = _dev->CreateDescriptorHeap(&rgstDesc,IID_PPV_ARGS(&_rgstDescHeap));
	auto wsize = Application::Instance().GetWindowSize();

	// ���W�̏����l
	eye = XMFLOAT3(0,10.5f,-5);
	target = XMFLOAT3(0, 10.5f,0);
	up = XMFLOAT3(0,1,0);

	angle = 0.f;

	_wvp.world = DirectX::XMMatrixRotationY(angle);

	_wvp.view = DirectX::XMMatrixLookAtLH(
		DirectX::XMLoadFloat3(&eye),
		DirectX::XMLoadFloat3(&target),
		DirectX::XMLoadFloat3(&up)
	);

	_wvp.projection = XMMatrixPerspectiveFovLH(
		XM_PIDIV2,
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
		IID_PPV_ARGS(&_constBuff));

	// Mapping
	result = _constBuff->Map(0,nullptr,(void**)&_wvpMP);
	std::memcpy(_wvpMP,&_wvp,sizeof(_wvp));
	//_constBuff->Unmap(0, nullptr);

	// �R���X�^���g�o�b�t�@�p�̃f�X�N
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _constBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = size;

	auto handle = _rgstDescHeap->GetCPUDescriptorHandleForHeapStart();
	// �R���X�^���g�o�b�t�@�r���[�̍쐬
	_dev->CreateConstantBufferView(&cbvDesc, handle);

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
	_depthClearValue.DepthStencil.Depth = 0.0f;
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
	auto result = CoInitializeEx(0, COINIT_MULTITHREADED);
}

Dx12Wrapper::~Dx12Wrapper()
{
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
	// �f�o�C�X�̏�����
	if (FAILED(DeviceInit())) 
		return 1;

	// �X���b�v�`�F�C���̏�����
	if (FAILED(CreateSwapChainAndCmdQue())) 
		return 2;

	// �R�}���h�n�̏�����
	if (FAILED(CreateCmdListAndAlloc())) 
		return 3;

	// �t�F���X�̏�����
	if (FAILED(CreateFence())) 
		return 4;

	// PMD�̃��[�h
	/*if (FAILED(LoadPMD())) 
		return 5;*/

	// �C���f�b�N�X�ƒ��_�o�b�t�@�̍쐬
	if (FAILED(CreateBuffersForIndexAndVertex())) 
		return 6;

	// �V�F�[�_�[�̏�����
	if (FAILED(LoadShader()))
		return 7;

	// �V�O�l�`���̏�����
	if (FAILED(CreateRootSignature())) 
		return 8;

	// �p�C�v���C���̍쐬
	if (FAILED(CreateGraphicsPipelineState()))
		return 9;

	// �[�x�o�b�t�@�̍쐬
	if (FAILED(CreateDSV())) 
		return 10;

	// �萔�o�b�t�@�̍쐬
	if (FAILED(CreateConstantBuffer())) 
		return 11;

	return 0;
}

// ���t���[���Ăяo���X�V����
void Dx12Wrapper::UpDate()
{
	auto wsize = Application::Instance().GetWindowSize();

	// �L�[�̓���
	char Oldkey[256];
	int i = 0;
	for (auto k : key)
	{
		Oldkey[i++] = k;
	}

	auto isOk = _keyBoadDev->GetDeviceState(sizeof(key), key);
	if (FAILED(isOk)) {
		// ���s�Ȃ�ĊJ�����Ă�����x�擾
		_keyBoadDev->Acquire();
		_keyBoadDev->GetDeviceState(sizeof(key), key);
	}

	float addsize = 0.1f;

	if (key[DIK_UP])
	{
		eye.y += addsize;
		target.y += addsize;
	}
	else if (key[DIK_DOWN])
	{
		eye.y-=addsize;
		target.y-=addsize;
	}

	if (key[DIK_Q]|| key[DIK_LEFT])
	{
		angle += 0.01f;
	}
	else if (key[DIK_E] || key[DIK_RIGHT])
	{
		angle -= 0.01f;
	}

	if (key[DIK_W])
	{
		eye.z+= addsize;
		target.z+= addsize;
	}
	else if (key[DIK_S])
	{
		eye.z-= addsize;
		target.z-= addsize;
	}

	if (key[DIK_D])
	{
		eye.x+= addsize;
		target.x+= addsize;
	}
	if (key[DIK_A])
	{
		eye.x-= addsize;
		target.x-= addsize;
	}

	if (key[DIK_R])
	{
		eye = XMFLOAT3(0, 10.5f, -5);
		target = XMFLOAT3(0, 10.5f, 0);
		up = XMFLOAT3(0, 1, 0);

		angle = 0.f;
	}
	if (key[DIK_P])
	{
		isOk = 0;
	}
	if (key[DIK_P]&&(Oldkey[DIK_P]==0))
	{
		float rota[3] = { XM_PIDIV2 ,XM_PIDIV4 ,XM_PIDIV4 / 2 };
		cnt++;
		_wvp.projection = XMMatrixPerspectiveFovLH(
			rota[cnt%3],
			static_cast<float>(wsize.width) / static_cast<float>(wsize.height),
			0.1f,
			300.0f
		);
	}

	// �㔼�g
	_wvp.view = DirectX::XMMatrixLookAtLH(
		DirectX::XMLoadFloat3(&eye),
		DirectX::XMLoadFloat3(&target),
		DirectX::XMLoadFloat3(&up)
	);
	pmxModel->UpDate(key);

	// ��]������
	_wvp.view = DirectX::XMMatrixRotationY(angle)*_wvp.view;

	_wvp.wvp = _wvp.world;
	_wvp.wvp *= _wvp.view;
	_wvp.wvp *= _wvp.projection;
	*_wvpMP = _wvp;
	

	auto heapStart = _swcDescHeap->GetCPUDescriptorHandleForHeapStart();
	float clearColor[] = { 0.5f,0.5f,0.5f,1.0f };//�N���A�J���[�ݒ�

	// �r���[�|�[�g
	D3D12_VIEWPORT _viewport;
	_viewport.TopLeftX = 0;
	_viewport.TopLeftY = 0;
	_viewport.Width  = Application::Instance().GetWindowSize().width;
	_viewport.Height = Application::Instance().GetWindowSize().height;
	_viewport.MaxDepth = 1.0f;
	_viewport.MinDepth = 0.0f;

	// �V�U�[�i����Ƃ�H�j
	D3D12_RECT _scissorRect;
	_scissorRect.left = 0;
	_scissorRect.top = 0;
	_scissorRect.right = Application::Instance().GetWindowSize().width;
	_scissorRect.bottom = Application::Instance().GetWindowSize().height;

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

	_cmdAlloc->Reset();//�A���P�[�^���Z�b�g
	_cmdList->Reset(_cmdAlloc, nullptr);//�R�}���h���X�g���Z�b�g

	// �p�C�v���C���̃Z�b�g
	_cmdList->SetPipelineState(_pipelineState);

	// ���[�g�V�O�l�`�����Z�b�g
	_cmdList->SetGraphicsRootSignature(_rootSignature);

	// �ŃX�N���v�^�[�q�[�v�̃Z�b�g(���W�X�^)
	_cmdList->SetDescriptorHeaps(1, &_rgstDescHeap);

	// �f�X�N���v�^�e�[�u���̃Z�b�g(���W�X�^)
	D3D12_GPU_DESCRIPTOR_HANDLE rgpuStart = _rgstDescHeap->GetGPUDescriptorHandleForHeapStart();
	_cmdList->SetGraphicsRootDescriptorTable(0, rgpuStart);
	
	//�r���[�|�[�g�ƃV�U�[�ݒ�
	_cmdList->RSSetViewports(1, &_viewport);
	_cmdList->RSSetScissorRects(1, &_scissorRect);

	// ���\�[�X�o���A
	_cmdList->ResourceBarrier(1, &BarrierDesc);

	//�����_�[�^�[�Q�b�g�ݒ�
	_cmdList->OMSetRenderTargets(1, &heapStart, false, &_dsvDescHeap->GetCPUDescriptorHandleForHeapStart());

	//�N���A
	_cmdList->ClearRenderTargetView(heapStart, clearColor, 0, nullptr);

	// �łՂ��̃N���A
	_cmdList->ClearDepthStencilView(_dsvDescHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

	// �C���f�b�N�X���̃Z�b�g
	_cmdList->IASetIndexBuffer(&_ibView);

	// ���_�o�b�t�@�r���[�̐ݒ�
	_cmdList->IASetVertexBuffers(0, 1, &_vbView);

	// �g�|���W�[�̃Z�b�g
	_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// ���f���̃}�e���A���K�p
	// �ǂ̃C���f�b�N�X����n�߂邩�����Ă����ϐ�
	unsigned int offset = 0;
	auto boneheap = pmxModel->GetBoneHeap();
	auto materialheap = pmxModel->GetMaterialHeap();

	// �f�X�N���v�^�[�n���h���ꖇ�̃T�C�Y�擾
	int incsize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	/*�@�{�[���𓮂����@*/

	// �n���h���̎擾
	auto bohandle = boneheap->GetGPUDescriptorHandleForHeapStart();

	// �f�X�N���v�^�q�[�v�̃Z�b�g
	_cmdList->SetDescriptorHeaps(1, &boneheap);

	// �f�X�N���v�^�e�[�u���̃Z�b�g
	_cmdList->SetGraphicsRootDescriptorTable(2, bohandle);


	/*�@�}�e���A���̓K�p�@*/

	// �n���h���̎擾
	auto mathandle = materialheap->GetGPUDescriptorHandleForHeapStart();

	// �f�X�N���v�^�q�[�v�̃Z�b�g
	_cmdList->SetDescriptorHeaps(1, &materialheap);


	// �`�惋�[�v
	for (auto& m : pmxModel->GetMaterials()) {
		// �f�X�N���v�^�e�[�u���̃Z�b�g
		_cmdList->SetGraphicsRootDescriptorTable(1, mathandle);

		// �`�敔
		_cmdList->DrawIndexedInstanced(m.faceVerCnt, 1, offset, 0, 0);

		// �|�C���^�̉��Z
		mathandle.ptr += incsize*5;// 5�����邩��5�{
		
		// �ϐ��̉��Z
		offset += m.faceVerCnt;
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

	_swapchain->Present(0, 0);
}

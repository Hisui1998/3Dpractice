#include "Dx12Wrapper.h"
#include "Application.h"
#include <wrl.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXTex.h>

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
			break;
		}
	}
	result = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgi));
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
	D3D12_STATIC_SAMPLER_DESC SamplerDesc = {};
	SamplerDesc.Filter = D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT;// ���ʂȃt�B���^���g�p���Ȃ�
	SamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;// �悪�J��Ԃ��`�悳���
	SamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	SamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;// ����Ȃ�
	SamplerDesc.MinLOD = 0.0f;// �����Ȃ�
	SamplerDesc.MipLODBias = 0.0f;// MIPMAP�̃o�C�A�X
	SamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;// �G�b�W�̐F(��)
	SamplerDesc.ShaderRegister = 0;// �g�p���郌�W�X�^�X���b�g
	SamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;// �ǂ̂��炢�̃f�[�^���V�F�[�_�Ɍ����邩(�S��)
	SamplerDesc.RegisterSpace = 0;// �킩���
	SamplerDesc.MaxAnisotropy = 0;// Filter��Anisotropy�̂Ƃ��̂ݗL��
	SamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;// ��ɔے�

	// �����W�̐ݒ�
	D3D12_DESCRIPTOR_RANGE descRange[3] = {};
	descRange[0].NumDescriptors = 1;
	descRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;//�萔
	descRange[0].BaseShaderRegister = 0;//���W�X�^�ԍ�
	descRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	descRange[1].NumDescriptors = 1;
	descRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;//�萔
	descRange[1].BaseShaderRegister = 1;//���W�X�^�ԍ�
	descRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// �e�N�X�`�������W
	descRange[2].NumDescriptors = 3;// �e�N�X�`���ƃX�t�B�A�̓��
	descRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//�Ă�������
	descRange[2].BaseShaderRegister = 0;//���W�X�^�ԍ�
	descRange[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ���[�g�p�����[�^�ϐ��̐ݒ�
	D3D12_ROOT_PARAMETER rootParam[2] = {};
	rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[0].DescriptorTable.pDescriptorRanges = &descRange[0];//�Ή����郌���W�ւ̃|�C���^
	rootParam[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//���ׂẴV�F�[�_����Q��

	rootParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[1].DescriptorTable.pDescriptorRanges = &descRange[1];//�Ή����郌���W�ւ̃|�C���^
	rootParam[1].DescriptorTable.NumDescriptorRanges = 2;
	rootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//�s�N�Z���V�F�[�_����Q��

	// ���[�g�V�O�l�`������邽�߂̕ϐ��̐ݒ�
	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.pStaticSamplers = &SamplerDesc;
	rsd.pParameters = rootParam;
	rsd.NumParameters = 2;
	rsd.NumStaticSamplers = 1;

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
		{
			"POSITION",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
		{
			"NORMAL",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
		{
			"TEXCOORD",
			0,
			DXGI_FORMAT_R32G32_FLOAT,
			0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		},
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

	//���̑�
	gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.NodeMask = 0;
	gpsDesc.SampleDesc.Count = 1;
	gpsDesc.SampleDesc.Quality = 0;
	gpsDesc.SampleMask = 0xffffffff;
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//�O�p�`

	result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&_pipelineState));
	return result;
}

// PMD�t�@�C���̓ǂݍ���
HRESULT Dx12Wrapper::LoadPMD()
{
	FILE*fp;
	PMDHeader data;
	std::string ModelPath = "model/�����~�Nmetal.pmd";

	fopen_s(&fp, ModelPath.c_str(),"rb");

	// �w�b�_�[���̓ǂݍ���
	fread(&data.signature, sizeof(data.signature), 1, fp);
	fread(&data.version, sizeof(PMDHeader) - sizeof(data.signature)-1, 1, fp);

	// ���_���̓ǂݍ���
	unsigned int vnum = 0;
	fread(&vnum,sizeof(vnum),1,fp);

	_vivec.resize(vnum);

	for (auto& vi:_vivec)
	{
		fread(&vi, sizeof(VertexInfo), 1, fp);
	}

	// �C���f�b�N�X���̓ǂݍ���
	unsigned int inum = 0;
	fread(&inum, sizeof(unsigned int), 1, fp);
	verindex.resize(inum);

	for (auto& vidx:verindex)
	{
		fread(&vidx,sizeof(unsigned short),1,fp);
	}

	// �}�e���A���̓ǂݍ���
	unsigned int mnum = 0;
	fread(&mnum,sizeof(unsigned int),1,fp);
	_materials.resize(mnum);

	for (auto& mat:_materials)
	{
		fread(&mat, sizeof(PMDMaterial), 1, fp);
	}

	// ����Ă����������
	auto result = CreateWhiteTexture();
	// ����Ă����������
	result = CreateBlackTexture();

	// �e�N�X�`���̓ǂݍ��݂ƃo�b�t�@�̍쐬
	_textureBuffer.resize(mnum);
	_sphBuffer.resize(mnum);
	_spaBuffer.resize(mnum);

	for (int i = 0; i < _materials.size(); ++i) {
		// �e�N�X�`���t�@�C���p�X�̎擾
		std::string texFileName = _materials[i].texFileName;
		
		if (std::count(texFileName.begin(),texFileName.end(),'*')>0)
		{
			auto namepair = SplitFileName(texFileName);
			if (GetExtension(namepair.first) == "sph"||
				GetExtension(namepair.first) == "spa")
			{
				texFileName = namepair.second;
			}
			else
			{
				texFileName = namepair.first;
			}
		}
		auto texFilePath = GetTexPath(ModelPath, texFileName.c_str());

		// �e�N�X�`���o�b�t�@���쐬���ē����
		if (GetExtension(texFileName) == "sph")
		{
			_sphBuffer[i] = LoadTextureFromFile(texFilePath);
		}
		else if (GetExtension(texFileName) == "spa")
		{
			_spaBuffer[i] = LoadTextureFromFile(texFilePath);
		}
		else
		{
			_textureBuffer[i] = LoadTextureFromFile(texFilePath);
		}
	}

	fclose(fp);
	return result;
}

HRESULT Dx12Wrapper::CreateBuffersForIndexAndVertex()
{
	HRESULT result = S_OK;

	// �q�[�v�̏��ݒ�
	D3D12_HEAP_PROPERTIES heapprop = {};
	heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;//CPU����GPU�֓]������p
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	// create
	result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,//���ʂȎw��Ȃ�
		&CD3DX12_RESOURCE_DESC::Buffer(_vivec.size() * sizeof(_vivec[0])),
		D3D12_RESOURCE_STATE_GENERIC_READ,//��݂���
		nullptr,//nullptr�ł���
		IID_PPV_ARGS(&_vertexBuffer));//������

	result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(verindex.size() * sizeof(verindex[0])),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_indexBuffer));
	
	// ���_�o�b�t�@�̃}�b�s���O
	VertexInfo* vertMap = nullptr;
	result = _vertexBuffer->Map(0, nullptr, (void**)&vertMap);
	std::copy(std::begin(_vivec), std::end(_vivec), vertMap);

	_vertexBuffer->Unmap(0, nullptr);

	_vbView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
	_vbView.StrideInBytes = sizeof(VertexInfo);
	_vbView.SizeInBytes = _vivec.size()* sizeof(VertexInfo);

	// �C���f�b�N�X�o�b�t�@�̃}�b�s���O
	unsigned short* idxMap = nullptr;
	result = _indexBuffer->Map(0, nullptr, (void**)&idxMap);
	std::copy(std::begin(verindex), std::end(verindex), idxMap);
	_indexBuffer->Unmap(0, nullptr);

	_ibView.BufferLocation = _indexBuffer->GetGPUVirtualAddress();//�o�b�t�@�̏ꏊ
	_ibView.Format = DXGI_FORMAT_R16_UINT;//�t�H�[�}�b�g(short������R16)
	_ibView.SizeInBytes = verindex.size() * sizeof(verindex[0]);//���T�C�Y

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


	DirectX::XMFLOAT3 eye(10,15,15);
	DirectX::XMFLOAT3 target(0,10,0);
	DirectX::XMFLOAT3 up(0,1,0);

	angle = 0.0f;

	_wvp.world = DirectX::XMMatrixRotationY(angle);

	_wvp.view = DirectX::XMMatrixLookAtLH(
		DirectX::XMLoadFloat3(&eye),
		DirectX::XMLoadFloat3(&target),
		DirectX::XMLoadFloat3(&up)
	);

	_wvp.projection = DirectX::XMMatrixPerspectiveFovLH(
		DirectX::XM_PIDIV2,
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
;
	
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

HRESULT Dx12Wrapper::CreateMaterialBuffer()
{	
	D3D12_DESCRIPTOR_HEAP_DESC matDescHeap = {};
	matDescHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	matDescHeap.NodeMask = 0;
	// �萔�o�b�t�@�ƃV�F�[�_�[���\�[�X�r���[��Sph��Spa�̂S����
	matDescHeap.NumDescriptors = _materials.size()*4;
	matDescHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	auto result = _dev->CreateDescriptorHeap(&matDescHeap, IID_PPV_ARGS(&_matDescHeap));

	size_t size = (sizeof(PMDMaterial) + 0xff)&~0xff;

	_materialsBuff.resize(_materials.size());

	int midx = 0;
	for (auto& mbuff : _materialsBuff) {

		// �}�e���A���o�b�t�@�̍쐬
		auto result = _dev->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(size),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mbuff));

		// �}�e���A���o�b�t�@�̃}�b�s���O
		result = mbuff->Map(0, nullptr, (void**)&MapColor);

		MapColor->diffuse.x = _materials[midx].diffuse_color.x;
		MapColor->diffuse.y = _materials[midx].diffuse_color.y;
		MapColor->diffuse.z = _materials[midx].diffuse_color.z;
		MapColor->diffuse.w = _materials[midx].alpha;

		MapColor->ambient.x = _materials[midx].mirror_color.x;
		MapColor->ambient.y = _materials[midx].mirror_color.y;
		MapColor->ambient.z = _materials[midx].mirror_color.z;

		MapColor->specular.x = _materials[midx].specular_color.x;
		MapColor->specular.y = _materials[midx].specular_color.y;
		MapColor->specular.z = _materials[midx].specular_color.z;
		MapColor->specular.w = _materials[midx].alpha;

		mbuff->Unmap(0, nullptr);
		++midx;
	}
	
	// �萔�o�b�t�@�r���[�f�X�N�̍쐬
	D3D12_CONSTANT_BUFFER_VIEW_DESC matDesc = {};

	// �V�F�[�_�[���\�[�X�r���[�f�X�N�̍쐬
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	auto matHandle = _matDescHeap->GetCPUDescriptorHandleForHeapStart();
	auto addsize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	

	for (int i = 0;i< _materialsBuff.size();++i)
	{
		// �萔�o�b�t�@�̍쐬
		matDesc.BufferLocation = _materialsBuff[i]->GetGPUVirtualAddress();
		matDesc.SizeInBytes = size;
		_dev->CreateConstantBufferView(&matDesc, matHandle);// �萔�o�b�t�@�r���[�̍쐬
		matHandle.ptr += addsize;// �|�C���^�̉��Z
		
		// �e�N�X�`���p�V�F�[�_���\�[�X�r���[�̍쐬
		if (_textureBuffer[i] == nullptr) {
			srvDesc.Format = whiteTex->GetDesc().Format;
			_dev->CreateShaderResourceView(whiteTex, &srvDesc, matHandle);
		}
		else
		{
			srvDesc.Format = _textureBuffer[i]->GetDesc().Format;// �e�N�X�`���̃t�H�[�}�b�g�̎擾
			_dev->CreateShaderResourceView(_textureBuffer[i], &srvDesc, matHandle);// �V�F�[�_�[���\�[�X�r���[�̍쐬
		}
		matHandle.ptr += addsize;// �|�C���^�̉��Z

		// ��Z�X�t�B�A�}�b�vSRV�̍쐬
		if (_sphBuffer[i] == nullptr) {
			srvDesc.Format = whiteTex->GetDesc().Format;
			_dev->CreateShaderResourceView(whiteTex, &srvDesc, matHandle);
		}
		else 
		{
			srvDesc.Format = _sphBuffer[i]->GetDesc().Format;
			_dev->CreateShaderResourceView(_sphBuffer[i], &srvDesc, matHandle);
		}
		matHandle.ptr += addsize;

		// ���Z�X�t�B�A�}�b�vSRV�̍쐬
		if (_spaBuffer[i] == nullptr) {
			srvDesc.Format = blackTex->GetDesc().Format;
			_dev->CreateShaderResourceView(blackTex, &srvDesc, matHandle);
		}
		else 
		{
			srvDesc.Format = _spaBuffer[i]->GetDesc().Format;
			_dev->CreateShaderResourceView(_spaBuffer[i], &srvDesc, matHandle);
		}		matHandle.ptr += addsize;
	}

	return result;
}

HRESULT Dx12Wrapper::CreateWhiteTexture()
{
	// ���o�b�t�@�p�̃f�[�^�z��
	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0xff);//�S��255�Ŗ��߂�

	// �]������p�̃q�[�v�ݒ�
	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	texHeapProp.CreationNodeMask = 0;
	texHeapProp.VisibleNodeMask = 0;

	// �]������p�̃f�X�N�ݒ�
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.Width = 4;
	resDesc.Height = 4;
	resDesc.DepthOrArraySize = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.MipLevels = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	// ���e�N�X�`���̍쐬
	auto result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,//���Ɏw��Ȃ�
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&whiteTex)
	);

	result = whiteTex->WriteToSubresource(0, nullptr, data.data(), 4 * 4, data.size());

	return result;
}

HRESULT Dx12Wrapper::CreateBlackTexture()
{
	// ���o�b�t�@�p�̃f�[�^�z��
	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0);//�S��0�Ŗ��߂�

	// �]������p�̃q�[�v�ݒ�
	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	texHeapProp.CreationNodeMask = 0;
	texHeapProp.VisibleNodeMask = 0;

	// �]������p�̃f�X�N�ݒ�
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.Width = 4;
	resDesc.Height = 4;
	resDesc.DepthOrArraySize = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.MipLevels = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	// ���e�N�X�`���̍쐬
	auto result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,//���Ɏw��Ȃ�
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&blackTex)
	);

	result = blackTex->WriteToSubresource(0, nullptr, data.data(), 4 * 4, data.size());

	return result;
}

std::string Dx12Wrapper::GetTexPath(const std::string & modelPath, const char * texPath)
{
	// �t�H���_�Z�p���[�^���u/�v����Ȃ��āu\\�v�̉\��������̂�2�p�^�[���擾����
	int pathIndex1 = modelPath.rfind('/');
	int pathIndex2 = modelPath.rfind('\\');

	// rfind�֐��͌�����Ȃ������Ƃ���0xffffffff��Ԃ�����2�̃p�X���r����
	int path = max(pathIndex1, pathIndex2);

	// ���f���f�[�^�̓����Ă���t�H���_���t�T���ŒT���Ă���
	std::string folderPath = modelPath.substr(0, path+1);

	// ����
	return std::string(folderPath + texPath);
}

/* �}���`�o�C�g������̃T�C�Y�Ń��C�h��������쐬���āA���̂��ƒ��g���R�s�[���� */
std::wstring Dx12Wrapper::GetWstringFromString(const std::string& str)
{
	//�Ăяo��1���(�����񐔂𓾂�)
	auto num1 = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(),
		-1,
		nullptr,
		0
	);

	// �������������̃��C�h��������쐬
	std::wstring wstr;
	wstr.resize(num1);

	//�Ăяo��2���(�m�ۍς݂�wstr�ɕϊ���������R�s�[)
	auto num2 = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(),
		-1,
		&wstr[0],
		num1
	);

	return wstr;
}

ID3D12Resource * Dx12Wrapper::LoadTextureFromFile(std::string & texPath)
{
	//WIC�e�N�X�`���̃��[�h
	DirectX::TexMetadata metadata = {};
	DirectX::ScratchImage scratchImg = {};

	auto it = _resourceTable.find(texPath);
	if (it != _resourceTable.end()) {
		//�e�[�u���ɓ��ɂ������烍�[�h����̂ł͂Ȃ��}�b�v����
		//���\�[�X��Ԃ�
		return (*it).second;
	}

	// �e�N�X�`���t�@�C���̃��[�h
	auto result = LoadFromWICFile(GetWstringFromString(texPath).c_str(),
		DirectX::WIC_FLAGS_NONE,
		&metadata,
		scratchImg);
	if (FAILED(result)) {
		return whiteTex;// ���s�����甒�e�N�X�`��������
	}

	// �C���[�W�f�[�^�����
	auto img = scratchImg.GetImage(0, 0, 0);

	// �]������p�̃q�[�v�ݒ�
	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	texHeapProp.CreationNodeMask = 0;
	texHeapProp.VisibleNodeMask = 0;

	// ���\�[�X�f�X�N�̍Đݒ�
	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = metadata.format;
	resDesc.Width = metadata.width;
	resDesc.Height = metadata.height;
	resDesc.DepthOrArraySize = metadata.arraySize;
	resDesc.MipLevels = metadata.mipLevels;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;

	// �e�N�X�`���o�b�t�@�̍쐬
	ID3D12Resource* texbuff = nullptr;
	result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,//���Ɏw��Ȃ�
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&texbuff)
	);

	// �]������
	result = texbuff->WriteToSubresource(0,
		nullptr,
		img->pixels,
		img->rowPitch,
		img->slicePitch
	);
	_resourceTable[texPath] = texbuff;
	return texbuff;
}

std::string Dx12Wrapper::GetExtension(const std::string & path)
{
	// �g���q�̎�O�̂ǂ��Ƃ܂ł̕�������擾����
	int idx = path.rfind('.');

	// ��������؂���
	auto p = path.substr(idx+1,path.length()-idx-1);
	return p;
}

std::pair<std::string, std::string> 
Dx12Wrapper::SplitFileName(const std::string & path, const char splitter)
{
	// �ǂ��ŕ������邩�̎擾
	int idx = path.find(splitter);

	// �Ԃ�l�p�ϐ�
	std::pair<std::string, std::string>ret;	
	ret.first = path.substr(0,idx);// �O������	
	ret.second = path.substr(idx + 1, path.length() - idx - 1);// �������

	return ret;
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
	if (FAILED(LoadPMD())) 
		return 5;

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

	// �}�e���A���o�b�t�@�̍쐬
	if (FAILED(CreateMaterialBuffer())) 
		return 12;
	
	return 0;
}

// ���t���[���Ăяo���X�V����
void Dx12Wrapper::UpDate()
{
	// ��]������
	angle = 0.01f;
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
	_viewport.Width = Application::Instance().GetWindowSize().width;
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
	//clearColor[0] = (bbIndex != 0 ? 0 : 1);

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

	// �ŃX�N���v�^�[�n���h�����ǂ̂��炢���Z���邩
	int incsize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	// �n���h���̎擾
	auto mathandle = _matDescHeap->GetGPUDescriptorHandleForHeapStart();

	// �f�X�N���v�^�q�[�v�̃Z�b�g
	_cmdList->SetDescriptorHeaps(1, &_matDescHeap);

	// �`�惋�[�v
	for (auto& m : _materials) {
		// �f�X�N���v�^�e�[�u���̃Z�b�g
		_cmdList->SetGraphicsRootDescriptorTable(1, mathandle);

		// �`�敔
		_cmdList->DrawIndexedInstanced(m.indexNum, 1, offset, 0, 0);

		// �|�C���^�̉��Z
		mathandle.ptr += incsize*4;// 4�����邩��4�{
		
		// �ϐ��̉��Z
		offset += m.indexNum;
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

	_swapchain->Present(1, 0);
}
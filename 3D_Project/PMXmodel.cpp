#pragma once
#include "PMXmodel.h"
#include <d3d12.h>
#include <iostream>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <DirectXTex.h>

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"DirectXTex.lib")


void PMXmodel::LoadModel(ID3D12Device* _dev, const std::string modelPath)
{
	FILE*fp;
	std::string ModelPath = modelPath;
	FolderPath = ModelPath.substr(0, ModelPath.rfind('/')+1);
	fopen_s(&fp, ModelPath.c_str(), "rb");

	// �w�b�_�[���̓ǂݍ���
	fread(&header, sizeof(PMXHeader), 1, fp);

	int idx[4];
	for (int i = 0; i < 4; ++i)
	{
		fread(&idx[i], sizeof(int), 1, fp);
		fseek(fp, idx[i], SEEK_CUR);
	}
	
	int vertexNum=0;
	fread(&vertexNum, sizeof(vertexNum), 1, fp);
	vertexInfo.resize(vertexNum);

	for (auto& vi : vertexInfo)
	{
		fread(&vi, sizeof(vi.pos)+ sizeof(vi.normal)+ sizeof(vi.uv), 1, fp);
		
		// �ǉ�UV�ǂݍ���
		for (int i = 0; i < header.data[1]; ++i)
		{
			fread(&vi.adduv[i], sizeof(vi.adduv), 1, fp);
		}

		// weight�ό`�`������݂���
		fread(&vi.weight, sizeof(vi.weight), 1, fp);

		// �ό`�`���ʂ̓ǂݍ���
		if (vi.weight == 0)
		{
			fread(&vi.boneIdx[0],header.data[5], 1, fp);
		}
		else if (vi.weight == 1)
		{
			fread(&vi.boneIdx[0], header.data[5], 1, fp);
			fread(&vi.boneIdx[1], header.data[5], 1, fp);
			fread(&vi.boneweight[0], sizeof(float), 1, fp);
		}
		else if (vi.weight == 2)
		{
			fread(&vi.boneIdx[0], header.data[5], 1, fp);
			fread(&vi.boneIdx[1], header.data[5], 1, fp);
			fread(&vi.boneIdx[2], header.data[5], 1, fp);
			fread(&vi.boneIdx[3], header.data[5], 1, fp);
			fread(&vi.boneweight[0], sizeof(float), 1, fp);
			fread(&vi.boneweight[1], sizeof(float), 1, fp);
			fread(&vi.boneweight[2], sizeof(float), 1, fp);
			fread(&vi.boneweight[3], sizeof(float), 1, fp);
		}
		else if (vi.weight == 3)
		{
			fread(&vi.boneIdx[0], header.data[5], 1, fp);
			fread(&vi.boneIdx[1], header.data[5], 1, fp);
			fread(&vi.boneweight[0], sizeof(float), 1, fp);
			fread(&vi.SDEFdata[0], sizeof(vi.SDEFdata[0]), 1, fp);
			fread(&vi.SDEFdata[1], sizeof(vi.SDEFdata[1]), 1, fp);
			fread(&vi.SDEFdata[2], sizeof(vi.SDEFdata[2]), 1, fp);
		}

		// �G�b�W�̓ǂݍ���
		fread(&vi.edge, sizeof(float), 1, fp);
	}

	// �C���f�b�N�X�ǂݍ���
	int indexNum = 0;
	fread(&indexNum, sizeof(indexNum), 1, fp);
	_verindex.resize(indexNum);
	for (int i= 0;i< indexNum;++i)
	{
		fread(&_verindex[i], header.data[2], 1, fp);
	}

	// �e�N�X�`���ǂݍ���
	int texNum = 0;
	fread(&texNum, sizeof(texNum), 1, fp);
	_texVec.resize(texNum);
	for (int i = 0; i < texNum; ++i)
	{
		std::string str;
		int pathnum = 0;
		fread(&pathnum, sizeof(int), 1, fp);
		for (int num = 0;num < pathnum/2;++num)
		{
			wchar_t c;
			fread(&c, sizeof(wchar_t), 1, fp);
			str.push_back(c);
		}
		_texVec[i] = str;
	}

	// �}�e���A���ǂݍ���
	int matNum = 0;
	fread(&matNum, sizeof(matNum), 1, fp);
	_materials.resize(matNum);

	for (int i = 0; i < matNum; ++i)
	{
		int num;
		fread(&num, sizeof(num), 1, fp);
		fseek(fp, num, SEEK_CUR);
		
		fread(&num, sizeof(num), 1, fp);
		fseek(fp, num, SEEK_CUR);


		fread(&_materials[i].Diffuse, sizeof(_materials[i].Diffuse), 1, fp);// �f�B�t���[�Y�̓ǂݍ���

		fread(&_materials[i].Specular, sizeof(_materials[i].Specular), 1, fp);
		fread(&_materials[i].SpecularPow, sizeof(_materials[i].SpecularPow), 1, fp);
		fread(&_materials[i].Ambient, sizeof(_materials[i].Ambient), 1, fp);
		fread(&_materials[i].bitFlag, sizeof(_materials[i].bitFlag), 1, fp);
		fread(&_materials[i].edgeColor, sizeof(_materials[i].edgeColor), 1, fp);
		fread(&_materials[i].edgeSize, sizeof(_materials[i].edgeSize), 1, fp);
		fread(&_materials[i].texIndex, header.data[3], 1, fp);
		fread(&_materials[i].sphereIndex, header.data[3], 1, fp);
		fread(&_materials[i].sphereMode, sizeof(_materials[i].sphereMode), 1, fp);
		fread(&_materials[i].toonFlag, sizeof(_materials[i].toonFlag), 1, fp);
		fread(&_materials[i].toonIndex, _materials[i].toonFlag?sizeof(unsigned char):header.data[3], 1, fp);
		
		fread(&num, sizeof(num), 1, fp);
		fseek(fp, num, SEEK_CUR);

		fread(&_materials[i].faceVerCnt, sizeof(_materials[i].faceVerCnt), 1, fp);
	}

	// �{�[���ǂݍ���
	int boneNum = 0;
	fread(&boneNum, sizeof(boneNum), 1, fp);
	_bones.resize(boneNum);

	std::wstring jbonename;
	std::wstring ebonename;
	for (int idx = 0;idx< boneNum;++idx)
	{
		int jnamenum;
		fread(&jnamenum, sizeof(jnamenum), 1, fp);
		jbonename.resize(jnamenum /2);
		for (auto& bn: jbonename)
		{
			fread(&bn, sizeof(bn), 1, fp);
		}
		_bones[idx].name = jbonename;

		int enamenum;
		fread(&enamenum, sizeof(enamenum), 1, fp);
		ebonename.resize(enamenum / 2);
		for (auto& bn : ebonename)
		{
			fread(&bn, sizeof(bn), 1, fp);
		}
	
		fread(&_bones[idx].pos, sizeof(_bones[idx].pos), 1, fp);
		fread(&_bones[idx].parentboneIndex, header.data[5], 1, fp);
		fread(&_bones[idx].tranceLevel, sizeof(_bones[idx].tranceLevel), 1, fp);
		fread(&_bones[idx].bitFlag, sizeof(_bones[idx].bitFlag), 1, fp);

		// �ڑ���
		if (_bones[idx].bitFlag & 0x0001)
		{
			fread(&_bones[idx].boneIndex, header.data[5], 1, fp);
		}
		else 
		{
			fread(&_bones[idx].offset, sizeof(_bones[idx].offset), 1, fp);
		}

		// ��]�t�^ �� �ړ��t�^
		if ((_bones[idx].bitFlag & 0x0100) || (_bones[idx].bitFlag & 0x0200))
		{
			fread(&_bones[idx].grantIndex, header.data[5], 1, fp);
			fread(&_bones[idx].grantPar, sizeof(_bones[idx].grantPar), 1, fp);
		}

		// ���Œ�
		if (_bones[idx].bitFlag & 0x0400)
		{
			fread(&_bones[idx].axisvector, sizeof(_bones[idx].axisvector), 1, fp);
		}

		// ���[�J����
		if (_bones[idx].bitFlag & 0x0800)
		{
			fread(&_bones[idx].axisXvector, sizeof(_bones[idx].axisXvector), 1, fp);
			fread(&_bones[idx].axisZvector, sizeof(_bones[idx].axisZvector), 1, fp);
		}

		// �O���e�ό`
		if (_bones[idx].bitFlag & 0x2000)
		{
			fread(&_bones[idx].key, sizeof(_bones[idx].key), 1, fp);
		}

		// IK�f�[�^
		if (_bones[idx].bitFlag & 0x0020)
		{
			fread(&_bones[idx].IkData.boneIdx, header.data[5], 1, fp);
			fread(&_bones[idx].IkData.loopCnt, sizeof(_bones[idx].IkData.loopCnt), 1, fp);
			fread(&_bones[idx].IkData.limrad, sizeof(_bones[idx].IkData.limrad), 1, fp);

			// IK�����N
			fread(&_bones[idx].IkData.linkNum, sizeof(_bones[idx].IkData.linkNum), 1, fp);
			for (int num = 0;num< _bones[idx].IkData.linkNum;++num)
			{
				fread(&_bones[idx].IkData.linkboneIdx, header.data[5], 1, fp);
				fread(&_bones[idx].IkData.isRadlim, sizeof(_bones[idx].IkData.isRadlim), 1, fp);
				if (_bones[idx].IkData.isRadlim)
				{
					fread(&_bones[idx].IkData.minRadlim, sizeof(_bones[idx].IkData.minRadlim), 1, fp);
					fread(&_bones[idx].IkData.maxRadlim, sizeof(_bones[idx].IkData.maxRadlim), 1, fp);
				}
			}
		}
	}

	// ���[��(�\��)�̓ǂݍ���
	int morphNum=0;
	fread(&morphNum, sizeof(morphNum), 1, fp);
	std::vector<MorphHeader> _morphs;
	std::vector<MorphOffsets> _morphOffsets;
	_morphs.resize(morphNum);

	for (int i = 0; i< morphNum; ++i)
	{
		int name;
		fread(&name, sizeof(name), 1, fp);
		std::wstring jname;
		jname.resize(name / 2);
		for (auto& mn : jname)
		{
			fread(&mn, sizeof(mn), 1, fp);
		}

		std::wstring ename;
		fread(&name, sizeof(name), 1, fp);
		ename.resize(name/2);
		for (auto& mn : ename)
		{
			fread(&mn, sizeof(mn), 1, fp);
		}

		fread(&_morphs[i], sizeof(_morphs[i]), 1, fp);
		_morphOffsets.resize(_morphs[i].dataNum);

		for (int num = 0; num < _morphs[i].dataNum; ++num)
		{
			if (_morphs[i].type == 0)
			{
				fread(&_morphOffsets[num].groupMorph.MorphIdx, header.data[6], 1, fp);
				fread(&_morphOffsets[num].groupMorph.MorphPar, sizeof(_morphOffsets[num].groupMorph.MorphPar), 1, fp);
			}
			else if (_morphs[i].type == 1)
			{
				fread(&_morphOffsets[num].vertexMorph.verIdx, header.data[2], 1, fp);
				fread(&_morphOffsets[num].vertexMorph.pos, sizeof(_morphOffsets[num].vertexMorph.pos), 1, fp);
			}
			else if (_morphs[i].type == 2)
			{
				fread(&_morphOffsets[num].boneMorph.boneIdx, header.data[5], 1, fp);
				fread(&_morphOffsets[num].boneMorph.moveVal, sizeof(_morphOffsets[num].boneMorph.moveVal), 1, fp);
				fread(&_morphOffsets[num].boneMorph.rotation, sizeof(_morphOffsets[num].boneMorph.rotation), 1, fp);

			}
			else if (_morphs[i].type == 8)
			{
				fread(&_morphOffsets[num].materialMorph.materialIdx, header.data[4], 1, fp);
				fread(&_morphOffsets[num].materialMorph.type, sizeof(_morphOffsets[num].materialMorph.type), 1, fp);
				fread(&_morphOffsets[num].materialMorph.diffuse, sizeof(_morphOffsets[num].materialMorph.diffuse), 1, fp);
				fread(&_morphOffsets[num].materialMorph.specular, sizeof(_morphOffsets[num].materialMorph.specular), 1, fp);
				fread(&_morphOffsets[num].materialMorph.specularPow, sizeof(_morphOffsets[num].materialMorph.specularPow), 1, fp);
				fread(&_morphOffsets[num].materialMorph.ambient, sizeof(_morphOffsets[num].materialMorph.ambient), 1, fp);
				fread(&_morphOffsets[num].materialMorph.edgeColor, sizeof(_morphOffsets[num].materialMorph.edgeColor), 1, fp);
				fread(&_morphOffsets[num].materialMorph.edgeSize, sizeof(_morphOffsets[num].materialMorph.edgeSize), 1, fp);
				fread(&_morphOffsets[num].materialMorph.texPow, sizeof(_morphOffsets[num].materialMorph.texPow), 1, fp);
				fread(&_morphOffsets[num].materialMorph.sphTexPow, sizeof(_morphOffsets[num].materialMorph.sphTexPow), 1, fp);
				fread(&_morphOffsets[num].materialMorph.toonTexPow, sizeof(_morphOffsets[num].materialMorph.toonTexPow), 1, fp);
			}
			else
			{
				fread(&_morphOffsets[num].uvMorph.verIdx, header.data[2], 1, fp);
				fread(&_morphOffsets[num].uvMorph.uvOffset, sizeof(_morphOffsets[num].uvMorph.uvOffset), 1, fp);
			}
		}
		_morphHeaders[jname] = _morphs[i];
		_morphData[jname] = _morphOffsets;
	}	

	fclose(fp);

	// �e�o�b�t�@�̏�����
	_materialsBuff.resize(matNum);
	_toonResources.resize(matNum);
	_textureBuffer.resize(matNum);
	_sphBuffer.resize(matNum);
	_spaBuffer.resize(matNum);

	for (int i=0;i< matNum;++i)
	{
		if (_materials[i].texIndex != 255)
		{
			auto str = FolderPath + _texVec[_materials[i].texIndex];
			_textureBuffer[i] = LoadTextureFromFile(str, _dev);
		}
	}

	auto result = CreateVertexBuffer(_dev);

	result = CreateIndexBuffer(_dev);

	// ����Ă����������
	result = CreateWhiteTexture(_dev);
	// ����Ă����������
	result = CreateBlackTexture(_dev);
	// �O���f�[�V�����e�N�X�`������
	result = CreateGrayGradationTexture(_dev);

	result = CreateMaterialBuffer(_dev);

	CreateBoneTree();

	result = CreateBoneBuffer(_dev);

}

void PMXmodel::BufferUpDate()
{
	// ���_�o�b�t�@�̍X�V
	PMXVertexInfo* vertMap = nullptr;
	auto result = _vertexBuffer->Map(0, nullptr, (void**)&vertMap);
	std::copy(vertexInfo.begin(), vertexInfo.end(), vertMap);
	_vertexBuffer->Unmap(0, nullptr);

	// �C���f�b�N�X�o�b�t�@�̃}�b�s���O
	unsigned short* idxMap = nullptr;
	result = _indexBuffer->Map(0, nullptr, (void**)&idxMap);
	std::copy(std::begin(_verindex), std::end(_verindex), idxMap);
	_indexBuffer->Unmap(0, nullptr);
}

PMXmodel::PMXmodel(ID3D12Device* dev, const std::string modelPath)
{
	LoadModel(dev,modelPath);
}


PMXmodel::~PMXmodel()
{
}

HRESULT PMXmodel::CreateWhiteTexture(ID3D12Device* _dev)
{
	// ���o�b�t�@�p�̃f�[�^�z��
	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0xff);//�S��255�Ŗ��߂�

	// �]������p�̃q�[�v�ݒ�
	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0
	);

	// �]������p�̃f�X�N�ݒ�
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM,
		4,
		4
	);

	// ���e�N�X�`���̍쐬
	auto result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,//���Ɏw��Ȃ�
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&whiteTex)
	);

	result = whiteTex->WriteToSubresource(0, nullptr, data.data(), 4 * 4, static_cast<unsigned int>(data.size()));

	return result;
}

HRESULT PMXmodel::CreateBlackTexture(ID3D12Device* _dev)
{
	// ���o�b�t�@�p�̃f�[�^�z��
	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0);//�S��0�Ŗ��߂�

	// �]������p�̃q�[�v�ݒ�
	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0
	);

	// �]������p�̃f�X�N�ݒ�
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM,
		4,
		4
	);

	// ���e�N�X�`���̍쐬
	auto result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,//���Ɏw��Ȃ�
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&blackTex)
	);

	result = blackTex->WriteToSubresource(0, nullptr, data.data(), 4 * 4, static_cast<unsigned int>(data.size()));

	return result;
}

HRESULT PMXmodel::CreateVertexBuffer(ID3D12Device * _dev)
{
	// �q�[�v�̏��ݒ�
	D3D12_HEAP_PROPERTIES heapprop = {};
	heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;//CPU����GPU�֓]������p
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	// create
	auto result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,//���ʂȎw��Ȃ�
		&CD3DX12_RESOURCE_DESC::Buffer(vertexInfo.size() * sizeof(PMXVertexInfo)),
		D3D12_RESOURCE_STATE_GENERIC_READ,//��݂���
		nullptr,//nullptr�ł���
		IID_PPV_ARGS(&_vertexBuffer));//������

	// ���_�o�b�t�@�̃}�b�s���O
	PMXVertexInfo* vertMap = nullptr;
	result = _vertexBuffer->Map(0, nullptr, (void**)&vertMap);
	std::copy(vertexInfo.begin(), vertexInfo.end(), vertMap);
	_vertexBuffer->Unmap(0, nullptr);

	_vbView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
	_vbView.StrideInBytes = sizeof(PMXVertexInfo);
	_vbView.SizeInBytes = static_cast<unsigned int>(vertexInfo.size()) * sizeof(PMXVertexInfo);

	return result;
}

HRESULT PMXmodel::CreateIndexBuffer(ID3D12Device * _dev)
{
	// �q�[�v�̏��ݒ�
	D3D12_HEAP_PROPERTIES heapprop = {};
	heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;//CPU����GPU�֓]������p
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	auto result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(_verindex.size() * sizeof(_verindex[0])),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_indexBuffer));

	// �C���f�b�N�X�o�b�t�@�̃}�b�s���O
	unsigned short* idxMap = nullptr;
	result = _indexBuffer->Map(0, nullptr, (void**)&idxMap);
	std::copy(std::begin(_verindex), std::end(_verindex), idxMap);

	_indexBuffer->Unmap(0, nullptr);

	_idxView.BufferLocation = _indexBuffer->GetGPUVirtualAddress();//�o�b�t�@�̏ꏊ
	_idxView.Format = DXGI_FORMAT_R16_UINT;//�t�H�[�}�b�g(short������R16)
	_idxView.SizeInBytes = static_cast<unsigned int>(_verindex.size()) * sizeof(_verindex[0]);//���T�C�Y

	return result;
}

HRESULT PMXmodel::CreateMaterialBuffer(ID3D12Device* _dev)
{
	D3D12_DESCRIPTOR_HEAP_DESC matDescHeap = {};
	matDescHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	matDescHeap.NodeMask = 0;

	// �萔�o�b�t�@�ƃV�F�[�_�[���\�[�X�r���[��Sph��Spa�ƃg�D�[���̂T����
	matDescHeap.NumDescriptors = static_cast<unsigned int>(_materials.size()) * 5;
	matDescHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	auto result = _dev->CreateDescriptorHeap(&matDescHeap, IID_PPV_ARGS(&_matDescHeap));

	size_t size = (sizeof(PMXColor) + 0xff)&~0xff;

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

		MapColor->diffuse = _materials[midx].Diffuse;

		MapColor->ambient = _materials[midx].Ambient;

		MapColor->specular.x = _materials[midx].Specular.x;
		MapColor->specular.y = _materials[midx].Specular.y;
		MapColor->specular.z = _materials[midx].Specular.z;
		MapColor->specular.w = _materials[midx].SpecularPow;

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

	for (int i = 0; i < _materialsBuff.size(); ++i)
	{
		//�g�D�[�����\�[�X�̓ǂݍ���
		std::string toonFilePath = "toon/";
		char toonFileName[16];
		sprintf_s(toonFileName, "toon%02d.bmp", _materials[i].toonIndex + 1);
		// �ŗL�g�D�[���̏ꍇ����
		if (!_materials[i].toonFlag)
		{
			if (_materials[i].toonIndex!=255)
			{
				toonFilePath = FolderPath;
				sprintf_s(toonFileName, _texVec[_materials[i].toonIndex].c_str());
			}
		}

		toonFilePath += toonFileName;
		_toonResources[i] = LoadTextureFromFile(toonFilePath, _dev);

		// �萔�o�b�t�@�̍쐬
		matDesc.BufferLocation = _materialsBuff[i]->GetGPUVirtualAddress();
		matDesc.SizeInBytes = static_cast<unsigned int>(size);
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
		}
		matHandle.ptr += addsize;

		// �g�D�[����SRV�̍쐬
		if (_toonResources[i] == nullptr) {
			srvDesc.Format = whiteTex->GetDesc().Format;
			_dev->CreateShaderResourceView(whiteTex, &srvDesc, matHandle);
		}
		else {
			srvDesc.Format = _toonResources[i]->GetDesc().Format;
			_dev->CreateShaderResourceView(_toonResources[i], &srvDesc, matHandle);
		}
		matHandle.ptr += addsize;

	}

	return result;
}

void PMXmodel::CreateBoneTree()
{
	_boneMats.resize(_bones.size());
	// �P�ʍs��ŏ�����
	std::fill(_boneMats.begin(), _boneMats.end(), XMMatrixIdentity());
	
	for (int idx = 0; idx < _bones.size(); ++idx) {
		auto& b = _bones[idx];
		auto& boneNode = _boneMap[b.name];
		boneNode.boneIdx = idx;
		if (b.boneIndex!=0xffff)
		{
			boneNode.startPos = b.pos;
			boneNode.endPos = _bones[b.boneIndex].pos;
		}
	}	for (auto& b : _boneMap) {
		if (_bones[b.second.boneIdx].parentboneIndex >= _bones.size())continue;
		auto parentName = _bones[_bones[b.second.boneIdx].parentboneIndex].name;
			_boneMap[parentName].children.push_back(&b.second);
	}

}

HRESULT PMXmodel::CreateBoneBuffer(ID3D12Device* _dev)
{
	// �T�C�Y�𒲐�
	size_t size = sizeof(DirectX::XMMATRIX)*_bones.size();
	size = (size + 0xff)&~0xff;

	// �{�[���o�b�t�@�̍쐬
	auto result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_boneBuffer));

	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = 1;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&_boneHeap));

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvdesc = {};
	cbvdesc.BufferLocation = _boneBuffer->GetGPUVirtualAddress();
	cbvdesc.SizeInBytes = static_cast<unsigned int>(size);

	auto handle = _boneHeap->GetCPUDescriptorHandleForHeapStart();
	_dev->CreateConstantBufferView(&cbvdesc, handle);

	result = _boneBuffer->Map(0, nullptr, (void**)&_mappedBones);

	for(auto& bm: _boneMats)
	{
		bm = XMMatrixRotationZ(XM_PIDIV4);
	}


	std::copy(std::begin(_boneMats), std::end(_boneMats), _mappedBones);

	return result;
}

void PMXmodel::RotationMatrix(std::wstring bonename, XMFLOAT3 theta)
{
	auto boneNode = _boneMap[bonename];
	auto start = XMLoadFloat3(&boneNode.startPos);// ���̍��W�����Ă���

	//���_�܂ŕ��s�ړ����Ă����ŉ�]���s���A���̈ʒu�܂Ŗ߂�
	_boneMats[boneNode.boneIdx] =
		XMMatrixTranslationFromVector(XMVectorScale(start, -1))*		XMMatrixRotationX(theta.x)*		XMMatrixRotationY(theta.y)*		XMMatrixRotationZ(theta.z)*		XMMatrixTranslationFromVector(start);
}

void PMXmodel::RecursiveMatrixMultiply(PMXBoneNode& node, XMMATRIX& MultiMat)
{
	// �s�����Z����
	_boneMats[node.boneIdx] *= MultiMat;

	// �ċA����
	for (auto& child : node.children) {
		RecursiveMatrixMultiply(*child, _boneMats[node.boneIdx]);
	}
}

void PMXmodel::UpDate(char key[256])
{
	//if (key[DIK_F])
	//{
	//	if (_materials[21].Diffuse.w < 1.f)
	//	{
	//		_materials[21].Diffuse.w += 0.01f;
	//	}
	//}
	//else
	//{
	//	if (_materials[21].Diffuse.w > 0.f)
	//	{
	//		_materials[21].Diffuse.w -= 0.01f;
	//	}
	//}

	//if (key[DIK_T])
	//{
	//	if (_materials[22].Diffuse.w < 1.f)
	//	{
	//		_materials[22].Diffuse.w += 0.01f;
	//	}
	//}
	//else
	//{
	//	if (_materials[22].Diffuse.w > 0.f)
	//	{
	//		_materials[22].Diffuse.w -= 0.01f;
	//	}
	//}

	if (key[DIK_H])
	{
		angle += 0.01f;
	}

	// �}�e���A���J���[�̓]��
	int midx = 0;
	for (auto& mbuff : _materialsBuff) {

		auto result = mbuff->Map(0, nullptr, (void**)&MapColor);
		MapColor->diffuse = _materials[midx].Diffuse;

		MapColor->ambient = _materials[midx].Ambient;

		MapColor->specular.x = _materials[midx].Specular.x;
		MapColor->specular.y = _materials[midx].Specular.y;
		MapColor->specular.z = _materials[midx].Specular.z;
		MapColor->specular.w = _materials[midx].SpecularPow;

		mbuff->Unmap(0, nullptr);

		++midx;
	}
	

	// �{�[��
	std::fill(_boneMats.begin(), _boneMats.end(), XMMatrixIdentity());

	RotationMatrix(L"�E��", DirectX::XMFLOAT3(angle, 0, 0));	RotationMatrix(L"����", DirectX::XMFLOAT3(angle, 0, 0));

	DirectX::XMMATRIX rootmat = DirectX::XMMatrixIdentity();
	RecursiveMatrixMultiply(_boneMap[L"�Z���^�["], rootmat);

	std::copy(_boneMats.begin(), _boneMats.end(), _mappedBones);

	// �o�b�t�@�̍X�V
	BufferUpDate();
}

const std::vector<D3D12_INPUT_ELEMENT_DESC> PMXmodel::GetInputLayout()
{
	// ���_���C�A�E�g (�\���̂Ə��Ԃ����킹�邱��)
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayoutDescs = {
		// ���W
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0, D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		// �@��
		{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		// UV
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		// �ǉ�UV
		{"ADDUV",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"ADDUV",1,DXGI_FORMAT_R32G32B32A32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"ADDUV",2,DXGI_FORMAT_R32G32B32A32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"ADDUV",3,DXGI_FORMAT_R32G32B32A32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		// wighttype
		{"WEIGHTTYPE",0,DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT ,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		// �{�[���C���f�b�N�X
		{"BONENO",0,DXGI_FORMAT_R8G8B8A8_SINT,0,D3D12_APPEND_ALIGNED_ELEMENT,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		// weight
		{"WEIGHT",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};
	return inputLayoutDescs;
}

const LPCWSTR PMXmodel::GetUseShader()
{
	return L"Shader.hlsl";
}

ID3D12Resource * PMXmodel::LoadTextureFromFile(std::string & texPath, ID3D12Device* _dev)
{
	//WIC�e�N�X�`���̃��[�h
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};

	// �e�N�X�`���t�@�C���̃��[�h
	auto result = LoadFromWICFile(GetWstringFromString(texPath).c_str(),
		WIC_FLAGS_NONE,
		&metadata,
		scratchImg);
	if (FAILED(result)) {
		return whiteTex;// ���s�����甒�e�N�X�`��������
	}

	// �C���[�W�f�[�^�����
	auto img = scratchImg.GetImage(0, 0, 0);

	// �]������p�̃q�[�v�ݒ�(pdf:p250) 
	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0
	);

	// ���\�[�X�f�X�N�̍Đݒ�
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		metadata.format,
		static_cast<unsigned int>(metadata.width),
		static_cast<unsigned int>(metadata.height),
		static_cast<UINT16>(metadata.arraySize),
		static_cast<UINT16>(metadata.mipLevels)
	);

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
		static_cast<unsigned int>(img->rowPitch),
		static_cast<unsigned int>(img->slicePitch)
	);

	return texbuff;
}

std::wstring PMXmodel::GetWstringFromString(const std::string& str)
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

HRESULT PMXmodel::CreateGrayGradationTexture(ID3D12Device* _dev)
{
	// �]������p�̃q�[�v�ݒ�
	D3D12_HEAP_PROPERTIES texHeapProp = {};
	texHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	texHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	texHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	texHeapProp.CreationNodeMask = 0;
	texHeapProp.VisibleNodeMask = 0;

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resDesc.Width = 4;
	resDesc.Height = 256;
	resDesc.DepthOrArraySize = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.MipLevels = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	// �オ�����Ă����������e�N�X�`���f�[�^�̍쐬
	auto result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,//���Ɏw��Ȃ�
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&gradTex)
	);


	std::vector<unsigned int> data(4 * 256);
	auto it = data.begin();
	unsigned int c = 0xff;
	for (; it != data.end(); it += 4) {
		auto col = (c << 0xff) | (c << 16) | (c << 8) | c;
		std::fill(it, it + 4, col);
		--c;
	}

	result = gradTex->WriteToSubresource(0,
		nullptr,
		data.data(),
		4 * sizeof(unsigned int),
		sizeof(unsigned int)*data.size());

	return result;
}

void PMXmodel::CreateBoneOrder(PMXBoneNode& node,int level)
{
	if (_bones[node.boneIdx].tranceLevel == level)
	{
		_orderMoveIdx.emplace_back(_bones[node.boneIdx].boneIndex);
	}
}

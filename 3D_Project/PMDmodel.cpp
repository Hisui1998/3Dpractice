#include "PMDmodel.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <DirectXTex.h>

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"DirectXTex.lib")


PMDmodel::PMDmodel(ID3D12Device* _dev, const std::string modelPath):angle(0.0f)
{
	// PMD�t�@�C���̓ǂݍ���
	LoadModel(_dev,modelPath);
}

PMDmodel::~PMDmodel()
{
}

void PMDmodel::UpDate()
{
	angle+=10*DirectX::XM_PI/180;
	// ����
	std::fill(_boneMats.begin(), _boneMats.end(), DirectX::XMMatrixIdentity());
	RotationMatrix("�E��", DirectX::XMFLOAT3(angle, 0, 0));	RotationMatrix("�E�Ђ�", DirectX::XMFLOAT3(angle, 0, 0));	RotationMatrix("����", DirectX::XMFLOAT3(angle, 0, 0));	RotationMatrix("���Ђ�", DirectX::XMFLOAT3(angle, 0, 0));

	DirectX::XMMATRIX rootmat = DirectX::XMMatrixIdentity();
	RecursiveMatrixMultiply(_boneMap["�Z���^�["], rootmat);

	std::copy(_boneMats.begin(), _boneMats.end(), _mappedBones);
}



void PMDmodel::RotationMatrix(std::string bonename, DirectX::XMFLOAT3 theta)
{
	auto boneNode = _boneMap[bonename];	
	auto vec = DirectX::XMLoadFloat3(&boneNode.startPos);// ���̍��W�����Ă���

	// ���_�܂ŕ��s�ړ����Ă����ŉ�]���s���A���̈ʒu�܂Ŗ߂�
	_boneMats[boneNode.boneIdx] =
		DirectX::XMMatrixTranslationFromVector(DirectX::XMVectorScale(vec, -1))*/* ���_�Ɉړ� */		DirectX::XMMatrixRotationX(theta.x)*									/* �{�[���s��̉�](X��) */		DirectX::XMMatrixRotationY(theta.y)*									/* �{�[���s��̉�](Y��) */		DirectX::XMMatrixRotationZ(theta.z)*									/* �{�[���s��̉�](Z��) */		DirectX::XMMatrixTranslationFromVector(vec);							/* ���̍��W�ɖ߂� */
}

void PMDmodel::RecursiveMatrixMultiply(BoneNode& node, DirectX::XMMATRIX& MultiMat)
{
	// �s�����Z����
	_boneMats[node.boneIdx] *= MultiMat;

	// �ċA����
	for (auto& child : node.children) {
		RecursiveMatrixMultiply(*child, _boneMats[node.boneIdx]);
	}
}

std::vector<PMDMaterial> PMDmodel::GetMaterials()
{
	return _materials;
}

std::vector<unsigned short> PMDmodel::GetVertexIndex()
{
	return _verindex;
}

std::vector<VertexInfo> PMDmodel::GetVertexInfo()
{
	return _vivec;
}

ID3D12DescriptorHeap *& PMDmodel::GetBoneHeap()
{
	return _boneHeap;
}

ID3D12DescriptorHeap *& PMDmodel::GetMaterialHeap()
{
	return _matDescHeap;
}

void PMDmodel::LoadModel(ID3D12Device* _dev,const std::string modelPath)
{
	FILE*fp;
	PMDHeader data;
	std::string ModelPath = modelPath;

	fopen_s(&fp, ModelPath.c_str(), "rb");

	// �w�b�_�[���̓ǂݍ���
	fread(&data.signature, sizeof(data.signature), 1, fp);
	fread(&data.version, sizeof(PMDHeader) - sizeof(data.signature) - 1, 1, fp);

	// ���_���̓ǂݍ���
	unsigned int vnum = 0;
	fread(&vnum, sizeof(vnum), 1, fp);

	_vivec.resize(vnum);

	for (auto& vi : _vivec)
	{
		fread(&vi, sizeof(VertexInfo), 1, fp);
	}

	// �C���f�b�N�X���̓ǂݍ���
	unsigned int inum = 0;
	fread(&inum, sizeof(unsigned int), 1, fp);
	_verindex.resize(inum);

	for (auto& vidx : _verindex)
	{
		fread(&vidx, sizeof(unsigned short), 1, fp);
	}

	// �}�e���A���̓ǂݍ���
	unsigned int mnum = 0;
	fread(&mnum, sizeof(unsigned int), 1, fp);
	_materials.resize(mnum);

	for (auto& mat : _materials)
	{
		fread(&mat, sizeof(PMDMaterial), 1, fp);
	}

	// ����Ă����������
	auto result = CreateWhiteTexture(_dev);
	// ����Ă����������
	result = CreateBlackTexture(_dev);
	// �O���f�[�V�����Ă����������
	result = CreateGrayGradationTexture(_dev);

	// �e�N�X�`���̓ǂݍ��݂ƃo�b�t�@�̍쐬
	_textureBuffer.resize(mnum);
	_sphBuffer.resize(mnum);
	_spaBuffer.resize(mnum);
	_toonResources.resize(mnum);

	for (int i = 0; i < _materials.size(); ++i) {
		// �e�N�X�`���t�@�C���p�X�̎擾
		std::string texFileName = _materials[i].texFileName;

		if (std::count(texFileName.begin(), texFileName.end(), '*') > 0)
		{
			auto namepair = SplitFileName(texFileName);
			if (GetExtension(namepair.first) == "sph" ||
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
			_sphBuffer[i] = LoadTextureFromFile(texFilePath, _dev);
		}
		else if (GetExtension(texFileName) == "spa")
		{
			_spaBuffer[i] = LoadTextureFromFile(texFilePath, _dev);
		}
		else
		{
			_textureBuffer[i] = LoadTextureFromFile(texFilePath, _dev);
		}
	}

	// �{�[���̓ǂݍ���
	unsigned int bnum = 0;
	fread(&bnum, sizeof(unsigned short), 1, fp);
	_bones.resize(bnum);
	for (auto& bone : _bones)
	{
		fread(&bone, sizeof(PMDBoneInfo), 1, fp);
	}
	fclose(fp);

	// �}�e���A���o�b�t�@�̍쐬
	CreateMaterialBuffer(_dev);
	// �{�[���c���[�쐬
	CreateBoneTree();
	// �{�[���o�b�t�@�쐬
	CreateBoneBuffer(_dev);
}

std::string PMDmodel::GetExtension(const std::string & path)
{
	// �g���q�̎�O�̂ǂ��Ƃ܂ł̕�������擾����
	int idx = path.rfind('.');

	// ��������؂���
	auto p = path.substr(idx + 1, path.length() - idx - 1);
	return p;
}

std::pair<std::string, std::string>
PMDmodel::SplitFileName(const std::string & path, const char splitter)
{
	// �ǂ��ŕ������邩�̎擾
	int idx = path.find(splitter);

	// �Ԃ�l�p�ϐ�
	std::pair<std::string, std::string>ret;
	ret.first = path.substr(0, idx);// �O������	
	ret.second = path.substr(idx + 1, path.length() - idx - 1);// �������

	return ret;
}

std::string PMDmodel::GetTexPath(const std::string & modelPath, const char * texPath)
{
	// �t�H���_�Z�p���[�^���u/�v����Ȃ��āu\\�v�̉\��������̂�2�p�^�[���擾����
	int pathIndex1 = modelPath.rfind('/');
	int pathIndex2 = modelPath.rfind('\\');

	// rfind�֐��͌�����Ȃ������Ƃ���0xffffffff��Ԃ�����2�̃p�X���r����
	int path = max(pathIndex1, pathIndex2);

	// ���f���f�[�^�̓����Ă���t�H���_���t�T���ŒT���Ă���
	std::string folderPath = modelPath.substr(0, path + 1);

	// ����
	return std::string(folderPath + texPath);
}

ID3D12Resource * PMDmodel::LoadTextureFromFile(std::string & texPath, ID3D12Device* _dev)
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

	// �]������p�̃q�[�v�ݒ�(pdf:p250) 
	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0
	);

	// ���\�[�X�f�X�N�̍Đݒ�
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		metadata.format,
		metadata.width,
		metadata.height,
		metadata.arraySize,
		metadata.mipLevels
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
		img->rowPitch,
		img->slicePitch
	);
	_resourceTable[texPath] = texbuff;
	return texbuff;
}

std::wstring PMDmodel::GetWstringFromString(const std::string& str)
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

HRESULT PMDmodel::CreateWhiteTexture(ID3D12Device* _dev)
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

	result = whiteTex->WriteToSubresource(0, nullptr, data.data(), 4 * 4, data.size());

	return result;
}

HRESULT PMDmodel::CreateBlackTexture(ID3D12Device* _dev)
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

	result = blackTex->WriteToSubresource(0, nullptr, data.data(), 4 * 4, data.size());

	return result;
}

HRESULT PMDmodel::CreateGrayGradationTexture(ID3D12Device* _dev)
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

void PMDmodel::CreateBoneTree()
{
	_boneMats.resize(_bones.size());
	// �P�ʍs��ŏ�����
	std::fill(_boneMats.begin(), _boneMats.end(), DirectX::XMMatrixIdentity());

	for (int i = 0; i < _bones.size(); ++i)
	{
		auto& b = _bones[i];
		auto& bonenede = _boneMap[b.bone_name];
		bonenede.boneIdx = i;
		bonenede.startPos = b.bone_head_pos;
		bonenede.endPos = _bones[b.tail_pos_bone_index].bone_head_pos;
	}

	for (auto& b : _boneMap) {
		if (_bones[b.second.boneIdx].parent_bone_index >= _bones.size())continue;
		auto parentName = _bones[_bones[b.second.boneIdx].parent_bone_index].bone_name;
		_boneMap[parentName].children.push_back(&b.second);
	}
}

HRESULT PMDmodel::CreateBoneBuffer(ID3D12Device* _dev)
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
	cbvdesc.SizeInBytes = size;

	auto handle = _boneHeap->GetCPUDescriptorHandleForHeapStart();
	_dev->CreateConstantBufferView(&cbvdesc, handle);

	result = _boneBuffer->Map(0, nullptr, (void**)&_mappedBones);
	std::copy(std::begin(_boneMats), std::end(_boneMats), _mappedBones);

	return result;
}

HRESULT PMDmodel::CreateMaterialBuffer(ID3D12Device* _dev)
{
	D3D12_DESCRIPTOR_HEAP_DESC matDescHeap = {};
	matDescHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	matDescHeap.NodeMask = 0;

	// �萔�o�b�t�@�ƃV�F�[�_�[���\�[�X�r���[��Sph��Spa�ƃg�D�[���̂T����
	matDescHeap.NumDescriptors = _materials.size() * 5;
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


	for (int i = 0; i < _materialsBuff.size(); ++i)
	{
		//�g�D�[�����\�[�X�̓ǂݍ���
		std::string toonFilePath = "toon/";
		char toonFileName[16];
		sprintf_s(toonFileName, "toon%02d.bmp", _materials[i].toon_index + 1);
		toonFilePath += toonFileName;
		_toonResources[i] = LoadTextureFromFile(toonFilePath,_dev);

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
		}
		matHandle.ptr += addsize;

		// �g�D�[����SRV�̍쐬
		if (_toonResources[i] == nullptr) {
			srvDesc.Format = gradTex->GetDesc().Format;
			_dev->CreateShaderResourceView(gradTex, &srvDesc, matHandle);
		}
		else {
			srvDesc.Format = _toonResources[i]->GetDesc().Format;
			_dev->CreateShaderResourceView(_toonResources[i], &srvDesc, matHandle);
		}
		matHandle.ptr += addsize;

	}

	return result;
}
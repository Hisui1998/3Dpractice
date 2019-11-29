#pragma once
#include "PMXmodel.h"
#include <d3d12.h>
#include <iostream>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <DirectXTex.h>
#include "VMDMotion.h"
#include "Application.h"

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"DirectXTex.lib")

std::map<std::string, ID3D12Resource*> PMXmodel::_texMap;

void PMXmodel::LoadModel(const std::string modelPath, const std::string vmdPath)
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
		fread(&vi.weightType, sizeof(vi.weightType), 1, fp);

		// �ό`�`���ʂ̓ǂݍ���
		if (vi.weightType == 0)
		{
			fread(&vi.boneIdx[0],header.data[5], 1, fp);
		}
		else if (vi.weightType == 1)
		{
			fread(&vi.boneIdx[0], header.data[5], 1, fp);
			fread(&vi.boneIdx[1], header.data[5], 1, fp);
			fread(&vi.boneweight[0], sizeof(float), 1, fp);
		}
		else if (vi.weightType == 2)
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
		else if (vi.weightType == 3)
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
	for (auto& vi: _verindex)
	{
		fread(&vi, header.data[2], 1, fp);
	}

	// �e�N�X�`���ǂݍ���
	int texNum = 0;
	fread(&texNum, sizeof(texNum), 1, fp);
	_texturePaths.resize(texNum);
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
		_texturePaths[i] = str;
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

	if (vmdPath != "")
	{
		// Vmd�ǂݍ���
		_vmdData = std::make_shared<VMDMotion>(vmdPath,60);
		_morphWeight = 0;
	}
	else
	{
		_vmdData = nullptr;
	}


	// �e�o�b�t�@�̏�����
	_materialsBuff.resize(matNum);
	_toonResources.resize(matNum);
	_textureBuffer.resize(matNum);
	_sphBuffer.resize(matNum);
	_spaBuffer.resize(matNum);

	for (int i=0;i< matNum;++i)
	{
		if (_materials[i].texIndex < _texturePaths.size())
		{
			auto str = FolderPath + _texturePaths[_materials[i].texIndex];
			_textureBuffer[i] = LoadTextureFromFile(str);
		}
	}
	firstVertexInfo = vertexInfo;

	auto result = CreateRootSignature();

	result = CreatePipeline();

	result = CreateVertexBuffer();

	result = CreateIndexBuffer();

	result = CreateShadowRS();

	result = CreateShadowPS();

	// ����Ă����������
	result = CreateWhiteTexture();

	// ����Ă����������
	result = CreateBlackTexture();

	// �O���f�[�V�����e�N�X�`������
	result = CreateGrayGradationTexture();

	result = CreateMaterialBuffer();

	CreateBoneTree();

	result = CreateBoneBuffer();
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

void PMXmodel::MotionUpDate(int frameno)
{
	// �{�[��
	std::fill(_boneMats.begin(), _boneMats.end(), XMMatrixIdentity());
	for (auto& boneanim : _vmdData->GetMotionData()) {
		auto& boneName = boneanim.first;
		auto& keyframes = boneanim.second;

		// ���g�����[�V�����f�[�^���擾
		auto frameIt = std::find_if(keyframes.rbegin(), keyframes.rend(),
			[frameno](const MotionData& k) {return k.FrameNo <= frameno; });
		if (frameIt == keyframes.rend())continue;

		// �C�e���[�^�𔽓]�����Ď��̗v�f���Ƃ��Ă���
		auto nextIt = frameIt.base();

		if (nextIt == keyframes.end()) {
			// ��]
			RotationMatrix(GetWstringFromString(boneName), frameIt->Quaternion);
			
			// ���W�ړ�
			_boneMats[_boneTree[GetWstringFromString(boneName)].boneIdx]
				*= DirectX::XMMatrixTranslationFromVector(XMLoadFloat3(&frameIt->Location));
			
			_bones[_boneTree[GetWstringFromString(boneName)].boneIdx].pos = frameIt->Location;
		}
		else
		{
			// ���݂��ڰшʒu����d�݌v�Z
			float pow = (static_cast<float>(frameno) - frameIt->FrameNo) / (nextIt->FrameNo - frameIt->FrameNo);
			pow = GetBezierPower(pow, frameIt->p1, frameIt->p2, 12);

			// ��]
			RotationMatrix(GetWstringFromString(boneName), frameIt->Quaternion, nextIt->Quaternion, pow);

			// ���W�ړ�
			_boneMats[_boneTree[GetWstringFromString(boneName)].boneIdx]
				*= DirectX::XMMatrixTranslationFromVector(XMVectorLerp(XMLoadFloat3(&frameIt->Location),XMLoadFloat3(&nextIt->Location), pow));
			
			 XMStoreFloat3(&_bones[_boneTree[GetWstringFromString(boneName)].boneIdx].pos,XMVectorLerp(XMLoadFloat3(&frameIt->Location), XMLoadFloat3(&nextIt->Location), pow));
		}
	}

	// �c���[�̃g���o�[�X
	DirectX::XMMATRIX rootmat = DirectX::XMMatrixIdentity();
	RecursiveMatrixMultiply(_boneTree[L"�Z���^�["], rootmat);

	std::copy(_boneMats.begin(), _boneMats.end(), _sendBone);
}

void PMXmodel::MorphUpDate(int frameno)
{
	auto morphData = _vmdData->GetMorphData();

	// ���g�����[�V�����f�[�^���擾
	auto frameIt = std::find_if(morphData.rbegin(), morphData.rend(),
		[frameno](const std::pair<int, std::vector<MorphInfo>>& k) {return k.first <= frameno; });
	if (frameIt == morphData.rend())return;
	// �C�e���[�^�𔽓]�����Ď��̗v�f���Ƃ��Ă���
	auto nextIt = frameIt.base();

	for (auto& nowmd:frameIt->second)
	{
		auto morphName = GetWstringFromString(nowmd.SkinName);
		auto& morphVec = _morphData[morphName];
		for (auto& nowMorph : morphVec)
		{
			int idx = 0;
			if (_morphHeaders[morphName].type == 1)
			{
				// ���v�f���Ȃ������ꍇ
				if (nextIt == morphData.end()) {
					// ���̃��[�t�������̂܂܎g��
					XMStoreFloat3(&vertexInfo[nowMorph.vertexMorph.verIdx].pos,
						XMLoadFloat3(&firstVertexInfo[nowMorph.vertexMorph.verIdx].pos)
					);
				}
				else// ���v�f���������ꍇ
				{
					// �d�݌v�Z
					float pow = (static_cast<float>(frameno) - frameIt->first) / (nextIt->first - frameIt->first);

					// ���̕\������i�[
					auto addVec = XMLoadFloat3(&nowMorph.vertexMorph.pos)*nowmd.Weight;
					for (auto& nextmd : nextIt->second)
					{
						// ���̕\������i�[
						auto addVec2 = XMLoadFloat3(&nowMorph.vertexMorph.pos)*nextmd.Weight;

						// ���̕\��Ǝ��̕\�����`�⊮���č��W�ɓK������
						XMStoreFloat3(&vertexInfo[nowMorph.vertexMorph.verIdx].pos,
							XMVectorLerp(addVec, addVec2, pow));

						XMStoreFloat3(&vertexInfo[nowMorph.vertexMorph.verIdx].pos,
							XMVectorAdd(
								XMLoadFloat3(&vertexInfo[nowMorph.vertexMorph.verIdx].pos),
								XMLoadFloat3(&firstVertexInfo[nowMorph.vertexMorph.verIdx].pos))
						);
					}
				}
			}
			else if (_morphHeaders[morphName].type == 2)
			{
				RotationMatrix(morphName, nowMorph.boneMorph.rotation);
			}
		}
	}
}

void PMXmodel::IKBoneRecursive(int frameno)
{

}


void PMXmodel::PreDrawShadow(ID3D12GraphicsCommandList * list, ID3D12DescriptorHeap * wvp)
{
	float clearColor[] = { 1,1,1,1 };

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

	// �p�C�v���C���̃Z�b�g
	list->SetPipelineState(_shadowMapGPS);

	// ���[�g�V�O�l�`�����Z�b�g
	list->SetGraphicsRootSignature(_shadowMapRS);

	//�r���[�|�[�g�ƃV�U�[�ݒ�
	list->RSSetViewports(1, &_viewPort);
	list->RSSetScissorRects(1, &_scissor);

	// ���f���`��
	// �C���f�b�N�X�o�b�t�@�r���[�̐ݒ�
	list->IASetIndexBuffer(&_idxView);

	// ���_�o�b�t�@�r���[�̐ݒ�
	list->IASetVertexBuffers(0, 1, &_vbView);

	// �g�|���W�[�̃Z�b�g
	list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	unsigned int offset = 0;

	// �ŃX�N���v�^�[�q�[�v�̃Z�b�g(WVP)
	list->SetDescriptorHeaps(1, &wvp);

	// �f�X�N���v�^�e�[�u���̃Z�b�g(WVP)
	D3D12_GPU_DESCRIPTOR_HANDLE wvpStart = wvp->GetGPUDescriptorHandleForHeapStart();
	list->SetGraphicsRootDescriptorTable(0, wvpStart);

	// �f�X�N���v�^�q�[�v�̃Z�b�g(�{�[��) 
	list->SetDescriptorHeaps(1, &_boneHeap);

	// �f�X�N���v�^�e�[�u���̃Z�b�g(�{�[��)
	auto bohandle = _boneHeap->GetGPUDescriptorHandleForHeapStart();
	list->SetGraphicsRootDescriptorTable(1, bohandle);

	for (auto& m : _materials) {
		// �`�敔
		list->DrawIndexedInstanced(m.faceVerCnt, 1, offset, 0, 0);

		// �ϐ��̉��Z
		offset += m.faceVerCnt;
	}
}

HRESULT PMXmodel::CreateShadowRS()
{
	ID3DBlob* signature = nullptr;
	ID3DBlob* error = nullptr;

	// �T���v���̐ݒ�
	D3D12_STATIC_SAMPLER_DESC SamplerDesc[1] = {};
	SamplerDesc[0].Filter = D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR;// ���ʂȃt�B���^���g�p���Ȃ�
	SamplerDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;// �悪�J��Ԃ��`�悳���
	SamplerDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	SamplerDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	SamplerDesc[0].MaxLOD = D3D12_FLOAT32_MAX;// ����Ȃ�
	SamplerDesc[0].MinLOD = 0.0f;// �����Ȃ�
	SamplerDesc[0].MipLODBias = 0.0f;// MIPMAP�̃o�C�A�X
	SamplerDesc[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;// �G�b�W�̐F(��)
	SamplerDesc[0].ShaderRegister = 0;// �g�p���郌�W�X�^�X���b�g
	SamplerDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;// �ǂ̂��炢�̃f�[�^���V�F�[�_�Ɍ����邩(�S��)
	
	// �����W�̐ݒ�
	D3D12_DESCRIPTOR_RANGE descRange[2] = {};
	//WVP
	descRange[0].NumDescriptors = 1;
	descRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;//�萔
	descRange[0].BaseShaderRegister = 0;//���W�X�^�ԍ�
	descRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	// �{�[��
	descRange[1].NumDescriptors = 1;
	descRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;//�萔
	descRange[1].BaseShaderRegister = 1;//���W�X�^�ԍ�
	descRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ���[�g�p�����[�^�ϐ��̐ݒ�
	D3D12_ROOT_PARAMETER rootParam[2] = {};
	rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[0].DescriptorTable.pDescriptorRanges = &descRange[0];//�Ή����郌���W�ւ̃|�C���^
	rootParam[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//���ׂẴV�F�[�_����Q��

	// �{�[��
	rootParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[1].DescriptorTable.pDescriptorRanges = &descRange[1];
	rootParam[1].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	// ���[�g�V�O�l�`������邽�߂̕ϐ��̐ݒ�
	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.pStaticSamplers = SamplerDesc;
	rsd.pParameters = rootParam;
	rsd.NumParameters = 2;
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
		IID_PPV_ARGS(&_shadowMapRS));

	return result;
}

HRESULT PMXmodel::CreateShadowPS()
{
	auto result = D3DCompileFromFile(L"Shadow.hlsl", nullptr, nullptr, "shadowVS", "vs_5_0", D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_shadowVertShader, nullptr);

	// �s�N�Z���V�F�[�_
	result = D3DCompileFromFile(L"Shadow.hlsl", nullptr, nullptr, "shadowPS", "ps_5_0", D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_shadowPixShader, nullptr);

	auto InputLayout = GetInputLayout();

	// �p�C�v���C������邽�߂�GPS�̕ϐ��̍쐬
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};

	//���[�g�V�O�l�`���ƒ��_���C�A�E�g
	gpsDesc.pRootSignature = _shadowMapRS;
	gpsDesc.InputLayout.pInputElementDescs = InputLayout.data();// �z��̊J�n�ʒu
	gpsDesc.InputLayout.NumElements = static_cast<unsigned int>(InputLayout.size());// �v�f�̐�������

	//�V�F�[�_�̃Z�b�g
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(_shadowVertShader);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(_shadowPixShader);

	// �����_�[�^�[�Q�b�g���̎w��(����̓����_�[�^�[�Q�b�g���Ȃ����߂O)
	gpsDesc.NumRenderTargets = 0;

	//�[�x�X�e���V��
	gpsDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	gpsDesc.DepthStencilState.DepthEnable = true;
	gpsDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;// �K�{
	gpsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;// �������ق���ʂ�

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

	result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&_shadowMapGPS));

	return result;
}

PMXmodel::PMXmodel(ID3D12Device* dev, const std::string modelPath, const std::string vmdPath):_dev(dev)
{
	LoadModel(modelPath, vmdPath);
}


PMXmodel::~PMXmodel()
{

}

HRESULT PMXmodel::CreateWhiteTexture()
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

HRESULT PMXmodel::CreateBlackTexture()
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

HRESULT PMXmodel::CreateVertexBuffer()
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

	auto desc = CD3DX12_RESOURCE_DESC::Buffer(vertexInfo.size() * sizeof(PMXVertexInfo));
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

HRESULT PMXmodel::CreateIndexBuffer()
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

HRESULT PMXmodel::CreateMaterialBuffer()
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
			if (_materials[i].toonIndex < _texturePaths.size())
			{
				toonFilePath = FolderPath;
				sprintf_s(toonFileName, _texturePaths[_materials[i].toonIndex].c_str());
			}
		}

		toonFilePath += toonFileName;
		_toonResources[i] = LoadTextureFromFile(toonFilePath);

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
	std::fill(_boneMats.begin(), _boneMats.end(), DirectX::XMMatrixIdentity());
	
	// �����̃f�[�^������
	for (int idx = 0; idx < _bones.size(); ++idx) {
		// �����̃{�[���C���f�b�N�X���i�[����
		_boneTree[_bones[idx].name].boneIdx = idx;
		if (_bones[idx].boneIndex < _bones.size())
		{
			// �����̃|�W�V�������i�[����
			_boneTree[_bones[idx].name].startPos = _bones[idx].pos;
		}
	}		for (auto& b : _boneTree) {
		if (_bones[b.second.boneIdx].parentboneIndex < _bones.size())// �e�̃{�[���ԍ����{�[�������𒴂��Ă��Ȃ���΃m�[�h�ǉ�
		{
			auto parentName = _bones[_bones[b.second.boneIdx].parentboneIndex].name;// �e�̖��O���m��

			_boneTree[parentName].children.push_back(&b.second);// �e�Ɏ����̃m�[�h�����q�Ƃ��Ēǉ�
		}
	}
}

// PMX���f���`��p�p�C�v���C���X�e�[�g�̐���
HRESULT PMXmodel::CreatePipeline()
{
	// �V�F�[�_�[�̓ǂݍ���
	ID3DBlob* vertexShader = nullptr;
	ID3DBlob* pixelShader = nullptr;

	// ���_�V�F�[�_
	auto result = D3DCompileFromFile(L"PMXShader.hlsl", nullptr, nullptr, "vs", "vs_5_0", D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION, 0, &vertexShader, nullptr);

	// �s�N�Z���V�F�[�_
	result = D3DCompileFromFile(L"PMXShader.hlsl", nullptr, nullptr, "ps", "ps_5_0", D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pixelShader, nullptr);
	auto inputLayout = GetInputLayout();

	// �p�C�v���C������邽�߂�GPS�̕ϐ��̍쐬
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};

	//���[�g�V�O�l�`���ƒ��_���C�A�E�g
	gpsDesc.pRootSignature = _pmxSignature;
	gpsDesc.InputLayout.pInputElementDescs = inputLayout.data();// �z��̊J�n�ʒu
	gpsDesc.InputLayout.NumElements = static_cast<unsigned int>(inputLayout.size());// �v�f�̐�������

	//�V�F�[�_�̃Z�b�g
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

	result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&_pmxPipeline));

	return result;
}

// PMX���f���`��p���[�g�V�O�l�`���̐���
HRESULT PMXmodel::CreateRootSignature()
{
	ID3DBlob* signature = nullptr;
	ID3DBlob* error = nullptr;

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

	SamplerDesc[1] = SamplerDesc[0];//�ύX�_�ȊO���R�s�[
	SamplerDesc[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;//�J��Ԃ��Ȃ�
	SamplerDesc[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;//�J��Ԃ��Ȃ�
	SamplerDesc[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;//�J��Ԃ��Ȃ�
	SamplerDesc[1].ShaderRegister = 1; //�V�F�[�_�X���b�g�ԍ���Y��Ȃ��悤��

	// �����W�̐ݒ�
	D3D12_DESCRIPTOR_RANGE descRange[5] = {};// �e�N�X�`���ƒ萔��Ƃځ[��
	// WVP
	descRange[0].NumDescriptors = 1;
	descRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;//�萔
	descRange[0].BaseShaderRegister = 0;//���W�X�^�ԍ�
	descRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// �}�e���A��
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

	// syadou 
	descRange[4].NumDescriptors = 1;
	descRange[4].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descRange[4].BaseShaderRegister = 4;//���W�X�^�ԍ�
	descRange[4].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ���[�g�p�����[�^�ϐ��̐ݒ�
	D3D12_ROOT_PARAMETER rootParam[4] = {};
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

	// ����ǂ���
	rootParam[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[3].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[3].DescriptorTable.pDescriptorRanges = &descRange[4];
	rootParam[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// ���[�g�V�O�l�`������邽�߂̕ϐ��̐ݒ�
	D3D12_ROOT_SIGNATURE_DESC rsd = {};
	rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsd.pStaticSamplers = SamplerDesc;
	rsd.pParameters = rootParam;
	rsd.NumParameters = 4;
	rsd.NumStaticSamplers = 2;

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
		IID_PPV_ARGS(&_pmxSignature));

	return result;
}

HRESULT PMXmodel::CreateBoneBuffer()
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

	result = _boneBuffer->Map(0, nullptr, (void**)&_sendBone);
	
	std::copy(std::begin(_boneMats), std::end(_boneMats), _sendBone);

	return result;
}

void PMXmodel::RotationMatrix(const std::wstring bonename, const XMFLOAT4 &quat1, const XMFLOAT4 &quat2, float pow)
{
	auto bonenode = _boneTree[bonename];
	auto start = XMLoadFloat3(&bonenode.startPos);// ���̍��W�����Ă���
	auto quaternion = XMLoadFloat4(&quat1);
	auto quaternion2 = XMLoadFloat4(&quat2);


	//���_�܂ŕ��s�ړ����Ă����ŉ�]���s���A���̈ʒu�܂Ŗ߂�
	_boneMats[bonenode.boneIdx] =
		DirectX::XMMatrixTranslationFromVector(XMVectorScale(start, -1))*
		XMMatrixRotationQuaternion(XMQuaternionSlerp(quaternion, quaternion2, pow))*
		DirectX::XMMatrixTranslationFromVector(start);
}

void PMXmodel::RotationMatrix(const std::wstring bonename, const XMFLOAT4 &quat1)
{
	auto bonenode = _boneTree[bonename];
	auto start = XMLoadFloat3(&bonenode.startPos);// ���̍��W�����Ă���
	auto quaternion = XMLoadFloat4(&quat1);
	auto bonetype = _bones[bonenode.boneIdx];


	//���_�܂ŕ��s�ړ����Ă����ŉ�]���s���A���̈ʒu�܂Ŗ߂�
	_boneMats[bonenode.boneIdx] =
		DirectX::XMMatrixTranslationFromVector(XMVectorScale(start, -1))*
		XMMatrixRotationQuaternion(quaternion)*
		DirectX::XMMatrixTranslationFromVector(start);
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

float PMXmodel::GetBezierPower(float x, const XMFLOAT2 & a, const XMFLOAT2 & b, uint8_t n)
{
	// a��b�������Ȃ�v�Z����K�v�Ȃ�
	if (a.x == a.y&&b.x == b.y)
	{
		return x;
	}

	float t = x;

	const float k0 = 1 + 3 * a.x - 3 * b.x;//t^3�̌W��
	const float k1 = 3 * b.x - 6 * a.x;//t^2�̌W��
	const float k2 = 3 * a.x;//t�̌W��

	constexpr float epsilon = 0.0005f;// �v�Z����߂�덷

	for (int i = 0; i < n; ++i) {
		
		auto ft = k0 * t*t*t + k1 * t*t + k2 * t - x;

		// �v�Z���ʂ��덷�͈̔͂Ȃ�v�Z�I��
		if (ft <= epsilon && ft >= -epsilon)
		{
			break;
		}
		t -= ft / 2;// ��Ŋ����Ă���
	}

	// y���v�Z����
	auto r = 1 - t;
	return t * t*t + 3 * t*t*r*b.y + 3 * t*r*r*a.y;
}

void PMXmodel::UpDate(char key[256])
{
	if (key[DIK_F])
	{
		for (auto &md: _morphData[L"����"])
		{
			if (_materials[md.materialMorph.materialIdx].Diffuse.w < 1.f)
			{
				_materials[md.materialMorph.materialIdx].Diffuse.w += 0.1f;
			}
		}
	}
	else
	{
		for (auto &md : _morphData[L"����"])
		{
			if (_materials[md.materialMorph.materialIdx].Diffuse.w > 0.f)
			{
				_materials[md.materialMorph.materialIdx].Diffuse.w -= 0.1f;
			}
		}
	}

	// �}�e���A���J���[�̓]��
	int midx = 0;
	for (auto& mbuff : _materialsBuff) {

		mbuff->Map(0, nullptr, (void**)&MapColor);
		MapColor->diffuse = _materials[midx].Diffuse;

		MapColor->ambient = _materials[midx].Ambient;

		MapColor->specular.x = _materials[midx].Specular.x;
		MapColor->specular.y = _materials[midx].Specular.y;
		MapColor->specular.z = _materials[midx].Specular.z;
		MapColor->specular.w = _materials[midx].SpecularPow;

		mbuff->Unmap(0, nullptr);
		++midx;
	}
	if (_vmdData)
	{
		// ���[�V�����̍X�V
		static auto lastTime = GetTickCount();
		auto TotalFrame = _vmdData->GetTotalFrame();
		auto NowFrame = static_cast<float>(GetTickCount() - lastTime) / 33.33333f;

		if (TotalFrame < NowFrame)
		{
			lastTime = GetTickCount();
		}

		MotionUpDate(NowFrame);// �����̍X�V
		MorphUpDate(NowFrame); // �\��̍X�V
	}

	// �o�b�t�@�̍X�V
	BufferUpDate();
}

void PMXmodel::Draw(ID3D12GraphicsCommandList* list, ID3D12DescriptorHeap* wvp, ID3D12DescriptorHeap*shadow)
{
	float clearColor[] = { 0.5f,0.5f,0.5f,0.0f };

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

	// �p�C�v���C���̃Z�b�g
	list->SetPipelineState(_pmxPipeline);

	// ���[�g�V�O�l�`�����Z�b�g
	list->SetGraphicsRootSignature(_pmxSignature);

	//�r���[�|�[�g�ƃV�U�[�ݒ�
	list->RSSetViewports(1, &_viewPort);
	list->RSSetScissorRects(1, &_scissor);

	// ���f���`��
	// �C���f�b�N�X�o�b�t�@�r���[�̐ݒ�
	list->IASetIndexBuffer(&_idxView);

	// ���_�o�b�t�@�r���[�̐ݒ�
	list->IASetVertexBuffers(0, 1, &_vbView);

	// �g�|���W�[�̃Z�b�g
	list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	unsigned int offset = 0;
	auto boneheap = _boneHeap;
	auto materialheap = _matDescHeap;

	// �ŃX�N���v�^�[�q�[�v�̃Z�b�g(WVP)
	list->SetDescriptorHeaps(1, &wvp);

	// �f�X�N���v�^�e�[�u���̃Z�b�g(WVP)
	D3D12_GPU_DESCRIPTOR_HANDLE wvpStart = wvp->GetGPUDescriptorHandleForHeapStart();
	list->SetGraphicsRootDescriptorTable(0, wvpStart);

	// �f�X�N���v�^�q�[�v�̃Z�b�g(�{�[��) 
	list->SetDescriptorHeaps(1, &boneheap);

	// �f�X�N���v�^�e�[�u���̃Z�b�g(�{�[��)
	auto bohandle = boneheap->GetGPUDescriptorHandleForHeapStart();
	list->SetGraphicsRootDescriptorTable(2, bohandle);

	// �ŃX�N���v�^�[�̃Z�b�g(shadow)
	list->SetDescriptorHeaps(1, &shadow);
	auto shadowStart = shadow->GetGPUDescriptorHandleForHeapStart();
	list->SetGraphicsRootDescriptorTable(3, shadowStart);

	// �`�惋�[�v
	// �}�e���A���̃f�X�N���v�^�q�[�v�̃Z�b�g
	list->SetDescriptorHeaps(1, &materialheap);

	// �f�X�N���v�^�[�n���h���ꖇ�̃T�C�Y�擾
	int incsize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// �}�e���A���q�[�v�̊J�n�ʒu�擾
	auto mathandle = materialheap->GetGPUDescriptorHandleForHeapStart();

	for (auto& m : _materials) {
		// �f�X�N���v�^�e�[�u���̃Z�b�g
		list->SetGraphicsRootDescriptorTable(1, mathandle);

		// �`�敔
		list->DrawIndexedInstanced(m.faceVerCnt, 1, offset, 0, 0);

		// �|�C���^�̉��Z
		mathandle.ptr += incsize * 5;// 5�����邩��5�{

		// �ϐ��̉��Z
		offset += m.faceVerCnt;
	}
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
		{"BONENO",0,DXGI_FORMAT_R32G32B32A32_SINT,0,D3D12_APPEND_ALIGNED_ELEMENT,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		// weight
		{"WEIGHT",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},

		{"SDEFDATA",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"SDEFDATA",1,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"SDEFDATA",2,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
				
		{"EDGESIZE",0,DXGI_FORMAT_R32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		
		{"INSTID",0,DXGI_FORMAT_R32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
	};
	return inputLayoutDescs;
}

const XMFLOAT3 PMXmodel::GetPos()
{
	return _bones[_boneTree[L"�Z���^�["].boneIdx].pos;
}

const LPCWSTR PMXmodel::GetUseShader()
{
	return L"PMXShader.hlsl";
}

ID3D12Resource * PMXmodel::LoadTextureFromFile(std::string & texPath)
{
	// �ǂݍ������Ƃ����e�N�X�`�������łɓǂݍ��񂾂��Ƃ̂���e�N�X�`���Ȃ�ǂݍ��܂Ȃ�
	auto tex = _texMap.find(texPath);
	if (tex != _texMap.end())
	{
		return (*tex).second;
	}

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
	// �e�N�X�`���}�b�v�ɒǉ�
	_texMap[texPath] = texbuff;

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
	wstr.resize(num1-1);// ���̂܂܂��ƈ�ԍŌ�ɋ󔒕����������Ă��܂����߃}�C�i�X�P����

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

HRESULT PMXmodel::CreateGrayGradationTexture()
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

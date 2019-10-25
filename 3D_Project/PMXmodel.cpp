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

	// ヘッダー情報の読み込み
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
		fread(&vi, sizeof(vi.pos)+ sizeof(vi.nomal)+ sizeof(vi.uv), 1, fp);
		
		// 追加UV読み込み
		for (int i = 0; i < header.data[1]; ++i)
		{
			fread(&vi.adduv[i], sizeof(vi.adduv), 1, fp);
		}

		// weight変形形式よっみこみ
		fread(&vi.weight, sizeof(vi.weight), 1, fp);

		// 変形形式別の読み込み
		if (vi.weight == 0)
		{
			fread(&vi.boneIdxSize[0],header.data[5], 1, fp);
		}
		else if (vi.weight == 1)
		{
			fread(&vi.boneIdxSize[0], header.data[5], 1, fp);
			fread(&vi.boneIdxSize[1], header.data[5], 1, fp);
			fread(&vi.boneweight[0], sizeof(float), 1, fp);
		}
		else if (vi.weight == 2)
		{
			fread(&vi.boneIdxSize[0], header.data[5], 1, fp);
			fread(&vi.boneIdxSize[1], header.data[5], 1, fp);
			fread(&vi.boneIdxSize[2], header.data[5], 1, fp);
			fread(&vi.boneIdxSize[3], header.data[5], 1, fp);
			fread(&vi.boneweight[0], sizeof(float), 1, fp);
			fread(&vi.boneweight[1], sizeof(float), 1, fp);
			fread(&vi.boneweight[2], sizeof(float), 1, fp);
			fread(&vi.boneweight[3], sizeof(float), 1, fp);
		}
		else if (vi.weight == 3)
		{
			fread(&vi.boneIdxSize[0], header.data[5], 1, fp);
			fread(&vi.boneIdxSize[1], header.data[5], 1, fp);
			fread(&vi.boneweight[0], sizeof(float), 1, fp);
			fread(&vi.SDEFdata[0], sizeof(vi.SDEFdata[0]), 1, fp);
			fread(&vi.SDEFdata[1], sizeof(vi.SDEFdata[1]), 1, fp);
			fread(&vi.SDEFdata[2], sizeof(vi.SDEFdata[2]), 1, fp);
		}

		// エッジの読み込み
		fread(&vi.edge, sizeof(float), 1, fp);
	}

	// インデックス読み込み
	int indexNum = 0;
	fread(&indexNum, sizeof(indexNum), 1, fp);
	_verindex.resize(indexNum);
	for (int i= 0;i< indexNum;++i)
	{
		fread(&_verindex[i], header.data[2], 1, fp);
	}

	// テクスチャ読み込み
	int texNum = 0;
	fread(&texNum, sizeof(texNum), 1, fp);
	_texVec.resize(texNum);
	std::string str;
	for (int i = 0; i < texNum; ++i)
	{
		int pathnum = 0;
		fread(&pathnum, sizeof(int), 1, fp);
		for (int num = 0;num < pathnum/2;++num)
		{
			wchar_t c;
			fread(&c, sizeof(wchar_t), 1, fp);
			str += c;
		}
		_texVec[i] = str;
		str = {};
	}

	// マテリアル読み込み
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


		fread(&_materials[i].Diffuse, sizeof(_materials[i].Diffuse), 1, fp);// ディフューズの読み込み

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

	// ボーン読み込み
	int boneNum = 0;
	fread(&boneNum, sizeof(boneNum), 1, fp);
	_bones.resize(boneNum);

	std::wstring jbonename;
	std::wstring ebonename;
	for (int i = 0;i< boneNum;++i)
	{
		int jnamenum;
		fread(&jnamenum, sizeof(jnamenum), 1, fp);
		jbonename.resize(jnamenum /2);
		for (auto& bn: jbonename)
		{
			fread(&bn, sizeof(bn), 1, fp);
		}

		int enamenum;
		fread(&enamenum, sizeof(enamenum), 1, fp);
		ebonename.resize(enamenum / 2);
		for (auto& bn : ebonename)
		{
			fread(&bn, sizeof(bn), 1, fp);
		}
	
		fread(&_bones[i].pos, sizeof(_bones[i].pos), 1, fp);
		fread(&_bones[i].parentboneIndex, header.data[5], 1, fp);
		fread(&_bones[i].tranceLevel, sizeof(_bones[i].tranceLevel), 1, fp);
		fread(&_bones[i].bitFlag, sizeof(_bones[i].bitFlag), 1, fp);

		// 接続先
		if (_bones[i].bitFlag & 0x0001)
		{
			fread(&_bones[i].boneIndex, header.data[5], 1, fp);
		}
		else 
		{
			fread(&_bones[i].offset, sizeof(_bones[i].offset), 1, fp);
		}

		// 回転付与 と 移動付与
		if ((_bones[i].bitFlag & 0x0100) || (_bones[i].bitFlag & 0x0200))
		{
			fread(&_bones[i].grantIndex, header.data[5], 1, fp);
			fread(&_bones[i].grantPar, sizeof(_bones[i].grantPar), 1, fp);
		}

		// 軸固定
		if (_bones[i].bitFlag & 0x0400)
		{
			fread(&_bones[i].axisvector, sizeof(_bones[i].axisvector), 1, fp);
		}

		// ローカル軸
		if (_bones[i].bitFlag & 0x0800)
		{
			fread(&_bones[i].axisXvector, sizeof(_bones[i].axisXvector), 1, fp);
			fread(&_bones[i].axisZvector, sizeof(_bones[i].axisZvector), 1, fp);
		}

		// 外部親変形
		if (_bones[i].bitFlag & 0x2000)
		{
			fread(&_bones[i].key, sizeof(_bones[i].key), 1, fp);
		}

		// IKデータ
		if (_bones[i].bitFlag & 0x0020)
		{
			fread(&_bones[i].IkData.boneIdx, header.data[5], 1, fp);
			fread(&_bones[i].IkData.loopCnt, sizeof(_bones[i].IkData.loopCnt), 1, fp);
			fread(&_bones[i].IkData.limrad, sizeof(_bones[i].IkData.limrad), 1, fp);

			// IKリンク
			fread(&_bones[i].IkData.linkNum, sizeof(_bones[i].IkData.linkNum), 1, fp);
			for (int num = 0;num< _bones[i].IkData.linkNum;++num)
			{
				fread(&_bones[i].IkData.linkboneIdx, header.data[5], 1, fp);
				fread(&_bones[i].IkData.isRadlim, sizeof(_bones[i].IkData.isRadlim), 1, fp);
				if (_bones[i].IkData.isRadlim)
				{
					fread(&_bones[i].IkData.minRadlim, sizeof(_bones[i].IkData.minRadlim), 1, fp);
					fread(&_bones[i].IkData.maxRadlim, sizeof(_bones[i].IkData.maxRadlim), 1, fp);
				}
			}
		}
		_boneMap[jbonename] = _bones[i];
	}

	// もーふ(表情)の読み込み
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

	// 各バッファの初期化
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

	// しろてくすちゃつくる
	auto result = CreateWhiteTexture(_dev);
	// くろてくすちゃつくる
	result = CreateBlackTexture(_dev);
	// グラデーションテクスチャつくる
	result = CreateGrayGradationTexture(_dev);

	result = CreateMaterialBuffer(_dev);

	CreateBoneTree();
	result= CreateBoneBuffer(_dev);
}

PMXmodel::PMXmodel(ID3D12Device* _dev, const std::string modelPath)
{
	LoadModel(_dev, modelPath);
}


PMXmodel::~PMXmodel()
{
}

HRESULT PMXmodel::CreateWhiteTexture(ID3D12Device* _dev)
{
	// 白バッファ用のデータ配列
	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0xff);//全部255で埋める

	// 転送する用のヒープ設定
	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0
	);

	// 転送する用のデスク設定
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM,
		4,
		4
	);

	// 白テクスチャの作成
	auto result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,//特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&whiteTex)
	);

	result = whiteTex->WriteToSubresource(0, nullptr, data.data(), 4 * 4, data.size());

	return result;
}

HRESULT PMXmodel::CreateBlackTexture(ID3D12Device* _dev)
{
	// 黒バッファ用のデータ配列
	std::vector<unsigned char> data(4 * 4 * 4);
	std::fill(data.begin(), data.end(), 0);//全部0で埋める

	// 転送する用のヒープ設定
	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0
	);

	// 転送する用のデスク設定
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM,
		4,
		4
	);

	// 黒テクスチャの作成
	auto result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,//特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&blackTex)
	);

	result = blackTex->WriteToSubresource(0, nullptr, data.data(), 4 * 4, data.size());

	return result;
}

HRESULT PMXmodel::CreateMaterialBuffer(ID3D12Device* _dev)
{
	D3D12_DESCRIPTOR_HEAP_DESC matDescHeap = {};
	matDescHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	matDescHeap.NodeMask = 0;

	// 定数バッファとシェーダーリソースビューとSphとSpaとトゥーンの５枚↓
	matDescHeap.NumDescriptors = _materials.size() * 5;
	matDescHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	auto result = _dev->CreateDescriptorHeap(&matDescHeap, IID_PPV_ARGS(&_matDescHeap));

	size_t size = (sizeof(PMXColor) + 0xff)&~0xff;

	_materialsBuff.resize(_materials.size());

	int midx = 0;
	for (auto& mbuff : _materialsBuff) {

		// マテリアルバッファの作成
		auto result = _dev->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(size),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mbuff));

		// マテリアルバッファのマッピング
		result = mbuff->Map(0, nullptr, (void**)&MapColor);

		MapColor->diffuse = _materials[midx].Diffuse;

		MapColor->ambient = _materials[midx].Ambient;

		MapColor->specular.x = _materials[midx].Specular.x;
		MapColor->specular.y = _materials[midx].Specular.y;
		MapColor->specular.z = _materials[midx].Specular.z;
		MapColor->specular.w = _materials[midx].SpecularPow;

		++midx;
	}

	// 定数バッファビューデスクの作成
	D3D12_CONSTANT_BUFFER_VIEW_DESC matDesc = {};

	// シェーダーリソースビューデスクの作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	auto matHandle = _matDescHeap->GetCPUDescriptorHandleForHeapStart();
	auto addsize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (int i = 0; i < _materialsBuff.size(); ++i)
	{
		//トゥーンリソースの読み込み
		std::string toonFilePath = "toon/";
		char toonFileName[16];
		sprintf_s(toonFileName, "toon%02d.bmp", _materials[i].toonIndex + 1);
		// 固有トゥーンの場合分け
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

		// 定数バッファの作成
		matDesc.BufferLocation = _materialsBuff[i]->GetGPUVirtualAddress();
		matDesc.SizeInBytes = size;
		_dev->CreateConstantBufferView(&matDesc, matHandle);// 定数バッファビューの作成
		matHandle.ptr += addsize;// ポインタの加算

		// テクスチャ用シェーダリソースビューの作成
		if (_textureBuffer[i] == nullptr) {
			srvDesc.Format = whiteTex->GetDesc().Format;
			_dev->CreateShaderResourceView(whiteTex, &srvDesc, matHandle);
		}
		else
		{
			srvDesc.Format = _textureBuffer[i]->GetDesc().Format;// テクスチャのフォーマットの取得
			_dev->CreateShaderResourceView(_textureBuffer[i], &srvDesc, matHandle);// シェーダーリソースビューの作成
		}
		matHandle.ptr += addsize;// ポインタの加算

		// 乗算スフィアマップSRVの作成
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

		// 加算スフィアマップSRVの作成
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

		// トゥーンのSRVの作成
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
	// 単位行列で初期化
	std::fill(_boneMats.begin(), _boneMats.end(), DirectX::XMMatrixIdentity());
}

HRESULT PMXmodel::CreateBoneBuffer(ID3D12Device* _dev)
{
	// サイズを調整
	size_t size = sizeof(DirectX::XMMATRIX)*_bones.size();
	size = (size + 0xff)&~0xff;

	// ボーンバッファの作成
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

void PMXmodel::RotationMatrix(std::string bonename, DirectX::XMFLOAT3 theta)
{
	auto boneNode = _boneMap[GetWstringFromString(bonename)];
	auto vec = DirectX::XMLoadFloat3(&boneNode.pos);// 元の座標を入れておく

	 //原点まで並行移動してそこで回転を行い、元の位置まで戻す
	_boneMats[boneNode.parentboneIndex] =
		DirectX::XMMatrixTranslationFromVector(DirectX::XMVectorScale(vec, -1))*/* 原点に移動 */		DirectX::XMMatrixRotationX(theta.x)*									/* ボーン行列の回転(X軸) */		DirectX::XMMatrixRotationY(theta.y)*									/* ボーン行列の回転(Y軸) */		DirectX::XMMatrixRotationZ(theta.z)*									/* ボーン行列の回転(Z軸) */		DirectX::XMMatrixTranslationFromVector(vec);							/* 元の座標に戻す */
}

void PMXmodel::RecursiveMatrixMultiply(BoneInfo& node, DirectX::XMMATRIX& MultiMat)
{
	// 行列を乗算する
	_boneMats[node.parentboneIndex] *= MultiMat;
	if (node.boneIndex == 0xffff)
	{
		return;
	}
	// 再帰する
	for (int i = 0; i < node.boneIndex;++i) {
		RecursiveMatrixMultiply(_bones[node.boneIndex], _boneMats[node.parentboneIndex]);
	}
}

void PMXmodel::UpDate(char key[256])
{
	if (key[DIK_F])
	{
		if (_materials[21].Diffuse.w < 1.f)
		{
			_materials[21].Diffuse.w += 0.01f;
		}
	}
	else
	{
		if (_materials[21].Diffuse.w > 0.f)
		{
			_materials[21].Diffuse.w -= 0.01f;
		}
	}

	if (key[DIK_T])
	{
		if (_materials[22].Diffuse.w < 1.f)
		{
			_materials[22].Diffuse.w += 0.01f;
		}
	}
	else
	{
		if (_materials[22].Diffuse.w > 0.f)
		{
			_materials[22].Diffuse.w -= 0.01f;
		}
	}

	

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
	angle = 1.0f;
	std::fill(_boneMats.begin(), _boneMats.end(), DirectX::XMMatrixIdentity());
	RotationMatrix("右腕", DirectX::XMFLOAT3(0, angle, 0));

	DirectX::XMMATRIX rootmat = DirectX::XMMatrixIdentity();
	RecursiveMatrixMultiply(_boneMap[GetWstringFromString("右肩")], rootmat);

	std::copy(_boneMats.begin(), _boneMats.end(), _mappedBones);
}

ID3D12Resource * PMXmodel::LoadTextureFromFile(std::string & texPath, ID3D12Device* _dev)
{
	//WICテクスチャのロード
	DirectX::TexMetadata metadata = {};
	DirectX::ScratchImage scratchImg = {};

	// テクスチャファイルのロード
	auto result = LoadFromWICFile(GetWstringFromString(texPath).c_str(),
		DirectX::WIC_FLAGS_NONE,
		&metadata,
		scratchImg);
	if (FAILED(result)) {
		return whiteTex;// 失敗したら白テクスチャを入れる
	}

	// イメージデータを搾取
	auto img = scratchImg.GetImage(0, 0, 0);

	// 転送する用のヒープ設定(pdf:p250) 
	auto texHeapProp = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0
	);

	// リソースデスクの再設定
	auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		metadata.format,
		metadata.width,
		metadata.height,
		metadata.arraySize,
		metadata.mipLevels
	);

	// テクスチャバッファの作成
	ID3D12Resource* texbuff = nullptr;
	result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,//特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&texbuff)
	);

	// 転送する
	result = texbuff->WriteToSubresource(0,
		nullptr,
		img->pixels,
		img->rowPitch,
		img->slicePitch
	);

	return texbuff;
}

std::wstring PMXmodel::GetWstringFromString(const std::string& str)
{
	//呼び出し1回目(文字列数を得る)
	auto num1 = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
		str.c_str(),
		-1,
		nullptr,
		0
	);

	// 得た文字数分のワイド文字列を作成
	std::wstring wstr;
	wstr.resize(num1);

	//呼び出し2回目(確保済みのwstrに変換文字列をコピー)
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
	// 転送する用のヒープ設定
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

	// 上が白くてしたが黒いテクスチャデータの作成
	auto result = _dev->CreateCommittedResource(
		&texHeapProp,
		D3D12_HEAP_FLAG_NONE,//特に指定なし
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
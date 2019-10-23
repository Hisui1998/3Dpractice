#pragma once
#include "PMXmodel.h"
#include <d3d12.h>
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
	PMXHeader header;
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
			fread(&vi.boneIdxSize[0],header.data[2], 1, fp);
		}
		else if (vi.weight == 1)
		{
			fread(&vi.boneIdxSize[0], header.data[2], 1, fp);
			fread(&vi.boneIdxSize[1], header.data[2], 1, fp);
			fread(&vi.boneweight[0], sizeof(float), 1, fp);
		}
		else if (vi.weight == 2)
		{
			fread(&vi.boneIdxSize[0], header.data[2], 1, fp);
			fread(&vi.boneIdxSize[1], header.data[2], 1, fp);
			fread(&vi.boneIdxSize[2], header.data[2], 1, fp);
			fread(&vi.boneIdxSize[3], header.data[2], 1, fp);
			fread(&vi.boneweight[0], sizeof(float), 1, fp);
			fread(&vi.boneweight[1], sizeof(float), 1, fp);
			fread(&vi.boneweight[2], sizeof(float), 1, fp);
			fread(&vi.boneweight[3], sizeof(float), 1, fp);
		}
		else if (vi.weight == 3)
		{
			fread(&vi.boneIdxSize[0], header.data[2], 1, fp);
			fread(&vi.boneIdxSize[1], header.data[2], 1, fp);
			fread(&vi.boneweight[0], sizeof(float), 1, fp);
			fread(&vi.SDEFdata[0], header.data[2], 1, fp);
			fread(&vi.SDEFdata[1], header.data[2], 1, fp);
			fread(&vi.SDEFdata[2], header.data[2], 1, fp);
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

		fread(&_materials[i], 65, 1, fp);
		fread(&_materials[i].texIndex, header.data[3], 1, fp);
		fread(&_materials[i].sphereIndex, header.data[3], 1, fp);
		fread(&_materials[i].sphereMode, sizeof(_materials[i].sphereMode), 1, fp);
		fread(&_materials[i].toonFlag, sizeof(_materials[i].toonFlag), 1, fp);
		fread(&_materials[i].toonIndex, _materials[i].toonFlag?sizeof(unsigned char):header.data[3], 1, fp);
		
		fread(&num, sizeof(num), 1, fp);
		fseek(fp, num, SEEK_CUR);
		fread(&_materials[i].faceVerCnt, sizeof(_materials[i].faceVerCnt), 1, fp);
	}

	fclose(fp);

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

	size_t size = (sizeof(PMXMaterial) + 0xff)&~0xff;

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

		mbuff->Unmap(0, nullptr);
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
	_boneMats.resize(512);
	// 単位行列で初期化
	std::fill(_boneMats.begin(), _boneMats.end(), DirectX::XMMatrixIdentity());
}

HRESULT PMXmodel::CreateBoneBuffer(ID3D12Device* _dev)
{
	// サイズを調整
	size_t size = sizeof(DirectX::XMMATRIX)*512;
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

void PMXmodel::UpDate()
{
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
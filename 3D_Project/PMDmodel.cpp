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
	// PMDファイルの読み込み
	LoadModel(_dev,modelPath);
}

PMDmodel::~PMDmodel()
{
}

void PMDmodel::UpDate()
{
	angle+=10*DirectX::XM_PI/180;
	// 実験
	std::fill(_boneMats.begin(), _boneMats.end(), DirectX::XMMatrixIdentity());
	RotationMatrix("右肩", DirectX::XMFLOAT3(angle, 0, 0));	RotationMatrix("右ひじ", DirectX::XMFLOAT3(angle, 0, 0));	RotationMatrix("左肩", DirectX::XMFLOAT3(angle, 0, 0));	RotationMatrix("左ひじ", DirectX::XMFLOAT3(angle, 0, 0));

	DirectX::XMMATRIX rootmat = DirectX::XMMatrixIdentity();
	RecursiveMatrixMultiply(_boneMap["センター"], rootmat);

	std::copy(_boneMats.begin(), _boneMats.end(), _mappedBones);
}



void PMDmodel::RotationMatrix(std::string bonename, DirectX::XMFLOAT3 theta)
{
	auto boneNode = _boneMap[bonename];	
	auto vec = DirectX::XMLoadFloat3(&boneNode.startPos);// 元の座標を入れておく

	// 原点まで並行移動してそこで回転を行い、元の位置まで戻す
	_boneMats[boneNode.boneIdx] =
		DirectX::XMMatrixTranslationFromVector(DirectX::XMVectorScale(vec, -1))*/* 原点に移動 */		DirectX::XMMatrixRotationX(theta.x)*									/* ボーン行列の回転(X軸) */		DirectX::XMMatrixRotationY(theta.y)*									/* ボーン行列の回転(Y軸) */		DirectX::XMMatrixRotationZ(theta.z)*									/* ボーン行列の回転(Z軸) */		DirectX::XMMatrixTranslationFromVector(vec);							/* 元の座標に戻す */
}

void PMDmodel::RecursiveMatrixMultiply(BoneNode& node, DirectX::XMMATRIX& MultiMat)
{
	// 行列を乗算する
	_boneMats[node.boneIdx] *= MultiMat;

	// 再帰する
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

	// ヘッダー情報の読み込み
	fread(&data.signature, sizeof(data.signature), 1, fp);
	fread(&data.version, sizeof(PMDHeader) - sizeof(data.signature) - 1, 1, fp);

	// 頂点情報の読み込み
	unsigned int vnum = 0;
	fread(&vnum, sizeof(vnum), 1, fp);

	_vivec.resize(vnum);

	for (auto& vi : _vivec)
	{
		fread(&vi, sizeof(VertexInfo), 1, fp);
	}

	// インデックス情報の読み込み
	unsigned int inum = 0;
	fread(&inum, sizeof(unsigned int), 1, fp);
	_verindex.resize(inum);

	for (auto& vidx : _verindex)
	{
		fread(&vidx, sizeof(unsigned short), 1, fp);
	}

	// マテリアルの読み込み
	unsigned int mnum = 0;
	fread(&mnum, sizeof(unsigned int), 1, fp);
	_materials.resize(mnum);

	for (auto& mat : _materials)
	{
		fread(&mat, sizeof(PMDMaterial), 1, fp);
	}

	// しろてくすちゃつくる
	auto result = CreateWhiteTexture(_dev);
	// くろてくすちゃつくる
	result = CreateBlackTexture(_dev);
	// グラデーションてくすちゃつくる
	result = CreateGrayGradationTexture(_dev);

	// テクスチャの読み込みとバッファの作成
	_textureBuffer.resize(mnum);
	_sphBuffer.resize(mnum);
	_spaBuffer.resize(mnum);
	_toonResources.resize(mnum);

	for (int i = 0; i < _materials.size(); ++i) {
		// テクスチャファイルパスの取得
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

		// テクスチャバッファを作成して入れる
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

	// ボーンの読み込み
	unsigned int bnum = 0;
	fread(&bnum, sizeof(unsigned short), 1, fp);
	_bones.resize(bnum);
	for (auto& bone : _bones)
	{
		fread(&bone, sizeof(PMDBoneInfo), 1, fp);
	}
	fclose(fp);

	// マテリアルバッファの作成
	CreateMaterialBuffer(_dev);
	// ボーンツリー作成
	CreateBoneTree();
	// ボーンバッファ作成
	CreateBoneBuffer(_dev);
}

std::string PMDmodel::GetExtension(const std::string & path)
{
	// 拡張子の手前のどっとまでの文字列を取得する
	int idx = path.rfind('.');

	// 引数から切り取る
	auto p = path.substr(idx + 1, path.length() - idx - 1);
	return p;
}

std::pair<std::string, std::string>
PMDmodel::SplitFileName(const std::string & path, const char splitter)
{
	// どこで分割するかの取得
	int idx = path.find(splitter);

	// 返り値用変数
	std::pair<std::string, std::string>ret;
	ret.first = path.substr(0, idx);// 前方部分	
	ret.second = path.substr(idx + 1, path.length() - idx - 1);// 後方部分

	return ret;
}

std::string PMDmodel::GetTexPath(const std::string & modelPath, const char * texPath)
{
	// フォルダセパレータが「/」じゃなくて「\\」の可能性もあるので2パターン取得する
	int pathIndex1 = modelPath.rfind('/');
	int pathIndex2 = modelPath.rfind('\\');

	// rfind関数は見つからなかったときに0xffffffffを返すため2つのパスを比較する
	int path = max(pathIndex1, pathIndex2);

	// モデルデータの入っているフォルダを逆探査で探してくる
	std::string folderPath = modelPath.substr(0, path + 1);

	// 合成
	return std::string(folderPath + texPath);
}

ID3D12Resource * PMDmodel::LoadTextureFromFile(std::string & texPath, ID3D12Device* _dev)
{
	//WICテクスチャのロード
	DirectX::TexMetadata metadata = {};
	DirectX::ScratchImage scratchImg = {};

	auto it = _resourceTable.find(texPath);
	if (it != _resourceTable.end()) {
		//テーブルに内にあったらロードするのではなくマップ内の
		//リソースを返す
		return (*it).second;
	}

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
	_resourceTable[texPath] = texbuff;
	return texbuff;
}

std::wstring PMDmodel::GetWstringFromString(const std::string& str)
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

HRESULT PMDmodel::CreateWhiteTexture(ID3D12Device* _dev)
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

HRESULT PMDmodel::CreateBlackTexture(ID3D12Device* _dev)
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

HRESULT PMDmodel::CreateGrayGradationTexture(ID3D12Device* _dev)
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

void PMDmodel::CreateBoneTree()
{
	_boneMats.resize(_bones.size());
	// 単位行列で初期化
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

HRESULT PMDmodel::CreateMaterialBuffer(ID3D12Device* _dev)
{
	D3D12_DESCRIPTOR_HEAP_DESC matDescHeap = {};
	matDescHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	matDescHeap.NodeMask = 0;

	// 定数バッファとシェーダーリソースビューとSphとSpaとトゥーンの５枚↓
	matDescHeap.NumDescriptors = _materials.size() * 5;
	matDescHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	auto result = _dev->CreateDescriptorHeap(&matDescHeap, IID_PPV_ARGS(&_matDescHeap));

	size_t size = (sizeof(PMDMaterial) + 0xff)&~0xff;

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
		sprintf_s(toonFileName, "toon%02d.bmp", _materials[i].toon_index + 1);
		toonFilePath += toonFileName;
		_toonResources[i] = LoadTextureFromFile(toonFilePath,_dev);

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
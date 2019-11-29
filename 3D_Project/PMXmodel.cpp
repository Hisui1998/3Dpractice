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
		fread(&vi, sizeof(vi.pos)+ sizeof(vi.normal)+ sizeof(vi.uv), 1, fp);
		
		// 追加UV読み込み
		for (int i = 0; i < header.data[1]; ++i)
		{
			fread(&vi.adduv[i], sizeof(vi.adduv), 1, fp);
		}

		// weight変形形式よっみこみ
		fread(&vi.weightType, sizeof(vi.weightType), 1, fp);

		// 変形形式別の読み込み
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
		// エッジの読み込み
		fread(&vi.edge, sizeof(float), 1, fp);
	}

	// インデックス読み込み
	int indexNum = 0;
	fread(&indexNum, sizeof(indexNum), 1, fp);
	_verindex.resize(indexNum);
	for (auto& vi: _verindex)
	{
		fread(&vi, header.data[2], 1, fp);
	}

	// テクスチャ読み込み
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

		// 接続先
		if (_bones[idx].bitFlag & 0x0001)
		{
			fread(&_bones[idx].boneIndex, header.data[5], 1, fp);
		}
		else 
		{
			fread(&_bones[idx].offset, sizeof(_bones[idx].offset), 1, fp);
		}

		// 回転付与 と 移動付与
		if ((_bones[idx].bitFlag & 0x0100) || (_bones[idx].bitFlag & 0x0200))
		{
			fread(&_bones[idx].grantIndex, header.data[5], 1, fp);
			fread(&_bones[idx].grantPar, sizeof(_bones[idx].grantPar), 1, fp);
		}

		// 軸固定
		if (_bones[idx].bitFlag & 0x0400)
		{
			fread(&_bones[idx].axisvector, sizeof(_bones[idx].axisvector), 1, fp);
		}

		// ローカル軸
		if (_bones[idx].bitFlag & 0x0800)
		{
			fread(&_bones[idx].axisXvector, sizeof(_bones[idx].axisXvector), 1, fp);
			fread(&_bones[idx].axisZvector, sizeof(_bones[idx].axisZvector), 1, fp);
		}

		// 外部親変形
		if (_bones[idx].bitFlag & 0x2000)
		{
			fread(&_bones[idx].key, sizeof(_bones[idx].key), 1, fp);
		}

		// IKデータ
		if (_bones[idx].bitFlag & 0x0020)
		{
			fread(&_bones[idx].IkData.boneIdx, header.data[5], 1, fp);
			fread(&_bones[idx].IkData.loopCnt, sizeof(_bones[idx].IkData.loopCnt), 1, fp);
			fread(&_bones[idx].IkData.limrad, sizeof(_bones[idx].IkData.limrad), 1, fp);

			// IKリンク
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

	if (vmdPath != "")
	{
		// Vmd読み込み
		_vmdData = std::make_shared<VMDMotion>(vmdPath,60);
		_morphWeight = 0;
	}
	else
	{
		_vmdData = nullptr;
	}


	// 各バッファの初期化
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

	// しろてくすちゃつくる
	result = CreateWhiteTexture();

	// くろてくすちゃつくる
	result = CreateBlackTexture();

	// グラデーションテクスチャつくる
	result = CreateGrayGradationTexture();

	result = CreateMaterialBuffer();

	CreateBoneTree();

	result = CreateBoneBuffer();
}

void PMXmodel::BufferUpDate()
{
	// 頂点バッファの更新
	PMXVertexInfo* vertMap = nullptr;
	auto result = _vertexBuffer->Map(0, nullptr, (void**)&vertMap);
	std::copy(vertexInfo.begin(), vertexInfo.end(), vertMap);
	_vertexBuffer->Unmap(0, nullptr);

	// インデックスバッファのマッピング
	unsigned short* idxMap = nullptr;
	result = _indexBuffer->Map(0, nullptr, (void**)&idxMap);
	std::copy(std::begin(_verindex), std::end(_verindex), idxMap);
	_indexBuffer->Unmap(0, nullptr);
}

void PMXmodel::MotionUpDate(int frameno)
{
	// ボーん
	std::fill(_boneMats.begin(), _boneMats.end(), XMMatrixIdentity());
	for (auto& boneanim : _vmdData->GetMotionData()) {
		auto& boneName = boneanim.first;
		auto& keyframes = boneanim.second;

		// 今使うモーションデータを取得
		auto frameIt = std::find_if(keyframes.rbegin(), keyframes.rend(),
			[frameno](const MotionData& k) {return k.FrameNo <= frameno; });
		if (frameIt == keyframes.rend())continue;

		// イテレータを反転させて次の要素をとってくる
		auto nextIt = frameIt.base();

		if (nextIt == keyframes.end()) {
			// 回転
			RotationMatrix(GetWstringFromString(boneName), frameIt->Quaternion);
			
			// 座標移動
			_boneMats[_boneTree[GetWstringFromString(boneName)].boneIdx]
				*= DirectX::XMMatrixTranslationFromVector(XMLoadFloat3(&frameIt->Location));
			
			_bones[_boneTree[GetWstringFromString(boneName)].boneIdx].pos = frameIt->Location;
		}
		else
		{
			// 現在のﾌﾚｰﾑ位置から重み計算
			float pow = (static_cast<float>(frameno) - frameIt->FrameNo) / (nextIt->FrameNo - frameIt->FrameNo);
			pow = GetBezierPower(pow, frameIt->p1, frameIt->p2, 12);

			// 回転
			RotationMatrix(GetWstringFromString(boneName), frameIt->Quaternion, nextIt->Quaternion, pow);

			// 座標移動
			_boneMats[_boneTree[GetWstringFromString(boneName)].boneIdx]
				*= DirectX::XMMatrixTranslationFromVector(XMVectorLerp(XMLoadFloat3(&frameIt->Location),XMLoadFloat3(&nextIt->Location), pow));
			
			 XMStoreFloat3(&_bones[_boneTree[GetWstringFromString(boneName)].boneIdx].pos,XMVectorLerp(XMLoadFloat3(&frameIt->Location), XMLoadFloat3(&nextIt->Location), pow));
		}
	}

	// ツリーのトラバース
	DirectX::XMMATRIX rootmat = DirectX::XMMatrixIdentity();
	RecursiveMatrixMultiply(_boneTree[L"センター"], rootmat);

	std::copy(_boneMats.begin(), _boneMats.end(), _sendBone);
}

void PMXmodel::MorphUpDate(int frameno)
{
	auto morphData = _vmdData->GetMorphData();

	// 今使うモーションデータを取得
	auto frameIt = std::find_if(morphData.rbegin(), morphData.rend(),
		[frameno](const std::pair<int, std::vector<MorphInfo>>& k) {return k.first <= frameno; });
	if (frameIt == morphData.rend())return;
	// イテレータを反転させて次の要素をとってくる
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
				// 次要素がなかった場合
				if (nextIt == morphData.end()) {
					// 今のモーフ情報をそのまま使う
					XMStoreFloat3(&vertexInfo[nowMorph.vertexMorph.verIdx].pos,
						XMLoadFloat3(&firstVertexInfo[nowMorph.vertexMorph.verIdx].pos)
					);
				}
				else// 次要素があった場合
				{
					// 重み計算
					float pow = (static_cast<float>(frameno) - frameIt->first) / (nextIt->first - frameIt->first);

					// 今の表情情報を格納
					auto addVec = XMLoadFloat3(&nowMorph.vertexMorph.pos)*nowmd.Weight;
					for (auto& nextmd : nextIt->second)
					{
						// 次の表情情報を格納
						auto addVec2 = XMLoadFloat3(&nowMorph.vertexMorph.pos)*nextmd.Weight;

						// 今の表情と次の表情を線形補完して座標に適応する
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

	// ビューポートの設定
	_viewPort.TopLeftX = 0;
	_viewPort.TopLeftY = 0;
	_viewPort.Width = static_cast<float>(Application::Instance().GetWindowSize().width);
	_viewPort.Height = static_cast<float>(Application::Instance().GetWindowSize().height);
	_viewPort.MaxDepth = 1.0f;
	_viewPort.MinDepth = 0.0f;

	// シザーの設定
	_scissor.left = 0;
	_scissor.top = 0;
	_scissor.right = Application::Instance().GetWindowSize().width;
	_scissor.bottom = Application::Instance().GetWindowSize().height;

	// パイプラインのセット
	list->SetPipelineState(_shadowMapGPS);

	// ルートシグネチャをセット
	list->SetGraphicsRootSignature(_shadowMapRS);

	//ビューポートとシザー設定
	list->RSSetViewports(1, &_viewPort);
	list->RSSetScissorRects(1, &_scissor);

	// モデル描画
	// インデックスバッファビューの設定
	list->IASetIndexBuffer(&_idxView);

	// 頂点バッファビューの設定
	list->IASetVertexBuffers(0, 1, &_vbView);

	// トポロジーのセット
	list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	unsigned int offset = 0;

	// でスクリプターヒープのセット(WVP)
	list->SetDescriptorHeaps(1, &wvp);

	// デスクリプタテーブルのセット(WVP)
	D3D12_GPU_DESCRIPTOR_HANDLE wvpStart = wvp->GetGPUDescriptorHandleForHeapStart();
	list->SetGraphicsRootDescriptorTable(0, wvpStart);

	// デスクリプタヒープのセット(ボーン) 
	list->SetDescriptorHeaps(1, &_boneHeap);

	// デスクリプタテーブルのセット(ボーン)
	auto bohandle = _boneHeap->GetGPUDescriptorHandleForHeapStart();
	list->SetGraphicsRootDescriptorTable(1, bohandle);

	for (auto& m : _materials) {
		// 描画部
		list->DrawIndexedInstanced(m.faceVerCnt, 1, offset, 0, 0);

		// 変数の加算
		offset += m.faceVerCnt;
	}
}

HRESULT PMXmodel::CreateShadowRS()
{
	ID3DBlob* signature = nullptr;
	ID3DBlob* error = nullptr;

	// サンプラの設定
	D3D12_STATIC_SAMPLER_DESC SamplerDesc[1] = {};
	SamplerDesc[0].Filter = D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR;// 特別なフィルタを使用しない
	SamplerDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;// 画が繰り返し描画される
	SamplerDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	SamplerDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	SamplerDesc[0].MaxLOD = D3D12_FLOAT32_MAX;// 上限なし
	SamplerDesc[0].MinLOD = 0.0f;// 下限なし
	SamplerDesc[0].MipLODBias = 0.0f;// MIPMAPのバイアス
	SamplerDesc[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;// エッジの色(黒)
	SamplerDesc[0].ShaderRegister = 0;// 使用するレジスタスロット
	SamplerDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;// どのくらいのデータをシェーダに見せるか(全部)
	
	// レンジの設定
	D3D12_DESCRIPTOR_RANGE descRange[2] = {};
	//WVP
	descRange[0].NumDescriptors = 1;
	descRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;//定数
	descRange[0].BaseShaderRegister = 0;//レジスタ番号
	descRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	// ボーン
	descRange[1].NumDescriptors = 1;
	descRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;//定数
	descRange[1].BaseShaderRegister = 1;//レジスタ番号
	descRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ルートパラメータ変数の設定
	D3D12_ROOT_PARAMETER rootParam[2] = {};
	rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[0].DescriptorTable.pDescriptorRanges = &descRange[0];//対応するレンジへのポインタ
	rootParam[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//すべてのシェーダから参照

	// ボーン
	rootParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[1].DescriptorTable.pDescriptorRanges = &descRange[1];
	rootParam[1].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	// ルートシグネチャを作るための変数の設定
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

	// ルートシグネチャ本体の作成
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

	// ピクセルシェーダ
	result = D3DCompileFromFile(L"Shadow.hlsl", nullptr, nullptr, "shadowPS", "ps_5_0", D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_shadowPixShader, nullptr);

	auto InputLayout = GetInputLayout();

	// パイプラインを作るためのGPSの変数の作成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};

	//ルートシグネチャと頂点レイアウト
	gpsDesc.pRootSignature = _shadowMapRS;
	gpsDesc.InputLayout.pInputElementDescs = InputLayout.data();// 配列の開始位置
	gpsDesc.InputLayout.NumElements = static_cast<unsigned int>(InputLayout.size());// 要素の数を入れる

	//シェーダのセット
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(_shadowVertShader);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(_shadowPixShader);

	// レンダーターゲット数の指定(今回はレンダーターゲットがないため０)
	gpsDesc.NumRenderTargets = 0;

	//深度ステンシル
	gpsDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	gpsDesc.DepthStencilState.DepthEnable = true;
	gpsDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;// 必須
	gpsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;// 小さいほうを通す

	gpsDesc.DepthStencilState.StencilEnable = false;

	//ラスタライザ
	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	//その他
	gpsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.NodeMask = 0;
	gpsDesc.SampleDesc.Count = 1;
	gpsDesc.SampleDesc.Quality = 0;
	gpsDesc.SampleMask = 0xffffffff;
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//三角形

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

	result = whiteTex->WriteToSubresource(0, nullptr, data.data(), 4 * 4, static_cast<unsigned int>(data.size()));

	return result;
}

HRESULT PMXmodel::CreateBlackTexture()
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

	result = blackTex->WriteToSubresource(0, nullptr, data.data(), 4 * 4, static_cast<unsigned int>(data.size()));

	return result;
}

HRESULT PMXmodel::CreateVertexBuffer()
{
	// ヒープの情報設定
	D3D12_HEAP_PROPERTIES heapprop = {};
	heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;//CPUからGPUへ転送する用
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	// create
	auto result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,//特別な指定なし
		&CD3DX12_RESOURCE_DESC::Buffer(vertexInfo.size() * sizeof(PMXVertexInfo)),
		D3D12_RESOURCE_STATE_GENERIC_READ,//よみこみ
		nullptr,//nullptrでいい
		IID_PPV_ARGS(&_vertexBuffer));//いつもの

	auto desc = CD3DX12_RESOURCE_DESC::Buffer(vertexInfo.size() * sizeof(PMXVertexInfo));
	// 頂点バッファのマッピング
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
	// ヒープの情報設定
	D3D12_HEAP_PROPERTIES heapprop = {};
	heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;//CPUからGPUへ転送する用
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	auto result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(_verindex.size() * sizeof(_verindex[0])),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_indexBuffer));

	// インデックスバッファのマッピング
	unsigned short* idxMap = nullptr;
	result = _indexBuffer->Map(0, nullptr, (void**)&idxMap);
	std::copy(std::begin(_verindex), std::end(_verindex), idxMap);

	_indexBuffer->Unmap(0, nullptr);

	_idxView.BufferLocation = _indexBuffer->GetGPUVirtualAddress();//バッファの場所
	_idxView.Format = DXGI_FORMAT_R16_UINT;//フォーマット(shortだからR16)
	_idxView.SizeInBytes = static_cast<unsigned int>(_verindex.size()) * sizeof(_verindex[0]);//総サイズ

	return result;
}

HRESULT PMXmodel::CreateMaterialBuffer()
{
	D3D12_DESCRIPTOR_HEAP_DESC matDescHeap = {};
	matDescHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	matDescHeap.NodeMask = 0;

	// 定数バッファとシェーダーリソースビューとSphとSpaとトゥーンの５枚↓
	matDescHeap.NumDescriptors = static_cast<unsigned int>(_materials.size()) * 5;
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
			if (_materials[i].toonIndex < _texturePaths.size())
			{
				toonFilePath = FolderPath;
				sprintf_s(toonFileName, _texturePaths[_materials[i].toonIndex].c_str());
			}
		}

		toonFilePath += toonFileName;
		_toonResources[i] = LoadTextureFromFile(toonFilePath);

		// 定数バッファの作成
		matDesc.BufferLocation = _materialsBuff[i]->GetGPUVirtualAddress();
		matDesc.SizeInBytes = static_cast<unsigned int>(size);
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
	
	// 自分のデータを入れる
	for (int idx = 0; idx < _bones.size(); ++idx) {
		// 自分のボーンインデックスを格納する
		_boneTree[_bones[idx].name].boneIdx = idx;
		if (_bones[idx].boneIndex < _bones.size())
		{
			// 自分のポジションを格納する
			_boneTree[_bones[idx].name].startPos = _bones[idx].pos;
		}
	}		for (auto& b : _boneTree) {
		if (_bones[b.second.boneIdx].parentboneIndex < _bones.size())// 親のボーン番号がボーン総数を超えていなければノード追加
		{
			auto parentName = _bones[_bones[b.second.boneIdx].parentboneIndex].name;// 親の名前を確保

			_boneTree[parentName].children.push_back(&b.second);// 親に自分のノード情報を子として追加
		}
	}
}

// PMXモデル描画用パイプラインステートの生成
HRESULT PMXmodel::CreatePipeline()
{
	// シェーダーの読み込み
	ID3DBlob* vertexShader = nullptr;
	ID3DBlob* pixelShader = nullptr;

	// 頂点シェーダ
	auto result = D3DCompileFromFile(L"PMXShader.hlsl", nullptr, nullptr, "vs", "vs_5_0", D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION, 0, &vertexShader, nullptr);

	// ピクセルシェーダ
	result = D3DCompileFromFile(L"PMXShader.hlsl", nullptr, nullptr, "ps", "ps_5_0", D3DCOMPILE_DEBUG |
		D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pixelShader, nullptr);
	auto inputLayout = GetInputLayout();

	// パイプラインを作るためのGPSの変数の作成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};

	//ルートシグネチャと頂点レイアウト
	gpsDesc.pRootSignature = _pmxSignature;
	gpsDesc.InputLayout.pInputElementDescs = inputLayout.data();// 配列の開始位置
	gpsDesc.InputLayout.NumElements = static_cast<unsigned int>(inputLayout.size());// 要素の数を入れる

	//シェーダのセット
	gpsDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader);
	gpsDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader);

	//レンダーターゲット
	gpsDesc.NumRenderTargets = 1;
	gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	//深度ステンシル
	gpsDesc.DepthStencilState.DepthEnable = true;
	gpsDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;// 必須
	gpsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;// 小さいほうを通す

	gpsDesc.DepthStencilState.StencilEnable = false;
	gpsDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	//ラスタライザ
	gpsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	//レンダーターゲットブレンド設定用構造体
	D3D12_RENDER_TARGET_BLEND_DESC renderBlend = {};
	renderBlend.BlendEnable = true;
	renderBlend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	renderBlend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	renderBlend.BlendOp = D3D12_BLEND_OP_ADD;
	renderBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
	renderBlend.DestBlendAlpha = D3D12_BLEND_ZERO;
	renderBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	renderBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	//ブレンドステート設定用構造体
	D3D12_BLEND_DESC blend = {};
	blend.AlphaToCoverageEnable = false;
	blend.IndependentBlendEnable = false;
	blend.RenderTarget[0] = renderBlend;

	//その他
	gpsDesc.BlendState = blend;
	gpsDesc.NodeMask = 0;
	gpsDesc.SampleDesc.Count = 1;
	gpsDesc.SampleDesc.Quality = 0;
	gpsDesc.SampleMask = 0xffffffff;
	gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//三角形

	result = _dev->CreateGraphicsPipelineState(&gpsDesc, IID_PPV_ARGS(&_pmxPipeline));

	return result;
}

// PMXモデル描画用ルートシグネチャの生成
HRESULT PMXmodel::CreateRootSignature()
{
	ID3DBlob* signature = nullptr;
	ID3DBlob* error = nullptr;

	// サンプラの設定
	D3D12_STATIC_SAMPLER_DESC SamplerDesc[2] = {};
	SamplerDesc[0].Filter = D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT;// 特別なフィルタを使用しない
	SamplerDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;// 画が繰り返し描画される
	SamplerDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	SamplerDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	SamplerDesc[0].MaxLOD = D3D12_FLOAT32_MAX;// 上限なし
	SamplerDesc[0].MinLOD = 0.0f;// 下限なし
	SamplerDesc[0].MipLODBias = 0.0f;// MIPMAPのバイアス
	SamplerDesc[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;// エッジの色(黒)
	SamplerDesc[0].ShaderRegister = 0;// 使用するレジスタスロット
	SamplerDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;// どのくらいのデータをシェーダに見せるか(全部)

	SamplerDesc[1] = SamplerDesc[0];//変更点以外をコピー
	SamplerDesc[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;//繰り返さない
	SamplerDesc[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;//繰り返さない
	SamplerDesc[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;//繰り返さない
	SamplerDesc[1].ShaderRegister = 1; //シェーダスロット番号を忘れないように

	// レンジの設定
	D3D12_DESCRIPTOR_RANGE descRange[5] = {};// テクスチャと定数二つとぼーん
	// WVP
	descRange[0].NumDescriptors = 1;
	descRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;//定数
	descRange[0].BaseShaderRegister = 0;//レジスタ番号
	descRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// マテリアル
	descRange[1].NumDescriptors = 1;
	descRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;//定数
	descRange[1].BaseShaderRegister = 1;//レジスタ番号
	descRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// テクスチャー
	descRange[2].NumDescriptors = 4;// テクスチャとスフィアの二つとトゥーン
	descRange[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//てくすちゃ
	descRange[2].BaseShaderRegister = 0;//レジスタ番号
	descRange[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ボーン
	descRange[3].NumDescriptors = 1;
	descRange[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descRange[3].BaseShaderRegister = 2;
	descRange[3].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// syadou 
	descRange[4].NumDescriptors = 1;
	descRange[4].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descRange[4].BaseShaderRegister = 4;//レジスタ番号
	descRange[4].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ルートパラメータ変数の設定
	D3D12_ROOT_PARAMETER rootParam[4] = {};
	rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[0].DescriptorTable.pDescriptorRanges = &descRange[0];//対応するレンジへのポインタ
	rootParam[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//すべてのシェーダから参照

	// テクスチャ
	rootParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[1].DescriptorTable.pDescriptorRanges = &descRange[1];//対応するレンジへのポインタ
	rootParam[1].DescriptorTable.NumDescriptorRanges = 2;
	rootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//ピクセルシェーダから参照

	// ボーン
	rootParam[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[2].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[2].DescriptorTable.pDescriptorRanges = &descRange[3];
	rootParam[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	// しゃどうｗ
	rootParam[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[3].DescriptorTable.NumDescriptorRanges = 1;
	rootParam[3].DescriptorTable.pDescriptorRanges = &descRange[4];
	rootParam[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// ルートシグネチャを作るための変数の設定
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

	// ルートシグネチャ本体の作成
	result = _dev->CreateRootSignature(
		0,
		signature->GetBufferPointer(),
		signature->GetBufferSize(),
		IID_PPV_ARGS(&_pmxSignature));

	return result;
}

HRESULT PMXmodel::CreateBoneBuffer()
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
	auto start = XMLoadFloat3(&bonenode.startPos);// 元の座標を入れておく
	auto quaternion = XMLoadFloat4(&quat1);
	auto quaternion2 = XMLoadFloat4(&quat2);


	//原点まで並行移動してそこで回転を行い、元の位置まで戻す
	_boneMats[bonenode.boneIdx] =
		DirectX::XMMatrixTranslationFromVector(XMVectorScale(start, -1))*
		XMMatrixRotationQuaternion(XMQuaternionSlerp(quaternion, quaternion2, pow))*
		DirectX::XMMatrixTranslationFromVector(start);
}

void PMXmodel::RotationMatrix(const std::wstring bonename, const XMFLOAT4 &quat1)
{
	auto bonenode = _boneTree[bonename];
	auto start = XMLoadFloat3(&bonenode.startPos);// 元の座標を入れておく
	auto quaternion = XMLoadFloat4(&quat1);
	auto bonetype = _bones[bonenode.boneIdx];


	//原点まで並行移動してそこで回転を行い、元の位置まで戻す
	_boneMats[bonenode.boneIdx] =
		DirectX::XMMatrixTranslationFromVector(XMVectorScale(start, -1))*
		XMMatrixRotationQuaternion(quaternion)*
		DirectX::XMMatrixTranslationFromVector(start);
}

void PMXmodel::RecursiveMatrixMultiply(PMXBoneNode& node, XMMATRIX& MultiMat)
{
	// 行列を乗算する
	_boneMats[node.boneIdx] *= MultiMat;

	// 再帰する
	for (auto& child : node.children) {
		RecursiveMatrixMultiply(*child, _boneMats[node.boneIdx]);
	}
}

float PMXmodel::GetBezierPower(float x, const XMFLOAT2 & a, const XMFLOAT2 & b, uint8_t n)
{
	// aとbが同じなら計算する必要なし
	if (a.x == a.y&&b.x == b.y)
	{
		return x;
	}

	float t = x;

	const float k0 = 1 + 3 * a.x - 3 * b.x;//t^3の係数
	const float k1 = 3 * b.x - 6 * a.x;//t^2の係数
	const float k2 = 3 * a.x;//tの係数

	constexpr float epsilon = 0.0005f;// 計算をやめる誤差

	for (int i = 0; i < n; ++i) {
		
		auto ft = k0 * t*t*t + k1 * t*t + k2 * t - x;

		// 計算結果が誤差の範囲なら計算終了
		if (ft <= epsilon && ft >= -epsilon)
		{
			break;
		}
		t -= ft / 2;// 二で割っていく
	}

	// yを計算する
	auto r = 1 - t;
	return t * t*t + 3 * t*t*r*b.y + 3 * t*r*r*a.y;
}

void PMXmodel::UpDate(char key[256])
{
	if (key[DIK_F])
	{
		for (auto &md: _morphData[L"青ざめ"])
		{
			if (_materials[md.materialMorph.materialIdx].Diffuse.w < 1.f)
			{
				_materials[md.materialMorph.materialIdx].Diffuse.w += 0.1f;
			}
		}
	}
	else
	{
		for (auto &md : _morphData[L"青ざめ"])
		{
			if (_materials[md.materialMorph.materialIdx].Diffuse.w > 0.f)
			{
				_materials[md.materialMorph.materialIdx].Diffuse.w -= 0.1f;
			}
		}
	}

	// マテリアルカラーの転送
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
		// モーションの更新
		static auto lastTime = GetTickCount();
		auto TotalFrame = _vmdData->GetTotalFrame();
		auto NowFrame = static_cast<float>(GetTickCount() - lastTime) / 33.33333f;

		if (TotalFrame < NowFrame)
		{
			lastTime = GetTickCount();
		}

		MotionUpDate(NowFrame);// 動きの更新
		MorphUpDate(NowFrame); // 表情の更新
	}

	// バッファの更新
	BufferUpDate();
}

void PMXmodel::Draw(ID3D12GraphicsCommandList* list, ID3D12DescriptorHeap* wvp, ID3D12DescriptorHeap*shadow)
{
	float clearColor[] = { 0.5f,0.5f,0.5f,0.0f };

	// ビューポートの設定
	_viewPort.TopLeftX = 0;
	_viewPort.TopLeftY = 0;
	_viewPort.Width = static_cast<float>(Application::Instance().GetWindowSize().width);
	_viewPort.Height = static_cast<float>(Application::Instance().GetWindowSize().height);
	_viewPort.MaxDepth = 1.0f;
	_viewPort.MinDepth = 0.0f;

	// シザーの設定
	_scissor.left = 0;
	_scissor.top = 0;
	_scissor.right = Application::Instance().GetWindowSize().width;
	_scissor.bottom = Application::Instance().GetWindowSize().height;

	// パイプラインのセット
	list->SetPipelineState(_pmxPipeline);

	// ルートシグネチャをセット
	list->SetGraphicsRootSignature(_pmxSignature);

	//ビューポートとシザー設定
	list->RSSetViewports(1, &_viewPort);
	list->RSSetScissorRects(1, &_scissor);

	// モデル描画
	// インデックスバッファビューの設定
	list->IASetIndexBuffer(&_idxView);

	// 頂点バッファビューの設定
	list->IASetVertexBuffers(0, 1, &_vbView);

	// トポロジーのセット
	list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	unsigned int offset = 0;
	auto boneheap = _boneHeap;
	auto materialheap = _matDescHeap;

	// でスクリプターヒープのセット(WVP)
	list->SetDescriptorHeaps(1, &wvp);

	// デスクリプタテーブルのセット(WVP)
	D3D12_GPU_DESCRIPTOR_HANDLE wvpStart = wvp->GetGPUDescriptorHandleForHeapStart();
	list->SetGraphicsRootDescriptorTable(0, wvpStart);

	// デスクリプタヒープのセット(ボーン) 
	list->SetDescriptorHeaps(1, &boneheap);

	// デスクリプタテーブルのセット(ボーン)
	auto bohandle = boneheap->GetGPUDescriptorHandleForHeapStart();
	list->SetGraphicsRootDescriptorTable(2, bohandle);

	// でスクリプターのセット(shadow)
	list->SetDescriptorHeaps(1, &shadow);
	auto shadowStart = shadow->GetGPUDescriptorHandleForHeapStart();
	list->SetGraphicsRootDescriptorTable(3, shadowStart);

	// 描画ループ
	// マテリアルのデスクリプタヒープのセット
	list->SetDescriptorHeaps(1, &materialheap);

	// デスクリプターハンドル一枚のサイズ取得
	int incsize = _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// マテリアルヒープの開始位置取得
	auto mathandle = materialheap->GetGPUDescriptorHandleForHeapStart();

	for (auto& m : _materials) {
		// デスクリプタテーブルのセット
		list->SetGraphicsRootDescriptorTable(1, mathandle);

		// 描画部
		list->DrawIndexedInstanced(m.faceVerCnt, 1, offset, 0, 0);

		// ポインタの加算
		mathandle.ptr += incsize * 5;// 5枚あるから5倍

		// 変数の加算
		offset += m.faceVerCnt;
	}
}

const std::vector<D3D12_INPUT_ELEMENT_DESC> PMXmodel::GetInputLayout()
{
	// 頂点レイアウト (構造体と順番を合わせること)
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayoutDescs = {
		// 座標
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0, D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		// 法線
		{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		// UV
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		// 追加UV
		{"ADDUV",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"ADDUV",1,DXGI_FORMAT_R32G32B32A32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"ADDUV",2,DXGI_FORMAT_R32G32B32A32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"ADDUV",3,DXGI_FORMAT_R32G32B32A32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		// wighttype
		{"WEIGHTTYPE",0,DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT ,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		// ボーンインデックス
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
	return _bones[_boneTree[L"センター"].boneIdx].pos;
}

const LPCWSTR PMXmodel::GetUseShader()
{
	return L"PMXShader.hlsl";
}

ID3D12Resource * PMXmodel::LoadTextureFromFile(std::string & texPath)
{
	// 読み込もうとしたテクスチャがすでに読み込んだことのあるテクスチャなら読み込まない
	auto tex = _texMap.find(texPath);
	if (tex != _texMap.end())
	{
		return (*tex).second;
	}

	//WICテクスチャのロード
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};

	// テクスチャファイルのロード
	auto result = LoadFromWICFile(GetWstringFromString(texPath).c_str(),
		WIC_FLAGS_NONE,
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
		static_cast<unsigned int>(metadata.width),
		static_cast<unsigned int>(metadata.height),
		static_cast<UINT16>(metadata.arraySize),
		static_cast<UINT16>(metadata.mipLevels)
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
		static_cast<unsigned int>(img->rowPitch),
		static_cast<unsigned int>(img->slicePitch)
	);
	// テクスチャマップに追加
	_texMap[texPath] = texbuff;

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
	wstr.resize(num1-1);// そのままだと一番最後に空白文字が入ってしまうためマイナス１する

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

HRESULT PMXmodel::CreateGrayGradationTexture()
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

#include "Polygon.h"
#include <d3dx12.h>
#include "Application.h"

using namespace DirectX;

HRESULT Polygon::CreateBuffer()
{
	Vertex vertices[] = {
		XMFLOAT3(-1,-1,0),XMFLOAT2(0,1),//左下
		XMFLOAT3(-1,1,0),XMFLOAT2(0,0),//左上
		XMFLOAT3(1,-1,0),XMFLOAT2(1,1),//右下
		XMFLOAT3(1,1,0),XMFLOAT2(1,0),//右上
	};

	//ペラバッファ生成
	auto result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(vertices)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_peraVertBuff)
	);	Vertex* mapver = nullptr;	result = _peraVertBuff->Map(0, nullptr, (void**)&mapver);
	std::copy(std::begin(vertices), std::end(vertices), mapver);	_peraVertBuff->Unmap(0, nullptr);	_peravbView.BufferLocation = _peraVertBuff->GetGPUVirtualAddress();
	_peravbView.SizeInBytes = sizeof(vertices);
	_peravbView.StrideInBytes = sizeof(Vertex);

	return result;
}

HRESULT Polygon::CreateHeapAndView()
{
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = 2;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	// レンダーターゲットデスクリプタヒープの作成
	auto result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&_rtvDescHeap));

	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;// タイプ変更

	// シェーダーリソースデスクリプタヒープの作成
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&_srvDescHeap));

	// リソースの作成
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


	D3D12_CLEAR_VALUE clear = { DXGI_FORMAT_R8G8B8A8_UNORM ,{0.5f,0.5f,0.5f,1}, };

	result = _dev->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAGS::D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		&clear,
		IID_PPV_ARGS(&_peraBuffer));


	// レンダーターゲットビューの作成
	_dev->CreateRenderTargetView(_peraBuffer, nullptr, _rtvDescHeap->GetCPUDescriptorHandleForHeapStart());

	// シェーダーリソースビューの作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = desc.Format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	_dev->CreateShaderResourceView(_peraBuffer, &srvDesc, _srvDescHeap->GetCPUDescriptorHandleForHeapStart());
	return result;
}

Polygon::Polygon(ID3D12Device*dev):_dev(dev)
{
}


Polygon::~Polygon()
{
}

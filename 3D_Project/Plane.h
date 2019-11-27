#pragma once
#include <d3d12.h>
#include <DirectXMath.h>

struct PlaneVertex
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT2 normal;
	DirectX::XMFLOAT2 uv;
};

class Plane
{
private:
	ID3D12Device* _dev;

	// �p�C�v���C���̐���
	bool CreatePipeline();

	// ���[�g�V�O�l�`���̐���
	bool CreateRootSignature();

	// ���_�o�b�t�@�̍쐬
	bool CreateVertexBuffer();

	// ���_�o�b�t�@�̍쐬
	bool CreateIndexBuffer();


	ID3D12PipelineState* _planeGPS;
	ID3D12RootSignature* _planeRS;

	ID3DBlob* _planeVertexShader = nullptr;// ���_�V�F�[�_
	ID3DBlob* _planePixelShader = nullptr;// �s�N�Z���V�F�[�_

	D3D12_VIEWPORT _viewPort;// �r���[�|�[�g
	D3D12_RECT _scissor;// �V�U�[�͈�

	// ���_�o�b�t�@
	ID3D12Resource* _vertexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW _vbView = {};// ���_�o�b�t�@�r���[

	// �C���f�b�N�X
	ID3D12Resource* _indexBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW _idView = {};// �C���f�b�N�X�o�b�t�@�r���[
public:
	Plane(ID3D12Device* _dev, D3D12_VIEWPORT _view, D3D12_RECT _scissor);
	~Plane();

	void Draw(ID3D12GraphicsCommandList* list, ID3D12DescriptorHeap* wvp, ID3D12DescriptorHeap*shadow);
};


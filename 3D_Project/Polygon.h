#pragma once
#include <d3d12.h>
#include <DirectXMath.h>

// ���_���
struct Vertex
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT2 uv;
};

class Polygon
{
private:
	ID3D12Device* _dev;

	// �o�b�t�@�̍쐬
	HRESULT CreateBuffer();
	// �q�[�v�ƃr���[�̍쐬
	HRESULT CreateHeapAndView();
	// �y���|���p
	ID3D12Resource* _peraBuffer = nullptr;// �y���|���{�̂̃o�b�t�@

	ID3D12Resource* _peraVertBuff = nullptr;// �y���|���p���_�o�b�t�@
	D3D12_VERTEX_BUFFER_VIEW _peravbView = {};// �y���|���p���_�o�b�t�@�r���[

	ID3D12DescriptorHeap* _rtvDescHeap = nullptr;// �y���|���p�����_�[�^�[�Q�b�g�f�X�N���v�^�q�[�v
	ID3D12DescriptorHeap* _srvDescHeap = nullptr;// �y���|���p�V�F�[�_�[���\�[�X�f�X�N���v�^�q�[�v

	ID3DBlob* peraVertShader = nullptr;// �y���|���p���_�V�F�[�_
	ID3DBlob* peraPixShader = nullptr;// �y���|���p�s�N�Z���V�F�[�_

	ID3D12PipelineState* _peraPipeline = nullptr;// �y���|���p�p�C�v���C��
	ID3D12RootSignature* _peraSignature = nullptr;// �y���|���p���[�g�V�O�l�`��
public:
	Polygon(ID3D12Device*dev);
	~Polygon();

};


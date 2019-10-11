#include "Camera.h"



Camera::Camera()
{
}


Camera::~Camera()
{
}

bool Camera::Init()
{
	auto eye = DirectX::XMFLOAT3(0,0,-30);
	auto target = DirectX::XMFLOAT3(0, 0, 0);
	auto up = DirectX::XMFLOAT3(0, 1, 0);
	return true;
}

#pragma once
#include <DirectXTex.h>
class Camera
{
private:
	DirectX::XMMATRIX _wvp;
public:
	Camera();
	~Camera();
	bool Init();
};


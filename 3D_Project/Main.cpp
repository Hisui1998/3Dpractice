#pragma once
#include <iostream>
#include "Application.h"

int main()
//int WINAPI WinMain(HINSTANCE,HINSTANCE,LPSTR,int)// ���g�̃n���h���A�Ăяo�����̃n���h���A�����̃|�C���^�A�����̐�
{
	auto& app = Application::Instance();

	if (int i = app.Init())
	{
		std::cout << i <<"�Ԗڂ̏������Ɏ��s" << std::endl;
		getchar();
		return -1;
	}
	else std::cout << "����������" << std::endl;

	app.Run();

	app.End();
	return 0;
}
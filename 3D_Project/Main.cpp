#pragma once
#include "Application.h"

#ifdef _DEBUG
#include <iostream>
int main()
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#endif
{
	auto& app = Application::Instance();

	if (int i = app.Init())
#ifdef _DEBUG

	{
		std::cout << i << "�Ԗڂ̏������Ɏ��s" << std::endl;
		getchar();
		return -1;
	}
	else std::cout << "����������" << std::endl;

	std::cout << "UP,DOWN:�J�����̏㉺�ړ�" << std::endl;
	std::cout << "Q,LEFT,E,RIGHT:�J������Y����]" << std::endl;
	std::cout << "WASD:�J�����̕��ʈړ�" << std::endl;
#else
	{};
#endif

	app.Run();

	app.End();
	return 0;
}
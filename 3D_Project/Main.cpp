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
		std::cout << i << "”Ô–Ú‚Ì‰Šú‰»‚ÉŽ¸”s" << std::endl;
		getchar();
		return -1;
	}
	else std::cout << "‰Šú‰»Š®—¹" << std::endl;

	std::cout << "UP,DOWN:ƒJƒƒ‰‚Ìã‰ºˆÚ“®" << std::endl;
	std::cout << "Q,LEFT,E,RIGHT:ƒJƒƒ‰‚ÌYŽ²‰ñ“]" << std::endl;
	std::cout << "WASD:ƒJƒƒ‰‚Ì•½–ÊˆÚ“®" << std::endl;
#else
	{};
#endif

	app.Run();

	app.End();
	return 0;
}
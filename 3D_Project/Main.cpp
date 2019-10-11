#pragma once
#include <iostream>
#include "Application.h"

int main()
//int WINAPI WinMain(HINSTANCE,HINSTANCE,LPSTR,int)// 自身のハンドル、呼び出し側のハンドル、引数のポインタ、引数の数
{
	auto& app = Application::Instance();

	if (int i = app.Init())
	{
		std::cout << i <<"番目の初期化に失敗" << std::endl;
		getchar();
		return -1;
	}
	else std::cout << "初期化完了" << std::endl;

	app.Run();

	app.End();
	return 0;
}
#include "Application.h"
#include "Dx12Wrapper.h"
#include <vector>

constexpr int WINDOW_WIDTH = 1366;
constexpr int WINDOW_HEIGHT = 768;

LRESULT WindowProcedure(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

Application::Application()
{
}

Application::~Application()
{
}

Application & Application::Instance() 
{
	static Application _instance;
	return _instance;
}

Size Application::GetWindowSize()
{
	return Size(WINDOW_WIDTH,WINDOW_HEIGHT);
}

int Application::Init()
{
	_wndClass.hInstance = GetModuleHandle(nullptr);// ハンドルの取得
	_wndClass.cbSize = sizeof(WNDCLASSEX);// コールバックするためのやつ
	_wndClass.lpfnWndProc = (WNDPROC)WindowProcedure;
	_wndClass.lpszClassName = ("dx12test");// アプリケーションクラス名
	RegisterClassEx(&_wndClass);// ここでOSが個別認識できるように身分証明書を作る

	RECT wrc = { 0,0,WINDOW_WIDTH,WINDOW_HEIGHT };// ウィンドウサイズの指定
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);// ウィンドウのサイズ補正 タイトルバーの大きさとかを考慮してリサイズしてくれる

	_hwnd = CreateWindow(
		_wndClass.lpszClassName,
		("DX12の練習"),
		WS_OVERLAPPEDWINDOW,// タイトルバーと境界線があるウィンドウ
		CW_USEDEFAULT,// OS依存
		CW_USEDEFAULT,// OS依存
		wrc.right - wrc.left,// 横幅
		wrc.bottom - wrc.top,// 縦幅
		nullptr,
		nullptr,
		_wndClass.hInstance,
		nullptr
	);
	if (_hwnd == 0)return -1;

	_dx12 = std::make_shared<Dx12Wrapper>(_hwnd);
	if (int i = _dx12->Init())return i;
	return 0;
}

void Application::Run()
{
	ShowWindow(_hwnd, SW_SHOW);//ウィンドウ表示
	MSG msg;
	while (true)// メインループ
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);// 翻訳
			DispatchMessage(&msg);
		}
		if (msg.message == WM_QUIT) {// 終了
			break;
		}

		_dx12->UpDate();// 描画
	}
}

void Application::End()
{
	UnregisterClass(_wndClass.lpszClassName, _wndClass.hInstance);
}

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
	_wndClass.hInstance = GetModuleHandle(nullptr);// �n���h���̎擾
	_wndClass.cbSize = sizeof(WNDCLASSEX);// �R�[���o�b�N���邽�߂̂��
	_wndClass.lpfnWndProc = (WNDPROC)WindowProcedure;
	_wndClass.lpszClassName = ("dx12test");// �A�v���P�[�V�����N���X��
	RegisterClassEx(&_wndClass);// ������OS���ʔF���ł���悤�ɐg���ؖ��������

	RECT wrc = { 0,0,WINDOW_WIDTH,WINDOW_HEIGHT };// �E�B���h�E�T�C�Y�̎w��
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);// �E�B���h�E�̃T�C�Y�␳ �^�C�g���o�[�̑傫���Ƃ����l�����ă��T�C�Y���Ă����

	_hwnd = CreateWindow(
		_wndClass.lpszClassName,
		("DX12�̗��K"),
		WS_OVERLAPPEDWINDOW,// �^�C�g���o�[�Ƌ��E��������E�B���h�E
		CW_USEDEFAULT,// OS�ˑ�
		CW_USEDEFAULT,// OS�ˑ�
		wrc.right - wrc.left,// ����
		wrc.bottom - wrc.top,// �c��
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
	ShowWindow(_hwnd, SW_SHOW);//�E�B���h�E�\��
	MSG msg;
	while (true)// ���C�����[�v
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);// �|��
			DispatchMessage(&msg);
		}
		if (msg.message == WM_QUIT) {// �I��
			break;
		}

		_dx12->UpDate();// �`��
	}
}

void Application::End()
{
	UnregisterClass(_wndClass.lpszClassName, _wndClass.hInstance);
}

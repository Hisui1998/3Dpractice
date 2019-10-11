#pragma once
#include <windows.h>
#include <memory>

struct Size
{
	int width;
	int height;
	Size() :width(0), height(0) {};
	Size(int x, int y) :width(x), height(y) {};
};
class Dx12Wrapper;
class Application
{
private:
	HWND _hwnd;// �E�B���h�E����̂��߂̃n���h��
	WNDCLASSEX _wndClass;

	//�����֎~ 
	Application();
	//�R�s�[�A����֎~ 
	Application(const Application&) = delete;
	void operator=(const Application&) = delete;
	struct AppDeleter
	{
		void operator()(Application * game) const
		{
			delete game;
		}
	};
	static std::unique_ptr<Application, AppDeleter> _instance;
	std::shared_ptr<Dx12Wrapper> _dx12;
public:
	~Application();
	static Application& Instance();
	Size GetWindowSize();

	int Init();// ������
	void Run();// ���s
	void End();// �㏈��
};


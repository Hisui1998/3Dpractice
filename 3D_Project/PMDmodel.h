#pragma once
#include <string>

class PMDmodel
{
public:
	PMDmodel();
	~PMDmodel();
	// ���f���f�[�^�̓ǂݍ��݂��s���֐�
	void LoadPMD(std::string modelName);
};


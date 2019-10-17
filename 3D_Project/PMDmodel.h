#pragma once
#include <string>

class PMDmodel
{
public:
	PMDmodel();
	~PMDmodel();
	// モデルデータの読み込みを行う関数
	void LoadPMD(std::string modelName);
};


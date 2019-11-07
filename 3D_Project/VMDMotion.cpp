#include "VMDMotion.h"



void VMDMotion::LoadVMD(std::string fileName)
{
	FILE*fp;
	std::string FilePath = fileName;
	fopen_s(&fp, FilePath.c_str(), "rb");

	// ÉwÉbÉ_Å[èÓïÒÇÃì«Ç›çûÇ›
	fread(&header, sizeof(VMDHeader), 1, fp);

	fread(&MotionCount, sizeof(MotionCount), 1, fp);
	_motionInfo.resize(MotionCount);
	for (auto &mi:_motionInfo)
	{
		fread(&mi, sizeof(MotionInfo), 1, fp);
	}
	

	fclose(fp);
}

VMDMotion::VMDMotion(std::string fileName)
{
	LoadVMD(fileName);
}


VMDMotion::~VMDMotion()
{
}

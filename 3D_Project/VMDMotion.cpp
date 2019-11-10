#include "VMDMotion.h"
#include <algorithm>



void VMDMotion::LoadVMD(std::string fileName)
{
	FILE*fp;
	std::string FilePath = fileName;

	fopen_s(&fp, FilePath.c_str(), "rb");

	VMDHeader header;
	fread(&header,sizeof(header),1,fp);

	fread(&MotionCount,sizeof(MotionCount),1,fp);

	for (int i = 0;i< MotionCount;++i)
	{
		MotionInfo mi;
		fread(&mi, sizeof(mi), 1, fp);
		_motionData[mi.BoneName].emplace_back(mi);
	}

	fread(&MorphCount, sizeof(MorphCount), 1, fp);
	for (int i = 0; i < MorphCount; ++i)
	{
		MorphInfo mi;
		fread(&mi, sizeof(mi), 1, fp);
		_morphData[mi.SkinName].emplace_back(mi);
	}

	fclose(fp);
	for (auto &ad:_motionData)
	{
		std::sort(ad.second.begin(), ad.second.end(), [](MotionInfo& a, MotionInfo& b) {return a.FrameNo < b.FrameNo; });
	}

}

VMDMotion::VMDMotion(std::string fileName):flame(0)
{
	LoadVMD(fileName);
}


VMDMotion::~VMDMotion()
{
}

std::map<std::string, std::vector<MotionInfo>> VMDMotion::GetMotionData()
{
	return _motionData;
}

std::map<std::string, std::vector<MorphInfo>> VMDMotion::GetMorphData()
{
	return _morphData;
}

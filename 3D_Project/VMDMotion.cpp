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
		_animData[mi.BoneName].emplace_back(mi);
	}

	fclose(fp);
	for (auto &ad:_animData)
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

std::map<std::string, std::vector<MotionInfo>> VMDMotion::GetAnimData()
{
	return _animData;
}

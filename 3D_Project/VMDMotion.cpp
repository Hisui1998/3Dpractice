#include "VMDMotion.h"
#include <algorithm>

using namespace DirectX;

int VMDMotion::flame = 0;

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
		MotionInfo mi = {};
		fread(&mi, sizeof(mi), 1, fp);

		MotionData md = {};
		md.BoneName = mi.BoneName;
		md.FrameNo = mi.FrameNo;
		md.Location = mi.Location;
		md.Quaternion = mi.Quaternion;
		
		md.p1 = XMFLOAT2((float)mi.Interpolation[3] / 127.0f, (float)mi.Interpolation[7] / 127.0f);
		md.p2 = XMFLOAT2((float)mi.Interpolation[11] / 127.0f, (float)mi.Interpolation[15] / 127.0f);

		_motionData[md.BoneName].emplace_back(md);
	}

	fread(&MorphCount, sizeof(MorphCount), 1, fp);
	for (int i = 0; i < MorphCount; ++i)
	{
		MorphInfo mi;
		fread(&mi, sizeof(mi), 1, fp);

		_morphData[mi.FrameNo].emplace_back(mi);
	}

	fread(&CameraCount, sizeof(CameraCount), 1, fp);
	

	fclose(fp);
	for (auto &ad : _motionData)
	{
		std::sort(ad.second.begin(), ad.second.end(), [](MotionData& a, MotionData& b) {return a.FrameNo < b.FrameNo; });
	}
	for (auto &ad : _motionData){
		for (auto d:ad.second)
		{
			if (d.FrameNo > totalframe)
			{
				totalframe = d.FrameNo;
			}
		}
	}
	totalframe += _waitFlame;
}

VMDMotion::VMDMotion(std::string fileName,int waitFlame):_waitFlame(waitFlame)
{
	flame = 0;
	totalframe = 0;
	LoadVMD(fileName);
}


VMDMotion::~VMDMotion()
{
}

std::map<std::string, std::vector<MotionData>> VMDMotion::GetMotionData()
{
	return _motionData;
}

std::map<int, std::vector<MorphInfo>> VMDMotion::GetMorphData()
{
	return _morphData;
}

int VMDMotion::Duration()
{
	return ++flame;
}

const int VMDMotion::GetTotalFrame()
{
	return totalframe;
}

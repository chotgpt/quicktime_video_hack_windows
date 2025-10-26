#include "MyClock.h"

MyClock::MyClock(UINT64 qwID)
{
	this->SetID(qwID);
}

void MyClock::SetID(UINT64 _qwID)
{
	this->qwID = _qwID;
	this->qwStartTime = GetTickCount64();
	this->qwTimeScale = 1000;	//ºÁÃë
	//this->qwStartTime = time(NULL);
	//this->qwTimeScale = 1;	//Ãë
}

UINT64 MyClock::GetTime()
{
	return GetTickCount64() - this->qwStartTime;
	//return time(NULL) - this->qwStartTime;
}

UINT64 MyClock::CMTimeToMilliSecond(const CMTime& cmtime)
{
	return cmtime.CMTimeValue * 1000 / cmtime.CMTimeScale;
}

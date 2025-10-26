#pragma once
#include <windows.h>
#include <time.h>

 struct CMTime
{
	UINT64 CMTimeValue = 0;  /*! @field value The value of the CMTime. value/timescale = seconds. */
	UINT32 CMTimeScale = 0;  /*! @field timescale The timescale of the CMTime. value/timescale = seconds.  */
	UINT32 CMTimeFlags = 0;  /*! @field flags The flags, eg. kCMTimeFlags_Valid, kCMTimeFlags_PositiveInfinity, etc. */
	UINT64 CMTimeEpoch = 0;  /*! @field epoch Differentiates between equal timestamps that are actually different because */
};


class MyClock
{
public:
	MyClock(UINT64 qwID);
	void SetID(UINT64 qwID);
	UINT64 GetTime();
public:
	UINT64 qwID;
	UINT64 qwStartTime;
	UINT64 qwTimeScale;
public:
	static UINT64 CMTimeToMilliSecond(const CMTime& cmtime);
};


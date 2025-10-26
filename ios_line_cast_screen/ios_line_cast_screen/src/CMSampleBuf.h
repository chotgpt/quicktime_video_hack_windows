#pragma once
#include <windows.h>
#include <iostream>
#include <vector>
#include "MyClock.h"


#define FormatDescriptorMagic            0x66647363		//fdsc - csdf
#define MediaTypeVideo                   0x76696465		//vide - ediv
#define MediaTypeSound                   0x736F756E		//nuos - soun
#define MediaTypeMagic                   0x6D646961		//mdia - aidm
#define VideoDimensionMagic              0x7664696D		//vdim - midv
#define CodecMagic                       0x636F6463		//codc - cdoc
#define CodecAvc1                        0x61766331		//avc1 - 1cva
#define ExtensionMagic                   0x6578746E		//extn - ntxe
#define AudioStreamBasicDescriptionMagic 0x61736264		//asdb - dbsa



class CMSampleTimingInfo
{
public:
	CMTime Duration;
	CMTime PresentationTimeStamp;
	CMTime DecodeTimeStamp;
};



class AudioStreamDescription
{
public:
	double float64SampleRate = 0.0;
	UINT32 uint32FormatID = 0;
	UINT32 uint32FormatFlags = 0;
	UINT32 uint32BytesPerPacket = 0;
	UINT32 uint32FramesPerPacket = 0;
	UINT32 uint32BytesPerFrame = 0;
	UINT32 uint32ChannelsPerFrame = 0;
	UINT32 uint32BitsPerChannel = 0;
	UINT32 uint32Reserved = 0;
};

class FormatDescriptor
{
public:
	UINT32  MediaType = 0;;
	UINT32	VideoDimensionWidth = 0;;
	UINT32	VideoDimensionHeight = 0;;
	UINT32	Codec = 0;;
	std::vector<unsigned char> Extensions;		//TODO : IndexKeyDict Extensions;
	std::vector<unsigned char> PPS;
	std::vector<unsigned char> SPS;
	std::vector<unsigned char> SPS_and_PPS;
	AudioStreamDescription AudioStreamBasicDescription;
};

class CMSampleBuffer
{
public:
	CMSampleBuffer(const std::vector<unsigned char>& vecCMSampleBuffer, UINT32 MediaType, bool* pbError);
	void parseStia(const unsigned char* ptr);
	void parseSampleSizeArray(const unsigned char* ptr);

	bool extractPPS();
	void parseFormatDescriptor(const unsigned char* ptr);
public:
	CMTime OutputPresentationTimestamp;																				//ok
	bool HasFormatDescription;								//是否有额外数据										//ok
	FormatDescriptor FormatDescription;						//额外数据(音频采样数据 or 视频宽高、PPS、SPS)			//ok
	int NumSamples = 0;										//nsmp													//ok
	std::vector<CMSampleTimingInfo> SampleTimingInfoArray;	//stia													//ok
	std::vector<unsigned char> SampleData;		//采样数据															//ok
	std::vector<UINT32> SampleSizes;			//																	//ok
	std::vector<unsigned char> Attachments;		//TODO : IndexKeyDict Attachments;	//satt							//ok
	std::vector<unsigned char> Sary;			//TODO : IndexKeyDict Sary;			//sary							//ok
	UINT32 MediaType = 0;						//音频(MediaTypeSound) or 视频(MediaTypeVideo)						//ok
};


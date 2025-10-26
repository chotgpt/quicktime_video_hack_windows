#pragma once
#include "lusb0_usb.h"
#include "MyClock.h"

#include <windows.h>
#include <iostream>
#include <vector>
#include <map>

#define VID_APPLE (0x05AC)
#define READ_QT_BUFFER_SIZE ((1024) * (1024) * (5))
#define USB_SUCCESS (0)



const UINT64 qwEmptyCFType = 1;

typedef enum
{
	EM_RUN_STATE_STOP,			//停止
	EM_RUN_STATE_RUNNING,		//运行
	EM_RUN_STATE_READY_STOP,	//通知停止
}thread_running_state_t;

typedef struct
{
	//void(*device_insert)(const char* serial_number);
	//void(*device_remove)(const char* serial_number);

	//音频
	void(*audio_init)(void* user_data, int bits, int channels, int samplerate, int isaudio);
	void(*audio_process)(void* user_data, unsigned char* buffer, int length);

	//视频
	void(*video_connect)(void* user_data);
	void(*video_dis_connect)(void* user_data);
	void(*video_h264_extradata)(void* user_data, unsigned char* buffer, unsigned int length);
	void(*video_h264_slice)(void* user_data, unsigned char* buffer, unsigned int length);

	//交互
	void(*message)(void* user_data, const char* msg);
	//日志
	void(*log)(void* user_data, const char* text);
}quick_time_callbacks_t;

typedef enum
{
	QT_E_SUCCESS = 0,
	QT_E_PARAM_FAIL,			//参数错误
	QT_E_LIBUSB_INIT_FAIL,		//libusb初始化失败
	QT_E_LIBUSB_OPEN_FAIL,		//libusb_open失败
	QT_E_CREATE_THREAD_FAIL,	//创建线程失败
	QT_E_ENABLE_QT_FAIL,		//开启投屏模式失败


	QT_E_UNKNOWN_ERROR = -255
}quick_time_error_t;

typedef enum 
{
	PING = 0x70696E67,
	SyncPacketMagic = 0x73796E63,
	ReplyPacketMagic = 0x72706C79,
	TIME = 0x74696D65,
	CWPA = 0x63777061,
	AFMT = 0x61666D74,
	CVRP = 0x63767270,
	CLOK = 0x636C6F6B,
	OG = 0x676F2120,
	SKEW = 0x736B6577,
	STOP = 0x73746F70,
	CEPM = 0x6365706D,
}qt_packet_type_t;

typedef enum
{
	AsynPacketMagic = 0x6173796E,
	FEED = 0x66656564, //These contain CMSampleBufs which contain raw h264 Nalus
	TJMP = 0x746A6D70,
	SRAT = 0x73726174, //CMTimebaseSetRateAndAnchorTime https://developer.apple.com/documentation/coremedia/cmtimebase?language=objc
	SPRP = 0x73707270, // Set Property
	TBAS = 0x74626173, //TimeBase https://developer.apple.com/library/archive/qa/qa1643/_index.html
	RELS = 0x72656C73,
	HPD1 = 0x68706431, //hpd1 - 1dph | For specifying/requesting the video format
	HPA1 = 0x68706131, //hpa1 - 1aph | For specifying/requesting the audio format
	NEED = 0x6E656564, //need - deen
	EAT = 0x65617421,  //contains audio sbufs
	HPD0 = 0x68706430,
	HPA0 = 0x68706130,
}qt_async_packet_type_t;

struct end_point_data
{
	int nConfigIndex = -1;                     //config index
	int nInterfaceIndex = -1;                  //interface index
	unsigned char btInEndPointAddress = 0x00;  //in  ep address ( >0x80 )
	int nInEndPointPacketSize = -1;            //in  ep packet size
	unsigned char btOutEndPointAddress = 0x00; //out ep address ( <0x80 )
	int nOutEndPointPacketSize = -1;           //out ep packet size
};

class QuickTime
{
public:
	QuickTime();
	~QuickTime();
	//全局初始化(回调函数)
	static bool Init(const quick_time_callbacks_t* pCallbacks);
	static bool isInit();
public:
	void SetUserData(void* user_input_data);
	quick_time_error_t Start(const char* serial_number);
	void Stop();
	thread_running_state_t GetRunningState() const;
private:
	quick_time_error_t SyncExec();
private:
	//响应Sync数据
	void SendHPD1();                            //通知设备发送视频流
	void SendHPA1(UINT64 qwDeviceClockRef);     //通知设备发送音频流
	void ReceiveSyncPacket_CWPA(const std::vector<unsigned char>& vecData);
	void ReceiveSyncPacket_AFMT(const std::vector<unsigned char>& vecData);
	void ReceiveSyncPacket_CVRP(const std::vector<unsigned char>& vecData);
	void ReceiveSyncPacket_CLOK(const std::vector<unsigned char>& vecData);
	void ReceiveSyncPacket_TIME(const std::vector<unsigned char>& vecData);
	void ReceiveSyncPacket_OG(const std::vector<unsigned char>& vecData);
	void ReceiveSyncPacket_SKEW(const std::vector<unsigned char>& vecData);
	void ReceiveSyncPacket_STOP(const std::vector<unsigned char>& vecData);
	void ReceiveSyncPacket(const std::vector<unsigned char>& vecData);
	//响应Asyn数据
	void ReceiveAsynPacket_CMSampleBuffer(const std::vector<unsigned char>& vecData, UINT32 mediaType, bool* pbError);
	void ReceiveAsynPacket(const std::vector<unsigned char>& vecData, bool* pbError);


	void SendPing(int timeout = -1);
	int SendQTBulk(std::vector<unsigned char> vecData, int timeout = -1);
	int ReadQTBulk(std::vector<unsigned char>& vecData, int timeout = -1);
	bool ReceiveLoop();
	bool DevicePair();
	bool DevicePairAndWait(DWORD dwTimeOut);
	void ClearQTEndPointHalt();
	void ClearFeature();
	bool SetMuxConfigAndInterface();
	bool SetQTConfigAndInterface();
	bool EnableQTConfig();
	bool DisableQTConfig();
	bool GetInOutEndPoint(const struct usb_interface_descriptor* interface_desc, struct end_point_data* pEpData);
	int FindInterfaceForSubclass(const struct usb_config_descriptor* config_desc, int nSubClass, struct end_point_data* pEpData);
	bool FindConfigurations(const struct usb_device* libusb_dev, struct end_point_data* pMuxEpData, struct end_point_data* pQtEpData);
	bool IsValidIosDeviceWithActiveQTConfig();
	quick_time_error_t OpenDeviceBySerial(const std::string& strSerialNumber);
	void Close();
	void Clear();

	bool CheckStatus();
private:
	thread_running_state_t running_status;	//脚本运行状态
	void* user_input_data;
	char serial_number[64];

	struct usb_device* libusb_dev;
	struct usb_dev_handle* libusb_dev_handle;
	struct end_point_data end_point_mux;
	struct end_point_data end_point_qt;
private:
	UINT64 qwNeedClockRef = 0;
	std::vector<unsigned char> vecNeedMessage;  //保存的need回复数据
	//音频时钟同步相关
	bool bFirstAudioTimeTaken = false;
	UINT64 StartTimeDeviceAudioMillSecond = 0;             //毫秒
	UINT64 StartTimeLocalAudioMillSecond = 0;              //毫秒
	UINT64 LastEatFrameReceivedDeviceAudioMillSecond = 0;  //毫秒
	UINT64 LastEatFrameReceivedLocalAudioMillSecond = 0;   //毫秒

	MyClock clok_Clock;             //CLOK时钟
	MyClock localAudioClock;        //本地音频时钟(CWPA创建)
	UINT64  deviceAudioClockRef = 0;//设备音频时钟ID
private:
	bool bFirstProcessAudio = true;
	bool bFirstProcessVideo = true;
	BOOL bSendSPS_and_PPS = false;
	BOOL bSendAudioHead = false;
	bool bConnect = false;
private:
	unsigned char* pReadBuffer;
	unsigned char* pReadCache;
	unsigned int dwReadCacheOffset1;
	unsigned int dwReadCacheOffset2;

private:
	//回调函数类
	void mAudioInit(int bits, int channels, int samplerate, int isaudio);
	void mAudioProcess(unsigned char* buffer, int length);
	void mVideoConnect();
	void mVideoDisConnect();
	void mVideoH264Extradata(unsigned char* buffer, unsigned int length);
	void mVideoH264Slice(unsigned char* buffer, unsigned int length);
	void mMessage(const char* fmt, ...);
	void mLog(const char* fmt, ...);

private:
	static DWORD WINAPI cast_screen_thread(void* args);
	static quick_time_callbacks_t g_FuncCallback;
	static bool g_isInit;
};




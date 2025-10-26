#include "gcusb.h"
#include "QuickTime.h"
#include "RW.h"
#include "H264Decoder.h"
#include "SoundPlay.h"

#include <assert.h>
#include <windows.h>
#include <iostream>
#include <map>
#include <mutex>

typedef struct 
{
	unsigned int dwDeviceIndex;
	std::string strSerialNumber;
}DeviceInfo;

static std::map<void*, DeviceInfo> DeviceInfoList;
static CH264Decoder* m_pH264Decoder;
static SoundPlay g_SoundPlay;

static void RGBDataSaveAsBmpFile(
	const char* bmpFile,                // BMP文件名称
	unsigned char* pRgbData,            // 图像数据
	int width,                           // 图像宽度  
	int height,                          // 图像高度
	int biBitCount,                      // 位图深度
	bool flipvertical)                   // 图像是否需要垂直翻转
{
	int size = 0;
	int bitsPerPixel = 3;
	if (biBitCount == 24)
	{
		bitsPerPixel = 3;
		size = width * height * bitsPerPixel * sizeof(char); // 每个像素点3个字节
	}
	else if (biBitCount == 32)
	{
		bitsPerPixel = 4;
		size = width * height * bitsPerPixel * sizeof(char); // 每个像素点4个字节
	}
	else return;

	// 位图第一部分，文件信息  
	BITMAPFILEHEADER bfh;
	bfh.bfType = (WORD)0x4d42;  //图像格式 必须为'BM'格式
	bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);//真正的数据的位置
	bfh.bfSize = size + bfh.bfOffBits;
	bfh.bfReserved1 = 0;
	bfh.bfReserved2 = 0;

	BITMAPINFOHEADER bih;
	bih.biSize = sizeof(BITMAPINFOHEADER);
	bih.biWidth = width;
	if (flipvertical)
		bih.biHeight = -height;//BMP图片从最后一个点开始扫描，显示时图片是倒着的，所以用-height，这样图片就正了  
	else
		bih.biHeight = height;
	bih.biPlanes = 1;
	bih.biBitCount = biBitCount;
	bih.biCompression = BI_RGB;
	bih.biSizeImage = size;
	bih.biXPelsPerMeter = 0;
	bih.biYPelsPerMeter = 0;
	bih.biClrUsed = 0;
	bih.biClrImportant = 0;
	FILE* fp = NULL;
	fopen_s(&fp, bmpFile, "wb");
	if (!fp)
		return;

	fwrite(&bfh, 8, 1, fp);
	fwrite(&bfh.bfReserved2, sizeof(bfh.bfReserved2), 1, fp);
	fwrite(&bfh.bfOffBits, sizeof(bfh.bfOffBits), 1, fp);
	fwrite(&bih, sizeof(BITMAPINFOHEADER), 1, fp);
	fwrite(pRgbData, size, 1, fp);
	fclose(fp);
}

void audio_init(void* user_data, int bits, int channels, int samplerate, int isaudio)
{
	printf("user_data[%u|%s] %s: %d|%d|%d|%d\n"
		, DeviceInfoList[user_data].dwDeviceIndex
		, DeviceInfoList[user_data].strSerialNumber.c_str()
		, __FUNCTION__
		, bits
		, channels
		, samplerate
		, isaudio);

	g_SoundPlay.Init();
	g_SoundPlay.SetFormat(channels, samplerate);
	g_SoundPlay.PauseAudioDevice(0);

	//user_data[1|00008110-00060D5822B9801E] log: text[AudioFormat[16][2][48000.00]]
	//user_data[1 | 00008110 - 00060D5822B9801E] audio_init: 16 | 2 | 48000 | 1

}

void audio_process(void* user_data, unsigned char* buffer, int length)
{
	static int count = 0;
	if (count++ % 60 == 0)
		printf("user_data[%u|%s] %s: %d\n"
			, DeviceInfoList[user_data].dwDeviceIndex
			, DeviceInfoList[user_data].strSerialNumber.c_str()
			, __FUNCTION__
			, length);

	g_SoundPlay.PushData(buffer, length);

}

void on_video_connect(void* user_data)
{
	printf("user_data[%u|%s] %s\n"
		, DeviceInfoList[user_data].dwDeviceIndex
		, DeviceInfoList[user_data].strSerialNumber.c_str()
		, __FUNCTION__
	);
}

void on_video_dis_connect(void* user_data)
{
	printf("user_data[%u|%s] %s\n"
		, DeviceInfoList[user_data].dwDeviceIndex
		, DeviceInfoList[user_data].strSerialNumber.c_str()
		, __FUNCTION__
	);
}

void video_h264_extradata(void* user_data, unsigned char* buffer, unsigned int length)
{
	static int count = 0;
	if (count++ % 60 == 0)
		printf("user_data[%u|%s] %s: %d\n"
			, DeviceInfoList[user_data].dwDeviceIndex
			, DeviceInfoList[user_data].strSerialNumber.c_str()
			, __FUNCTION__
			, length);

	if (m_pH264Decoder->send_extradata(buffer, length))
	{
		//CSDLMutexLocker locker(m_vmutex);
		AVFrame* frame = NULL;
		while (m_pH264Decoder->receive_frame(&frame) && frame)
		{
			printf("extradata frame : %p\n", frame);
			std::vector<unsigned char> vecRgbData;
			m_pH264Decoder->yuv2rgb(frame, vecRgbData);
			printf("extradata rgb size : %d\n", (int)vecRgbData.size());
			RGBDataSaveAsBmpFile("C:\\111.bmp", vecRgbData.data(), frame->width, frame->height, 32, true);
		}
	}
}

void video_h264_slice(void* user_data, unsigned char* buffer, unsigned int length)
{
	static int count = 0;
	if (count++ % 60 == 0)
		printf("user_data[%u|%s] %s: %d\n"
			, DeviceInfoList[user_data].dwDeviceIndex
			, DeviceInfoList[user_data].strSerialNumber.c_str()
			, __FUNCTION__
			, length);

	if (m_pH264Decoder->send_slice(buffer, length))
	{
		//CSDLMutexLocker locker(m_vmutex);
		AVFrame* frame = NULL;
		while (m_pH264Decoder->receive_frame(&frame) && frame)
		{
			printf("slice frame : %p\n", frame);
			std::vector<unsigned char> vecRgbData;
			m_pH264Decoder->yuv2rgb(frame, vecRgbData);
			printf("slice rgb size : %d\n", (int)vecRgbData.size());
			RGBDataSaveAsBmpFile("C:\\222.bmp", vecRgbData.data(), frame->width, frame->height, 32, true);
		}
	}
}

void message(void* user_data, const char* msg)
{
	printf("user_data[%u|%s] %s: text[%s]\n"
		, DeviceInfoList[user_data].dwDeviceIndex
		, DeviceInfoList[user_data].strSerialNumber.c_str()
		, __FUNCTION__
		, msg);
}

void log(void* user_data, const char* text)
{
	printf("user_data[%u|%s] %s: text[%s]\n"
		, DeviceInfoList[user_data].dwDeviceIndex
		, DeviceInfoList[user_data].strSerialNumber.c_str()
		, __FUNCTION__
		, text);
}


int main(int argc, char* argv[])
{
	SDL_version v;
	SDL_VERSION(&v);

	printf("SDL version: %d.%d.%d\n", v.major, v.minor, v.patch);

	// 初始化Audio子系统
	if (SDL_Init(SDL_INIT_AUDIO)) 
	{
		// 返回值不是0，就代表失败
		printf("SDL初始化失败: %s\n", SDL_GetError());
		return -1;
	}
	printf("SDL初始化成功\n");


	m_pH264Decoder = new CH264Decoder;
	m_pH264Decoder->Init();

	quick_time_callbacks_t callbacks;
	callbacks.audio_init = audio_init;
	callbacks.audio_process = audio_process;
	callbacks.video_connect = on_video_connect;
	callbacks.video_dis_connect = on_video_dis_connect;
	callbacks.video_h264_extradata = video_h264_extradata;
	callbacks.video_h264_slice = video_h264_slice;
	callbacks.message = message;
	callbacks.log = log;
	QuickTime::Init(&callbacks);

	std::vector<UsbDeviceData> UsbDeviceDataList = GCUSB::GetUsbDeviceList_ByVidAndComposite(VID_APPLE, true);
	printf("设备数量[%u]\n", (unsigned int)UsbDeviceDataList.size());
	for (unsigned int i = 0; i < (unsigned int)UsbDeviceDataList.size(); i++)
	{
		UsbDeviceData& device = UsbDeviceDataList[i];
		printf("serial_number[%s]\n", device.strSerial.c_str());
		printf("driver_name[%s]\n", device.strDriveName.c_str());
		{
			printf("开始投屏[%s]\n", device.strSerial.c_str());

			QuickTime* quick_time = new QuickTime;
			DeviceInfoList[quick_time].dwDeviceIndex = i + 1;
			DeviceInfoList[quick_time].strSerialNumber = device.strSerial;
			quick_time->SetUserData(quick_time);
			quick_time->Start(device.strSerial.c_str());
		}
		printf("\n");
	}




	while (1) Sleep(500);

	m_pH264Decoder->UnInit();
	delete m_pH264Decoder;
	system("pause");
	return 0;
}

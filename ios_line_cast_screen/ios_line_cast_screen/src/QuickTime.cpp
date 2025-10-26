#include "QuickTime.h"
#include "RW.h"
#include "gcdict.h"
#include "CMSampleBuf.h"

#include <assert.h>
#include <mutex>


quick_time_callbacks_t QuickTime::g_FuncCallback;
bool QuickTime::g_isInit;


QuickTime::QuickTime() :clok_Clock(0), localAudioClock(0)
{
	this->running_status = EM_RUN_STATE_STOP;
	this->user_input_data = nullptr;
	this->libusb_dev = nullptr;
	this->libusb_dev_handle = nullptr;
	this->dwReadCacheOffset1 = 0;
	this->dwReadCacheOffset2 = 0;
	this->pReadBuffer = (unsigned char*)malloc(READ_QT_BUFFER_SIZE);
	this->pReadCache = (unsigned char*)malloc(READ_QT_BUFFER_SIZE);
}

QuickTime::~QuickTime()
{
	if (this->pReadBuffer)
	{
		free(this->pReadBuffer);
		this->pReadBuffer = nullptr;
	}
	if (this->pReadCache)
	{
		free(this->pReadCache);
		this->pReadCache = nullptr;
	}
}

bool QuickTime::Init(const quick_time_callbacks_t* pCallbacks)
{
	if (!pCallbacks) return false;
	if (QuickTime::g_isInit) return true;

	//�����ص�����
	memcpy_s(&QuickTime::g_FuncCallback, sizeof(quick_time_callbacks_t), pCallbacks, sizeof(quick_time_callbacks_t));

	QuickTime::g_isInit = true;
	return true;
}

bool QuickTime::isInit()
{
	return QuickTime::g_isInit;
}

void QuickTime::SetUserData(void* user_input_data)
{
	assert(user_input_data);
	this->user_input_data = user_input_data;
}

quick_time_error_t QuickTime::SyncExec()
{
	quick_time_error_t error;
	this->running_status = EM_RUN_STATE_RUNNING;

	while (this->CheckStatus())
	{
		this->Clear();
		mLog("���ڳ�ʼ��...");
		//����豸�Ƿ�libusb���� �� ɸѡ������

		//���豸
		error = this->OpenDeviceBySerial(this->serial_number);
		if (error != QT_E_SUCCESS)
		{
			mLog("���豸ʧ��");
			Sleep(1000);
			continue;
		}
		mLog("���豸�ɹ�");

		//�ж��豸�Ƿ���Ͷ��ģʽ
		if (!this->IsValidIosDeviceWithActiveQTConfig())
		{
			mLog("�豸δ����Ͷ��ģʽ");
			Sleep(1000);
			continue;


			//����Ͷ��ģʽ
			if (!this->EnableQTConfig())
			{
				mLog("����Ͷ��ģʽʧ��1");
				Sleep(1000);
				continue;
			}
			mLog("����Ͷ��ģʽ�ɹ�");
			//���´��豸
			Sleep(2000);
			this->Close();
			continue;
		}
		mLog("�豸����Ͷ��ģʽ");

		if (!this->SetQTConfigAndInterface())
		{
			mLog("SetQTConfigAndInterface ʧ��");
			Sleep(1000);
			continue;
		}
		//this->ClearFeature();
		//this->ClearQTEndPointHalt();
		//usb_resetep(libusb_dev_handle, this->end_point_qt.btOutEndPointAddress);
		//usb_resetep(libusb_dev_handle, this->end_point_qt.btInEndPointAddress);
		//usb_resetep(libusb_dev_handle, this->end_point_mux.btOutEndPointAddress);
		//usb_resetep(libusb_dev_handle, this->end_point_mux.btInEndPointAddress);
		//usb_clear_halt(libusb_dev_handle, this->end_point_qt.btOutEndPointAddress);
		//usb_clear_halt(libusb_dev_handle, this->end_point_qt.btInEndPointAddress);
		//usb_clear_halt(libusb_dev_handle, this->end_point_mux.btOutEndPointAddress);
		//usb_clear_halt(libusb_dev_handle, this->end_point_mux.btInEndPointAddress);

		Sleep(1000);
		mLog("Ͷ����...");
		this->ReceiveLoop();
		Sleep(1000);
	}

	this->DisableQTConfig();
	Sleep(1000);
	this->Clear();
	this->running_status = EM_RUN_STATE_STOP;
	return QT_E_SUCCESS;
}

quick_time_error_t QuickTime::Start(const char* serial_number)
{
	assert(serial_number);
	strcpy_s(this->serial_number, sizeof(this->serial_number), serial_number);

	this->running_status = EM_RUN_STATE_RUNNING;
	HANDLE thread_handle = CreateThread(NULL, 0, QuickTime::cast_screen_thread, this, 0, NULL);
	if (!thread_handle) return QT_E_CREATE_THREAD_FAIL;
	CloseHandle(thread_handle);
	return QT_E_SUCCESS;
}

void QuickTime::Stop()
{
	this->running_status = EM_RUN_STATE_READY_STOP;
}

thread_running_state_t QuickTime::GetRunningState() const
{
	return this->running_status;
}





//֪ͨ�豸������Ƶ��
void QuickTime::SendHPD1()
{
	std::vector<unsigned char> sendData;
	std::vector<unsigned char> tmpData;
	// DWORD dwWidth = 1200;
	// DWORD dwHeight = 1920;
	DWORD dwWidth = 2160;
	DWORD dwHeight = 3840;
	/*
	TODO: ������go��coremedia��dict��ʽ,��ʱ��Ӳ����
	github.com/danielpaulus/quicktime_video_hack/screencapture/coremedia/dict.go

	dict��ʽ��
	{
		"Valeria" : true  //falseΪ�����ӿ�(APP������Ƶ���в�ͬ��Э��)��      trueΪvģʽ(ȫ��¼����Ļ)��ʱ�����ʾ9:41���źš�����������ʾ����
		"HEVCDecoderSupports444" : true
		"DisplaySize" :
			{
				"Width"  : 1920
				"Height" : 1200
			}
	}
	*/
	bool bValeria = true;  //1Ϊ�����׽  0Ϊ�����ӿ�
	gcdict _dict;
	_dict.add_item_uint("Width", dwWidth);
	_dict.add_item_uint("Height", dwHeight);
	gcdict _dict2;
	_dict2.add_item_bool("Valeria", bValeria);
	//_dict2.add_item_bool("HEVCDecoderSupports", true);
	_dict2.add_item_dict("DisplaySize", _dict);

	tmpData = _dict2.vec_data;
	//if (SCREEN_MODE_DEVICE_BASE == gConfig.nScreenMode)
	//{
	//    tmpData =
	//    {
	//        0xC7,0x00,0x00,0x00,0x74,0x63,0x69,0x64,0x20,0x00,0x00,0x00,0x76,0x79,0x65,0x6B,0x0F,0x00,0x00,0x00,0x6B,0x72,0x74,0x73,0x56,0x61,0x6C,0x65,0x72,0x69,0x61,0x09,0x00,0x00,0x00,0x76,0x6C,0x75,0x62,
	//        0x00 /*1Ϊ�����׽  0Ϊ�����ӿ�*/,
	//        0x2F,0x00,0x00,0x00,0x76,0x79,0x65,0x6B,0x1E,0x00,0x00,0x00,0x6B,0x72,0x74,0x73,0x48,0x45,0x56,0x43,0x44,0x65,0x63,0x6F,0x64,0x65,0x72,0x53,0x75,0x70,0x70,0x6F,0x72,0x74,0x73,0x34,0x34,0x34,0x09,0x00,0x00,0x00,0x76,0x6C,0x75,0x62,0x01,0x70,0x00,0x00,0x00,0x76,0x79,0x65,0x6B,0x13,0x00,0x00,0x00,0x6B,0x72,0x74,0x73,0x44,0x69,0x73,0x70,0x6C,0x61,0x79,0x53,0x69,0x7A,0x65,0x55,0x00,0x00,0x00,0x74,0x63,0x69,0x64,0x26,0x00,0x00,0x00,0x76,0x79,0x65,0x6B,0x0D,0x00,0x00,0x00,0x6B,0x72,0x74,0x73,0x57,0x69,0x64,0x74,0x68,0x11,0x00,0x00,0x00,0x76,0x62,0x6D,0x6E,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x9E,0x40,0x27,0x00,0x00,0x00,0x76,0x79,0x65,0x6B,0x0E,0x00,0x00,0x00,0x6B,0x72,0x74,0x73,0x48,0x65,0x69,0x67,0x68,0x74,0x11,0x00,0x00,0x00,0x76,0x62,0x6D,0x6E,0x06,0x00,0x00,0x00,0x00,0x00,0xC0,0x92,0x40
	//    };
	//}
	//else if (SCREEN_MODE_SOFTWARE_CAPTURE == gConfig.nScreenMode)
	//{
	//    tmpData =
	//    {
	//        0xC7,0x00,0x00,0x00,0x74,0x63,0x69,0x64,0x20,0x00,0x00,0x00,0x76,0x79,0x65,0x6B,0x0F,0x00,0x00,0x00,0x6B,0x72,0x74,0x73,0x56,0x61,0x6C,0x65,0x72,0x69,0x61,0x09,0x00,0x00,0x00,0x76,0x6C,0x75,0x62,
	//        0x01 /*1Ϊ�����׽  0Ϊ�����ӿ�*/,
	//        0x2F,0x00,0x00,0x00,0x76,0x79,0x65,0x6B,0x1E,0x00,0x00,0x00,0x6B,0x72,0x74,0x73,0x48,0x45,0x56,0x43,0x44,0x65,0x63,0x6F,0x64,0x65,0x72,0x53,0x75,0x70,0x70,0x6F,0x72,0x74,0x73,0x34,0x34,0x34,0x09,0x00,0x00,0x00,0x76,0x6C,0x75,0x62,0x01,0x70,0x00,0x00,0x00,0x76,0x79,0x65,0x6B,0x13,0x00,0x00,0x00,0x6B,0x72,0x74,0x73,0x44,0x69,0x73,0x70,0x6C,0x61,0x79,0x53,0x69,0x7A,0x65,0x55,0x00,0x00,0x00,0x74,0x63,0x69,0x64,0x26,0x00,0x00,0x00,0x76,0x79,0x65,0x6B,0x0D,0x00,0x00,0x00,0x6B,0x72,0x74,0x73,0x57,0x69,0x64,0x74,0x68,0x11,0x00,0x00,0x00,0x76,0x62,0x6D,0x6E,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x9E,0x40,0x27,0x00,0x00,0x00,0x76,0x79,0x65,0x6B,0x0E,0x00,0x00,0x00,0x6B,0x72,0x74,0x73,0x48,0x65,0x69,0x67,0x68,0x74,0x11,0x00,0x00,0x00,0x76,0x62,0x6D,0x6E,0x06,0x00,0x00,0x00,0x00,0x00,0xC0,0x92,0x40
	//    };
	//}
	//else
	//{
	//    //Ĭ���豸����
	//    tmpData =
	//    {
	//        0xC7,0x00,0x00,0x00,0x74,0x63,0x69,0x64,0x20,0x00,0x00,0x00,0x76,0x79,0x65,0x6B,0x0F,0x00,0x00,0x00,0x6B,0x72,0x74,0x73,0x56,0x61,0x6C,0x65,0x72,0x69,0x61,0x09,0x00,0x00,0x00,0x76,0x6C,0x75,0x62,
	//        0x00 /*1Ϊ�����׽  0Ϊ�����ӿ�*/,
	//        0x2F,0x00,0x00,0x00,0x76,0x79,0x65,0x6B,0x1E,0x00,0x00,0x00,0x6B,0x72,0x74,0x73,0x48,0x45,0x56,0x43,0x44,0x65,0x63,0x6F,0x64,0x65,0x72,0x53,0x75,0x70,0x70,0x6F,0x72,0x74,0x73,0x34,0x34,0x34,0x09,0x00,0x00,0x00,0x76,0x6C,0x75,0x62,0x01,0x70,0x00,0x00,0x00,0x76,0x79,0x65,0x6B,0x13,0x00,0x00,0x00,0x6B,0x72,0x74,0x73,0x44,0x69,0x73,0x70,0x6C,0x61,0x79,0x53,0x69,0x7A,0x65,0x55,0x00,0x00,0x00,0x74,0x63,0x69,0x64,0x26,0x00,0x00,0x00,0x76,0x79,0x65,0x6B,0x0D,0x00,0x00,0x00,0x6B,0x72,0x74,0x73,0x57,0x69,0x64,0x74,0x68,0x11,0x00,0x00,0x00,0x76,0x62,0x6D,0x6E,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x9E,0x40,0x27,0x00,0x00,0x00,0x76,0x79,0x65,0x6B,0x0E,0x00,0x00,0x00,0x6B,0x72,0x74,0x73,0x48,0x65,0x69,0x67,0x68,0x74,0x11,0x00,0x00,0x00,0x76,0x62,0x6D,0x6E,0x06,0x00,0x00,0x00,0x00,0x00,0xC0,0x92,0x40
	//    };
	//}


	sendData.resize(20 + tmpData.size());
	*(int*)(sendData.data() + 0) = (int)(20 + tmpData.size());
	*(int*)(sendData.data() + 4) = AsynPacketMagic;
	*(UINT64*)(sendData.data() + 8) = qwEmptyCFType;
	*(int*)(sendData.data() + 16) = HPD1;
	memcpy(sendData.data() + 20, tmpData.data(), tmpData.size());
	SendQTBulk(sendData);
	SendQTBulk(sendData);
	mLog("SendHPD1 Valeria[%d] w[%u] h[%u]", bValeria, dwWidth, dwHeight);
}

//֪ͨ�豸������Ƶ��
void QuickTime::SendHPA1(UINT64 qwDeviceClockRef)
{
	std::vector<unsigned char> sendData;
	std::vector<unsigned char> tmpData;
	/*
	TODO: ������go��coremedia��dict��ʽ,��ʱ��Ӳ����
	github.com/danielpaulus/quicktime_video_hack/screencapture/coremedia/dict.go

	dict��ʽ��
	{
		"BufferAheadInterval" : 0.07300000000000001
		"deviceUID" : "Valeria"
		"ScreenLatency" : 0.04
		"formats" :
		{
			binary.LittleEndian.PutUint64(adsbBytes, math.Float64bits(adsb.SampleRate))
			var index = 8
			binary.LittleEndian.PutUint32(adsbBytes[index:], AudioFormatIDLpcm)     //0x6C70636D��LPCM��
			index += 4
			binary.LittleEndian.PutUint32(adsbBytes[index:], adsb.FormatFlags)
			index += 4
			binary.LittleEndian.PutUint32(adsbBytes[index:], adsb.BytesPerPacket)
			index += 4
			binary.LittleEndian.PutUint32(adsbBytes[index:], adsb.FramesPerPacket)
			index += 4
			binary.LittleEndian.PutUint32(adsbBytes[index:], adsb.BytesPerFrame)
			index += 4
			binary.LittleEndian.PutUint32(adsbBytes[index:], adsb.ChannelsPerFrame)
			index += 4
			binary.LittleEndian.PutUint32(adsbBytes[index:], adsb.BitsPerChannel)
			index += 4
			binary.LittleEndian.PutUint32(adsbBytes[index:], adsb.Reserved)
			index += 4
			binary.LittleEndian.PutUint64(adsbBytes[index:], math.Float64bits(adsb.SampleRate))
			index += 8
			binary.LittleEndian.PutUint64(adsbBytes[index:], math.Float64bits(adsb.SampleRate))
		}
		"EDIDAC3Support" : 0
		"deviceName" : "Valeria"
	}
	*/
	tmpData =
	{
		0x3D,0x01,0x00,0x00,0x74,0x63,0x69,0x64,0x34,0x00,0x00,0x00,0x76,0x79,0x65,0x6B,0x1B,0x00,0x00,0x00,0x6B,0x72,0x74,0x73,0x42,0x75,0x66,0x66,0x65,0x72,0x41,0x68,0x65,0x61,0x64,0x49,0x6E,0x74,0x65,0x72,0x76,0x61,0x6C,0x11,0x00,0x00,0x00,0x76,0x62,0x6D,0x6E,0x06,0xE4,0xA5,0x9B,0xC4,0x20,0xB0,0xB2,0x3F,0x28,0x00,0x00,0x00,0x76,0x79,0x65,0x6B,0x11,0x00,0x00,0x00,0x6B,0x72,0x74,0x73,0x64,0x65,0x76,0x69,0x63,0x65,0x55,0x49,0x44,0x0F,0x00,0x00,0x00,0x76,0x72,0x74,0x73,0x56,0x61,0x6C,0x65,0x72,0x69,0x61,0x2E,0x00,0x00,0x00,0x76,0x79,0x65,0x6B,0x15,0x00,0x00,0x00,0x6B,0x72,0x74,0x73,0x53,0x63,0x72,0x65,0x65,0x6E,0x4C,0x61,0x74,0x65,0x6E,0x63,0x79,0x11,0x00,0x00,0x00,0x76,0x62,0x6D,0x6E,0x06,0x7B,0x14,0xAE,0x47,0xE1,0x7A,0xA4,0x3F,0x57,0x00,0x00,0x00,0x76,0x79,0x65,0x6B,0x0F,0x00,0x00,0x00,0x6B,0x72,0x74,0x73,0x66,0x6F,0x72,0x6D,0x61,0x74,0x73,0x40,0x00,0x00,0x00,0x76,0x74,0x61,0x64,0x00,0x00,0x00,0x00,0x00,0x70,0xE7,0x40,0x6D,0x63,0x70,0x6C,0x0C,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x70,0xE7,0x40,0x00,0x00,0x00,0x00,0x00,0x70,0xE7,0x40,0x2B,0x00,0x00,0x00,0x76,0x79,0x65,0x6B,0x16,0x00,0x00,0x00,0x6B,0x72,0x74,0x73,0x45,0x44,0x49,0x44,0x41,0x43,0x33,0x53,0x75,0x70,0x70,0x6F,0x72,0x74,0x0D,0x00,0x00,0x00,0x76,0x62,0x6D,0x6E,0x03,0x00,0x00,0x00,0x00,0x29,0x00,0x00,0x00,0x76,0x79,0x65,0x6B,0x12,0x00,0x00,0x00,0x6B,0x72,0x74,0x73,0x64,0x65,0x76,0x69,0x63,0x65,0x4E,0x61,0x6D,0x65,0x0F,0x00,0x00,0x00,0x76,0x72,0x74,0x73,0x56,0x61,0x6C,0x65,0x72,0x69,0x61
	};
	sendData.resize(20 + tmpData.size());
	*(int*)(sendData.data() + 0) = (int)(20 + tmpData.size());
	*(int*)(sendData.data() + 4) = AsynPacketMagic;
	*(UINT64*)(sendData.data() + 8) = qwDeviceClockRef;
	*(int*)(sendData.data() + 16) = HPA1;
	memcpy(sendData.data() + 20, tmpData.data(), tmpData.size());
	SendQTBulk(sendData);
	mLog("SendHPA1");
}

void QuickTime::ReceiveSyncPacket_CWPA(const std::vector<unsigned char>& vecData)
{
	UINT64 qwClockRef;
	UINT64 qwCorrelationID;
	UINT64 qwDeviceClockRef;
	//����
	memcpy(&qwClockRef, vecData.data() + 8, 8);
	memcpy(&qwCorrelationID, vecData.data() + 20, 8);
	memcpy(&qwDeviceClockRef, vecData.data() + 28, 8);
	if (qwClockRef != qwEmptyCFType)
	{
		mLog("warning CWPA packet should have empty CFTypeID for ClockRef but has:0x%016llx\n", qwClockRef);
	}
	mLog("RECV : SYNC_CWPA {ClockRef:0x%016llx, CorrelationID:0x%016llx, DeviceClockRef:0x%016llx}", qwClockRef, qwCorrelationID, qwDeviceClockRef);

	UINT64 qwLocalClockRef = qwDeviceClockRef + 1000;
	//����������Ƶʱ��
	this->localAudioClock = MyClock(qwLocalClockRef);
	//�����豸ʱ��ID
	this->deviceAudioClockRef = qwDeviceClockRef;

	//1��֪ͨ�豸������Ƶ��(ASYN HPD1)
	SendHPD1();
	//2���ظ�CWPA-RPLY
	{
		std::vector<unsigned char> sendData;
		//��Ӧ
		sendData.resize(28);
		*(int*)(sendData.data() + 0) = 28;
		*(int*)(sendData.data() + 4) = ReplyPacketMagic;
		*(UINT64*)(sendData.data() + 8) = qwCorrelationID;
		*(int*)(sendData.data() + 16) = 0;
		*(UINT64*)(sendData.data() + 20) = qwLocalClockRef;
		SendQTBulk(sendData);
	}
	//3��֪ͨ�豸������Ƶ��(ASYN HPA1)
	SendHPA1(qwDeviceClockRef);
}

void QuickTime::ReceiveSyncPacket_AFMT(const std::vector<unsigned char>& vecData)
{
	std::vector<unsigned char> sendData;
	std::vector<unsigned char> tmpData;
	UINT64 qwClockRef;
	UINT64 qwCorrelationID;
	memcpy(&qwClockRef, vecData.data() + 8, 8);
	memcpy(&qwCorrelationID, vecData.data() + 20, 8);
	mLog("RECV : SYNC_AFMT : {ClockRef:0x%016llx, CorrelationID:0x%016llx}", qwClockRef, qwCorrelationID);
	tmpData =
	{
		//TODO:�ֵ�
		//{"Error":NSNumberUint32(0)}
		0x2A,0x00,0x00,0x00,0x74,0x63,0x69,0x64,0x22,0x00,0x00,0x00,0x76,0x79,0x65,0x6B,0x0D,0x00,0x00,0x00,0x6B,0x72,0x74,0x73,0x45,0x72,0x72,0x6F,0x72,0x0D,0x00,0x00,0x00,0x76,0x62,0x6D,0x6E,0x03,0x00,0x00,0x00,0x00
	};
	sendData.resize(20 + tmpData.size());
	*(int*)(sendData.data() + 0) = (int)(20 + tmpData.size());
	*(int*)(sendData.data() + 4) = ReplyPacketMagic;
	*(UINT64*)(sendData.data() + 8) = qwCorrelationID;
	*(int*)(sendData.data() + 16) = 0x00;
	memcpy(sendData.data() + 20, tmpData.data(), tmpData.size());
	SendQTBulk(sendData);

}

void QuickTime::ReceiveSyncPacket_CVRP(const std::vector<unsigned char>& vecData)
{
	UINT64 qwClockRef;
	UINT64 qwCorrelationID;
	UINT64 qwDeviceClockRef;
	std::vector<unsigned char> sendData;
	std::vector<unsigned char> tmpData;
	memcpy(&qwClockRef, vecData.data() + 8, 8);
	memcpy(&qwCorrelationID, vecData.data() + 20, 8);
	memcpy(&qwDeviceClockRef, vecData.data() + 28, 8);
	//vecData.data() + 28��ʣ���ֵ��а���h264 ͼƬ�����в�������PPS/SPS��

	//����needʱ�ӺͰ�����
	sendData.resize(20);
	*(int*)(sendData.data() + 0) = 20;
	*(int*)(sendData.data() + 4) = AsynPacketMagic;
	*(UINT64*)(sendData.data() + 8) = qwDeviceClockRef;
	*(int*)(sendData.data() + 16) = NEED;
	qwNeedClockRef = qwDeviceClockRef;
	vecNeedMessage = sendData;

	//�ظ�need��
	mLog("Send NEED 0x%016llx", qwNeedClockRef);
	SendQTBulk(vecNeedMessage);

	//�ظ�cvrp
	UINT64 qwClockRef2 = qwDeviceClockRef + 0x1000AF;
	mLog("Send CVRP-RPLY {correlation:0x%016llx, clockRef:0x%016llx}", qwCorrelationID, qwClockRef2);
	sendData.resize(28);
	*(int*)(sendData.data() + 0) = 28;
	*(int*)(sendData.data() + 4) = ReplyPacketMagic;
	*(UINT64*)(sendData.data() + 8) = qwCorrelationID;
	*(int*)(sendData.data() + 16) = 0x00;
	*(UINT64*)(sendData.data() + 20) = qwClockRef2;
	SendQTBulk(sendData);
}

void QuickTime::ReceiveSyncPacket_CLOK(const std::vector<unsigned char>& vecData)
{
	std::vector<unsigned char> sendData;
	UINT64 qwClockRef;
	UINT64 qwCorrelationID;
	memcpy(&qwClockRef, vecData.data() + 8, 8);
	memcpy(&qwCorrelationID, vecData.data() + 20, 8);
	mLog("RECV : SYNC_CLOK : {ClockRef:0x%016llx, CorrelationID:0x%016llx}", qwClockRef, qwCorrelationID);
	//����ʱ��
	this->clok_Clock = MyClock(qwClockRef);

	//�ظ�CLOK
	UINT64 qwClockRef2 = qwClockRef + 0x10000;
	mLog("Send SYNC_CLOK {correlation:0x%016llx, clockRef:0x%016llx}", qwCorrelationID, qwClockRef2);
	sendData.resize(28);
	*(int*)(sendData.data() + 0) = 28;
	*(int*)(sendData.data() + 4) = ReplyPacketMagic;
	*(UINT64*)(sendData.data() + 8) = qwCorrelationID;
	*(int*)(sendData.data() + 16) = 0x00;
	*(UINT64*)(sendData.data() + 20) = qwClockRef2;
	SendQTBulk(sendData);
}

void QuickTime::ReceiveSyncPacket_TIME(const std::vector<unsigned char>& vecData)
{
	std::vector<unsigned char> sendData;
	UINT64 qwClockRef;
	UINT64 qwCorrelationID;
	memcpy(&qwClockRef, vecData.data() + 8, 8);
	memcpy(&qwCorrelationID, vecData.data() + 20, 8);
	mLog("RECV : SYNC_TIME : {ClockRef:0x%016llx, CorrelationID:0x%016llx}", qwClockRef, qwCorrelationID);
	//�ظ�TIME
	UINT64 qwCurrentTime = this->clok_Clock.GetTime();
	sendData.resize(0x2C);
	*(int*)(sendData.data() + 0) = 0x2C;
	*(int*)(sendData.data() + 4) = ReplyPacketMagic;
	*(UINT64*)(sendData.data() + 8) = qwCorrelationID;
	*(int*)(sendData.data() + 16) = 0x00;
	*(UINT64*)(sendData.data() + 20) = qwCurrentTime;
	*(int*)(sendData.data() + 28) = 1000;   //����
	*(int*)(sendData.data() + 32) = 1;
	*(UINT64*)(sendData.data() + 36) = 0;
	mLog("Send TIME-REPLY {CorrelationID:0x%016llx, qwID: %llu, time:%llu}", qwCorrelationID, this->clok_Clock.qwID, qwCurrentTime);
	SendQTBulk(sendData);

}

void QuickTime::ReceiveSyncPacket_OG(const std::vector<unsigned char>& vecData)
{
	std::vector<unsigned char> sendData;
	UINT64 qwClockRef;
	UINT64 qwCorrelationID;
	memcpy(&qwClockRef, vecData.data() + 8, 8);
	memcpy(&qwCorrelationID, vecData.data() + 20, 8);
	mLog("RECV : SYNC_OG : {ClockRef:0x%016llx, CorrelationID:0x%016llx}", qwClockRef, qwCorrelationID);

	sendData.resize(24);
	*(int*)(sendData.data() + 0) = 24;
	*(int*)(sendData.data() + 4) = ReplyPacketMagic;
	*(UINT64*)(sendData.data() + 8) = qwCorrelationID;
	*(UINT64*)(sendData.data() + 16) = 0x00;
	SendQTBulk(sendData);
}

void QuickTime::ReceiveSyncPacket_SKEW(const std::vector<unsigned char>& vecData)
{
	//��Ƶʱ��ƫ��У׼
	// ���ʱ�Ӷ��룬��Ϊ 48000
	//������ǵ�ʱ�ӽ�������ĳЩֵ���� 48000
	//������ǵ�ʱ�ӱ��豸ʱ�ӿ죬��ĳЩֵ���� 48000 ���ʵʩ��ȷ������Ӧ�ÿ���ƫб��Ӧ������ 48000����ʱƫ���С(48000 + x where - 1 < x < 1)
	std::vector<unsigned char> sendData;
	UINT64 qwClockRef;
	UINT64 qwCorrelationID;
	memcpy(&qwClockRef, vecData.data() + 8, 8);
	memcpy(&qwCorrelationID, vecData.data() + 20, 8);
	//LOG_INFO("RECV : SYNC_SKEW : {ClockRef:0x%016llx, CorrelationID:0x%016llx}", qwClockRef, qwCorrelationID);

	double lfClockOffset = 0.0;
	UINT64 qwLocalRunningMillSecond = this->LastEatFrameReceivedLocalAudioMillSecond - this->StartTimeLocalAudioMillSecond;      //����ʱ�Ӿ���������
	UINT64 qwDeviceRunningMillSecond = this->LastEatFrameReceivedDeviceAudioMillSecond - this->StartTimeDeviceAudioMillSecond;   //�豸ʱ�Ӿ���������
	if (qwLocalRunningMillSecond > qwDeviceRunningMillSecond)
	{
		//����ʱ�ӿ�(<48000)
		lfClockOffset = 48000.0 - (double)(qwLocalRunningMillSecond - qwDeviceRunningMillSecond) / 48;
	}
	else if (qwLocalRunningMillSecond < qwDeviceRunningMillSecond)
	{
		//����ʱ����(>48000)
		lfClockOffset = 48000.0 + (double)(qwDeviceRunningMillSecond - qwLocalRunningMillSecond) / 48;
	}
	else
	{
		lfClockOffset = 48000.0;
	}
	//LOG_INFO("REF : SYNC_SKEW : %.2lf", lfClockOffset);

	sendData.resize(28);
	*(int*)(sendData.data() + 0) = 28;
	*(int*)(sendData.data() + 4) = ReplyPacketMagic;
	*(UINT64*)(sendData.data() + 8) = qwCorrelationID;
	*(int*)(sendData.data() + 16) = 0x00;
	*(double*)(sendData.data() + 20) = lfClockOffset; //48000.0;
	SendQTBulk(sendData);
}

void QuickTime::ReceiveSyncPacket_STOP(const std::vector<unsigned char>& vecData)
{
	std::vector<unsigned char> sendData;
	UINT64 qwClockRef;
	UINT64 qwCorrelationID;
	memcpy(&qwClockRef, vecData.data() + 8, 8);
	memcpy(&qwCorrelationID, vecData.data() + 20, 8);
	mLog("RECV : SYNC_STOP : {ClockRef:0x%016llx, CorrelationID:0x%016llx}", qwClockRef, qwCorrelationID);

	//��Ƶʱ�ӳ�ʼ��������ʼ���ᵼ���������ɳ��
	bFirstAudioTimeTaken = false;
	this->StartTimeDeviceAudioMillSecond = 0;
	this->StartTimeLocalAudioMillSecond = 0;
	this->LastEatFrameReceivedDeviceAudioMillSecond = 0;
	this->LastEatFrameReceivedLocalAudioMillSecond = 0;

	sendData.resize(24);
	*(int*)(sendData.data() + 0) = 24;
	*(int*)(sendData.data() + 4) = ReplyPacketMagic;
	*(UINT64*)(sendData.data() + 8) = qwCorrelationID;
	*(UINT32*)(sendData.data() + 16) = 0x00;
	*(UINT32*)(sendData.data() + 20) = 0x00;
	SendQTBulk(sendData);
}

void QuickTime::ReceiveSyncPacket(const std::vector<unsigned char>& vecData)
{
	int nEventType = 0;
	memcpy(&nEventType, vecData.data() + 16, 4);
	//mLog("sync packet type : 0x%08x", nEventType);
	switch (nEventType)
	{
	case CLOK:
		ReceiveSyncPacket_CLOK(vecData);
		break;
	case TIME:
		ReceiveSyncPacket_TIME(vecData);
		break;
	case CWPA:
		ReceiveSyncPacket_CWPA(vecData);
		break;
	case AFMT:
		ReceiveSyncPacket_AFMT(vecData);
		break;
	case CVRP:
		ReceiveSyncPacket_CVRP(vecData);
		break;
	case OG:
		//��Ƶ��ʼ�����Ӻ󴥷�һ�Σ�����ÿ�ιر���˷紥��
		ReceiveSyncPacket_OG(vecData);
		break;
	case SKEW:
		ReceiveSyncPacket_SKEW(vecData);
		break;
	case STOP:
		//��Ƶ������ÿ�ο�����˷紥��(΢�����������߿����)
		ReceiveSyncPacket_STOP(vecData);
		//DebugBreak();
		break;
	case CEPM:
		//LOG_WARNING("ReceiveSyncPacket unknown magic type:(0x%x) CEPM \n", nEventType);
		//LOG_WARNING("data : %s\n", QuickTime::GetVectorHexString2(vecData).c_str());
		break;
	default:
		//DebugBreak();
		//LOG_WARNING("ReceiveSyncPacket unknown magic type:(0x%x)\n", nEventType);
		//LOG_WARNING("data : %s\n", QuickTime::GetVectorHexString2(vecData).c_str());
		break;
	}
}

void QuickTime::ReceiveAsynPacket_CMSampleBuffer(const std::vector<unsigned char>& vecData, UINT32 mediaType, bool* pbError)
{
	//�豸����
	if (!bConnect)
	{
		bConnect = true;
		mVideoConnect();
	}

	//ClockRef
	UINT64 qwClockRef;
	memcpy(&qwClockRef, vecData.data() + 8, 8);
	std::vector<unsigned char> vecCMSampleBuffer(vecData.begin() + 20, vecData.end());
	bool bError = false;
	CMSampleBuffer CMSampleBuf(vecCMSampleBuffer, mediaType, &bError);
	if (bError)
	{
		if (pbError)*pbError = true;
		return;
	}
	if (mediaType == MediaTypeSound)
	{
		//ʱ�Ӹ���
		if (!bFirstAudioTimeTaken)
		{
			bFirstAudioTimeTaken = true;
			this->StartTimeDeviceAudioMillSecond = MyClock::CMTimeToMilliSecond(CMSampleBuf.OutputPresentationTimestamp);
			this->StartTimeLocalAudioMillSecond = this->localAudioClock.GetTime();
		}
		this->LastEatFrameReceivedDeviceAudioMillSecond = MyClock::CMTimeToMilliSecond(CMSampleBuf.OutputPresentationTimestamp);
		this->LastEatFrameReceivedLocalAudioMillSecond = this->localAudioClock.GetTime();

		if (CMSampleBuf.HasFormatDescription)
		{
			//�״η�����Ƶ��
			bFirstProcessAudio = false;

			mLog("AudioFormat[%u][%u][%.2lf]",
				CMSampleBuf.FormatDescription.AudioStreamBasicDescription.uint32BitsPerChannel,
				CMSampleBuf.FormatDescription.AudioStreamBasicDescription.uint32ChannelsPerFrame,
				CMSampleBuf.FormatDescription.AudioStreamBasicDescription.float64SampleRate);

			mAudioInit(
				CMSampleBuf.FormatDescription.AudioStreamBasicDescription.uint32BitsPerChannel,
				CMSampleBuf.FormatDescription.AudioStreamBasicDescription.uint32ChannelsPerFrame,
				(int)CMSampleBuf.FormatDescription.AudioStreamBasicDescription.float64SampleRate,
				1);
		}
		if (bFirstProcessAudio)
		{
			//��û������Ƶͷ��һ���ߵ�����������豸����������(�Ӵ�����)�������˳������ؿ�Ҳ��
			mLog("warning : not set audio head");
			if (pbError) *pbError = true;
			return;
		}
		if (!CMSampleBuf.SampleData.empty())
		{
			mAudioProcess(CMSampleBuf.SampleData.data(), (int)CMSampleBuf.SampleData.size());
		}
		
		/*
		//����wav��Ƶ
		FILE* fp = NULL;
		fopen_s(&fp, "C:\\TEST.pcm", "ab+");
		if (fp)
		{
			fwrite(CMSampleBuf.SampleData.data(), CMSampleBuf.SampleData.size(), 1, fp);
			fclose(fp);
		}
		*/
	}
	else if (mediaType == MediaTypeVideo)
	{
		int length;
		if (CMSampleBuf.HasFormatDescription)
		{
			mLog("CMSampleBuf.HasFormatDescription == true");
			mLog("CMSampleBuf.FormatDescription.SPS_and_PPS.size() == %d", CMSampleBuf.FormatDescription.SPS_and_PPS.size());
			mLog("CMSampleBuf.FormatDescription.SPS_and_PPS : %s\n", RW::GetVectorHexString(CMSampleBuf.FormatDescription.SPS_and_PPS).c_str());
			mLog("send SPS_and_PPS bFirstProcessVideo == [%d]", bFirstProcessVideo ? 1 : 0);
			bSendSPS_and_PPS = true;
			//type 1 extradata     type 0 slice
			mVideoH264Extradata(CMSampleBuf.FormatDescription.SPS_and_PPS.data(), (int)CMSampleBuf.FormatDescription.SPS_and_PPS.size());
		}

		if (CMSampleBuf.SampleData.empty())
		{
			mLog("warning CMSampleBuf.SampleData  empty!!  vecCMSampleBuffer : %s\n", RW::GetVectorHexString(vecCMSampleBuffer).c_str());
			return;
		}
		//if (CMSampleBuf.SampleData.size() == 800)
		//{
		//    LOG_DEBUG("CMSampleBuf.SampleData  == 800!!");
		//    LOG_INFO("vecCMSampleBuffer : %s\n", IosDevice::GetVectorHexString2(vecCMSampleBuffer).c_str());
		//}
		const unsigned char* ptr = CMSampleBuf.SampleData.data();
		const unsigned char* ptr_end = CMSampleBuf.SampleData.data() + CMSampleBuf.SampleData.size();
		//LOG_DEBUG("video length : %d", ptr_end - ptr);
		while (ptr < ptr_end)
		{
			//���
			*(unsigned char*)((unsigned char*)&length + 0) = *(unsigned char*)(ptr + 3);
			*(unsigned char*)((unsigned char*)&length + 1) = *(unsigned char*)(ptr + 2);
			*(unsigned char*)((unsigned char*)&length + 2) = *(unsigned char*)(ptr + 1);
			*(unsigned char*)((unsigned char*)&length + 3) = *(unsigned char*)(ptr + 0);

			if (bSendSPS_and_PPS == false)
			{
				//bSendSPS_and_PPS = true;
				//��û������Ƶͷ��һ���ߵ�����������豸����������(�Ӵ�����)�������˳������ؿ�Ҳ��
				if (pbError)
				{
					*pbError = true;
				}
				mLog("error : not set video head");
				return;
			}
			//type 1 extradata     type 0 slice 
			mVideoH264Slice((unsigned char*)ptr, length + 4);
			ptr += (length + 4);
		}

		
		//����h264��Ƶ
		if(false)
		{
			int length;
			unsigned char startCode[] = { 0, 0, 0, 1 };
			FILE* fp = NULL;
			fopen_s(&fp, "C:\\TEST.h264", "ab+");
			if (fp)
			{
				if (CMSampleBuf.HasFormatDescription)
				{
					fwrite(startCode, 4, 1, fp);
					fwrite(CMSampleBuf.FormatDescription.PPS.data(), CMSampleBuf.FormatDescription.PPS.size(), 1, fp);
					fwrite(startCode, 4, 1, fp);
					fwrite(CMSampleBuf.FormatDescription.SPS.data(), CMSampleBuf.FormatDescription.SPS.size(), 1, fp);
				}

				const unsigned char* ptr = CMSampleBuf.SampleData.data();
				const unsigned char* ptr_end = CMSampleBuf.SampleData.data() + CMSampleBuf.SampleData.size();
				while (ptr < ptr_end)
				{
					//���
					*(unsigned char*)((unsigned char*)&length + 0) = *(unsigned char*)(ptr + 3);
					*(unsigned char*)((unsigned char*)&length + 1) = *(unsigned char*)(ptr + 2);
					*(unsigned char*)((unsigned char*)&length + 2) = *(unsigned char*)(ptr + 1);
					*(unsigned char*)((unsigned char*)&length + 3) = *(unsigned char*)(ptr + 0);


					fwrite(startCode, 4, 1, fp);
					fwrite(ptr + 4, length, 1, fp);
					ptr += 4;
					ptr += length;
				}

				fclose(fp);
			}
		}
		
	}
	else
	{
		//DebugBreak();
	}
}

void QuickTime::ReceiveAsynPacket(const std::vector<unsigned char>& vecData, bool* pbError)
{
	bool bError = false;

	int nEventType = 0;
	memcpy(&nEventType, vecData.data() + 16, 4);

	//LOG_DEBUG("asyn packet type : 0x%08x [%c%c%c%c]", nEventType
	//	, *(char*)((char*)&nEventType + 3)
	//	, *(char*)((char*)&nEventType + 2)
	//	, *(char*)((char*)&nEventType + 1)
	//	, *(char*)((char*)&nEventType + 0));

	switch (nEventType)
	{
	case FEED:
		//CMSampleBuffer(h264 ��Ƶ����)
		//LOG_DEBUG("RECV : FEED");
		ReceiveAsynPacket_CMSampleBuffer(vecData, MediaTypeVideo, &bError);
		//�ظ�NEED
		//LOG_DEBUG("Send NEED 0x%016llx\n", qwNeedClockRef);
		SendQTBulk(vecNeedMessage);
		if (bError)
		{
			if (pbError) *pbError = true;
			return;
		}
		break;
	case EAT:
		//����ظ�
		//CMSampleBuffer(��Ƶ����)(wav?)
		//LOG_DEBUG("RECV : EAT");
		ReceiveAsynPacket_CMSampleBuffer(vecData, MediaTypeSound, &bError);
		if (bError)
		{
			if (pbError) *pbError = true;
			return;
		}
		break;
	case TJMP:
		//����ظ�
		//Asyn TJMP - Time Jump Notification��ʱ����ת֪ͨ��
		//LOG_DEBUG("RECV : TJMP");
		break;
	case SRAT:
		//����ظ�
		//Asyn SRAT - Set time rate and Ancho������ʱ�����ʺ�ê�㣩
		//LOG_DEBUG("RECV : SRAT");
		break;
	case SPRP:
		//����ظ�
		//��Ƶ��������
		//ObeyEmptyMediaMarkers = true
		//RenderEmptyMedia = false
		//LOG_DEBUG("RECV : SPRP");
		break;
	case TBAS:
		//����ظ�
		//Asyn TBAS - Set TimeBase
		//LOG_DEBUG("RECV : TBAS");
		break;
	case RELS:
		//����ظ�
		// Tell us about a released Clock on the device
		//LOG_DEBUG("RECV : RELS");
		break;
	default:
		//DebugBreak();
		//LOG_WARNING("ReceiveAsynPacket unknown magic type:(0x%x)", nEventType);
		//LOG_WARNING("data : %s", IosDevice::GetVectorHexString2(vecData).c_str());
		break;
	}
}

void QuickTime::SendPing(int timeout)
{
	SendQTBulk({ 0x10, 0x00, 0x00, 0x00, 0x67, 0x6E, 0x69, 0x70, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 }, timeout);
}

int QuickTime::SendQTBulk(std::vector<unsigned char> vecData, int timeout)
{
	return usb_bulk_write(libusb_dev_handle, this->end_point_qt.btOutEndPointAddress, (char*)(vecData.data()), (int)vecData.size(), timeout);
}

int QuickTime::ReadQTBulk(std::vector<unsigned char>& vecData, int timeout)
{
	int nRet;

	vecData.clear();
	if (dwReadCacheOffset1 < dwReadCacheOffset2)
	{
		//������
		int nOnePacketSize = *(int*)(pReadCache + dwReadCacheOffset1 + 0);
		memcpy(pReadBuffer, pReadCache + dwReadCacheOffset1, nOnePacketSize);
		dwReadCacheOffset1 += nOnePacketSize;
		if (dwReadCacheOffset1 == dwReadCacheOffset2)
		{
			dwReadCacheOffset1 = 0;
			dwReadCacheOffset2 = 0;
		}
		nRet = nOnePacketSize;
	}
	else
	{
		nRet = usb_bulk_read(libusb_dev_handle, this->end_point_qt.btInEndPointAddress, (char*)pReadBuffer, READ_QT_BUFFER_SIZE, timeout);
	}
	if (nRet > 4)
	{
		//���ճ��
		int nOnePacketSize = *(int*)(pReadBuffer + 0);
		if (nOnePacketSize < nRet)
		{
			//2����һ���յ�
			//�ӵ�����
			memcpy(pReadCache + dwReadCacheOffset1, pReadBuffer + nOnePacketSize, nRet - nOnePacketSize);
			dwReadCacheOffset2 += (nRet - nOnePacketSize);
		}
		else if (nOnePacketSize > nRet)
		{
			//��������(TODO)
			//DebugBreak();
		}
		vecData.resize(nOnePacketSize);
		memcpy(vecData.data(), pReadBuffer, nOnePacketSize);
		//LOG_INFO("ReadQTBulk[%d] : ", nOnePacketSize);
		//IosDevice::PrintBytes(NULL, vecData);
		return nRet;
	}
	else
	{
		//LOG_INFO("ReadQTBulk fail ret[%d]", nRet);
		return nRet;
	}
}

bool QuickTime::ReceiveLoop()
{
	std::mutex _mutex;
	bool bError = false;
	int nReadNum;
	std::vector<unsigned char> vecData;
	unsigned int dwConnectTimeOutCount = 0; //���ӳ�ʱ������2�ξͱ�ʾʧ�ܣ���ʾ������

	//if (!this->DevicePairAndWait(8000))
	//{
	//	mLog("����ʧ�ܣ���������2");
	//	return false;
	//}

	std::string strDeviceName;
	RW::ExecProcess("Frameworks\\ideviceinfo.exe", std::string("-u ").append(this->serial_number).append(" -k DeviceName"), strDeviceName, -1);
	mLog("�豸�� : %s", strDeviceName.c_str());
	
	//���Ӧ����������������ÿ��ܵ������޶�ȡ��ʱ����Ҫ�����ֻ�����ʹ��
	//usb_control_msg(libusb_dev_handle, 0x40, 0x40, 0x6400, 0x6400, 0, 0, 1000);
	//SendPing(1000);

	while (this->CheckStatus())
	{
		nReadNum = ReadQTBulk(vecData, 1000);
		if (nReadNum == -116)
		{
			mLog("116");
			usb_control_msg(libusb_dev_handle, 0x40, 0x40, 0x6400, 0x6400, 0, 0, 1000);
			SendPing(1000);
			continue;
		}
		else if (nReadNum < 0)
		{
			mLog("�豸�ѶϿ�[%d]", nReadNum);
			mVideoDisConnect();
			break;
		}

		bError = false;

		_mutex.lock();
		if (vecData.size() > 8)
		{
			int nEventType = 0;
			memcpy(&nEventType, vecData.data() + 4, 4);
			switch (nEventType)
			{
			case PING:
				//�ظ�ping
				mLog("PING received");
				SendPing();
				break;
			case SyncPacketMagic:
				//mLog("SyncPacketMagic received");
				ReceiveSyncPacket(vecData);
				break;
			case AsynPacketMagic:
				//mLog("AsynPacketMagic received");
				ReceiveAsynPacket(vecData, &bError);
				break;
			default:
				//DebugBreak();
				mLog("ReceiveLoop unknown magic type:(0x%x)", nEventType);
				break;
			}
		}
		_mutex.unlock();

		if (bError)
		{
			mLog("ReceiveAsynPacket error");
			usb_reset(libusb_dev_handle);
			Sleep(1000);
			break;
		}
	}


	return true;
}

bool QuickTime::DevicePair()
{
	bool bRet;
	//std::string strValidate;
	//RW::ExecProcess("Frameworks\\idevicepair.exe", std::string("-u ").append(this->user_input_data.serial_number).append(" validate"), strValidate, -1);
	//if (strValidate.find("SUCCESS:") != std::string::npos)
	//{
	//	//"ERROR: Please accept the trust dialog on the screen of device 00008110-00060D5822B9801E, then attempt to pair again."
	//	//"SUCCESS: Validated pairing with device 00008110-00060D5822B9801E"
	//	mLog("������� strValidate[%s]", strValidate.c_str());
	//	return true;
	//}
	
	//std::string strPairList;
	//RW::ExecProcess("Frameworks\\idevicepair.exe", "list", strPairList, -1);
	//if (strPairList.find(this->user_input_data.serial_number) != std::string::npos)
	//{
	//	mLog("������� strPairList[%s]", strPairList.c_str());
	//	return true;
	//}
	
	//if (RW::isProcessExist("usbmuxd.exe") == false)
	//{
	//	//RW::ExitProcess("usbmuxd.exe");
	//	RW::ExecProcessDetached("Frameworks\\usbmuxd.exe", "");
	//	Sleep(2000);
	//}

	std::string strText;
	//RW::ExecProcess("Frameworks\\idevicepair.exe", std::string("-u ").append(this->user_input_data.serial_number).append(" unpair"), strText, -1);
	RW::ExecProcess("Frameworks\\idevicepair.exe", std::string("-u ").append(this->serial_number).append(" pair"), strText, -1);

	//mLog("���η����ı� : %s", strText.c_str());
	if (strText.find("SUCCESS: Paired with device") != std::string::npos)
	{
		mLog("���γɹ�");
		bRet = true;
	}
	else if (strText.find("ERROR: Please accept the trust dialog on the screen of device") != std::string::npos)
	{
		mLog("δ����(������Ļ�ϵ����ΰ�ť)");
		bRet = false;
	}
	else if (strText.find("because a passcode is set. Please enter the passcode on the device and retry.") != std::string::npos)
	{
		mLog("δ����(�������Ļ���������)");
		bRet = false;
	}
	else if (strText.find("said that the user denied the trust dialog") != std::string::npos)
	{
		mLog("δ����(����˲����Σ������������豸)");
		bRet = false;
	}
	else if (strText.find("No device found with udid") != std::string::npos)
	{
		mLog("δ����(δ�ҵ��豸)");
		bRet = false;
	}
	else
	{
		mLog("����ʧ��(δ֪����)");
		bRet = false;
	}

	return bRet;
}

bool QuickTime::DevicePairAndWait(DWORD dwTimeOut)
{
	ULONGLONG StartTime = GetTickCount64();
	while (this->CheckStatus())
	{
		if (this->DevicePair()) return true;
		Sleep(1000);
		if (dwTimeOut != -1 && GetTickCount64() - StartTime > dwTimeOut)
		{
			mLog("���γ�ʱ");
			return false;	//���γ�ʱ
		}
	}
	return false;
}

void QuickTime::ClearQTEndPointHalt()
{
	usb_clear_halt(libusb_dev_handle, this->end_point_qt.btOutEndPointAddress);
	usb_clear_halt(libusb_dev_handle, this->end_point_qt.btInEndPointAddress);
}

void QuickTime::ClearFeature()
{
	usb_control_msg(libusb_dev_handle, 0x02, 0x01, 0, this->end_point_qt.btOutEndPointAddress, 0, 0, 1000);
	usb_control_msg(libusb_dev_handle, 0x02, 0x01, 0, this->end_point_qt.btInEndPointAddress, 0, 0, 1000);
}

bool QuickTime::SetMuxConfigAndInterface()
{
	int ret;

	//if (this->end_point_qt.nInterfaceIndex)
	//{
	//	usb_release_interface(libusb_dev_handle, this->end_point_qt.nInterfaceIndex);
	//}

	ret = usb_set_configuration(libusb_dev_handle, this->end_point_mux.nConfigIndex);
	if (ret != USB_SUCCESS) return false;

	ret = usb_claim_interface(libusb_dev_handle, this->end_point_mux.nInterfaceIndex);
	if (ret != USB_SUCCESS) return false;

	return true;
}

bool QuickTime::SetQTConfigAndInterface()
{
	int ret;

	//if (this->end_point_mux.nInterfaceIndex)
	//{
	//	usb_release_interface(libusb_dev_handle, this->end_point_mux.nInterfaceIndex);
	//}

	ret = usb_set_configuration(libusb_dev_handle, this->end_point_qt.nConfigIndex);
	if (ret != USB_SUCCESS) return false;

	ret = usb_claim_interface(libusb_dev_handle, this->end_point_qt.nInterfaceIndex);
	if (ret != USB_SUCCESS) return false;

	return true;
}

bool QuickTime::EnableQTConfig()
{
	int ret = usb_control_msg(this->libusb_dev_handle, 0x40, 0x52, 0x00, 0x02, 0, 0, 1000);
	return ret == USB_SUCCESS;
}

bool QuickTime::DisableQTConfig()
{
	int ret = usb_control_msg(this->libusb_dev_handle, 0x40, 0x52, 0x00, 0x00, 0, 0, 1000);
	return ret == USB_SUCCESS;
}

bool QuickTime::GetInOutEndPoint(const struct usb_interface_descriptor* interface_desc, struct end_point_data* pEpData)
{
	if (!interface_desc || !pEpData) return false;
	pEpData->btInEndPointAddress = 0x00;
	pEpData->nInEndPointPacketSize = -1;
	pEpData->btOutEndPointAddress = 0x00;
	pEpData->nOutEndPointPacketSize = -1;
	for (int i = 0; i < interface_desc->bNumEndpoints; i++)
	{
		const struct usb_endpoint_descriptor* endpoint_desc = &interface_desc->endpoint[i];
		if (!endpoint_desc) continue;

		if ((endpoint_desc->bEndpointAddress & 0x80) != 0)
		{
			//in�˵�
			//���ܳ��ֶ���˵�,ȡwMaxPacketSize��Ķ˿�
			if (endpoint_desc->wMaxPacketSize > pEpData->nInEndPointPacketSize)
			{
				pEpData->btInEndPointAddress = endpoint_desc->bEndpointAddress;
				pEpData->nInEndPointPacketSize = endpoint_desc->wMaxPacketSize;
			}
		}
		else
		{
			//out�˵�
			//���ܳ��ֶ���˵�,ȡwMaxPacketSize��Ķ˿�
			if (endpoint_desc->wMaxPacketSize > pEpData->nOutEndPointPacketSize)
			{
				pEpData->btOutEndPointAddress = endpoint_desc->bEndpointAddress;
				pEpData->nOutEndPointPacketSize = endpoint_desc->wMaxPacketSize;
			}
		}
	}
	return true;
}

int QuickTime::FindInterfaceForSubclass(const struct usb_config_descriptor* config_desc, int nSubClass, struct end_point_data* pEpData)
{
	if (!config_desc || !pEpData) return -1;

	for (int i = 0; i < config_desc->bNumInterfaces; i++)
	{
		const struct usb_interface* tmp_interface = &config_desc->interface[i];
		//LOG_INFO("tmp_interface[%d][%p]", i, tmp_interface);
		for (int j = 0; j < tmp_interface->num_altsetting; j++)
		{
			const struct usb_interface_descriptor* tmp_interface_desc = &tmp_interface->altsetting[j];
			if (tmp_interface_desc->bLength != 0)
			{
				if (tmp_interface_desc->bInterfaceClass == USB_CLASS_VENDOR_SPEC
					&& tmp_interface_desc->bInterfaceSubClass == nSubClass)
				{
					GetInOutEndPoint(tmp_interface_desc, pEpData);
					return tmp_interface_desc->bInterfaceNumber;
				}
			}
		}
	}
	return -1;
}

bool QuickTime::FindConfigurations(const struct usb_device* libusb_dev, struct end_point_data* pMuxEpData, struct end_point_data* pQtEpData)
{
	int muxInterfaceIndex = -1;
	int qtInterfaceIndex = -1;
	if (!libusb_dev || !libusb_dev->config) return false;

	pMuxEpData->nConfigIndex = -1;
	pMuxEpData->nInterfaceIndex = -1;
	pQtEpData->nConfigIndex = -1;
	pQtEpData->nInterfaceIndex = -1;

	for (int i = 0; i < libusb_dev->descriptor.bNumConfigurations; i++)
	{
		const struct usb_config_descriptor* config_descriptor = &libusb_dev->config[i];
		muxInterfaceIndex = FindInterfaceForSubclass(config_descriptor, 0xFE, pMuxEpData);
		if (muxInterfaceIndex != -1)
		{
			pMuxEpData->nConfigIndex = config_descriptor->bConfigurationValue;
			pMuxEpData->nInterfaceIndex = muxInterfaceIndex;
			mLog("Found MuxConfig[%d]  Interface[%d],for Device %s", pMuxEpData->nConfigIndex, pMuxEpData->nInterfaceIndex, libusb_dev->filename);
		}
		qtInterfaceIndex = FindInterfaceForSubclass(config_descriptor, 0x2A, pQtEpData);
		if (qtInterfaceIndex != -1)
		{
			pQtEpData->nConfigIndex = config_descriptor->bConfigurationValue;
			pQtEpData->nInterfaceIndex = qtInterfaceIndex;
			mLog("Found QtConfig[%d]  Interface[%d],for Device %s", pQtEpData->nConfigIndex, pQtEpData->nInterfaceIndex, libusb_dev->filename);
		}

		config_descriptor = nullptr;
	}
	return false;
}

bool QuickTime::IsValidIosDeviceWithActiveQTConfig()
{
	FindConfigurations(this->libusb_dev, &this->end_point_mux, &this->end_point_qt);
	return this->end_point_qt.nConfigIndex != -1 && this->end_point_qt.nInterfaceIndex != -1;
}

quick_time_error_t QuickTime::OpenDeviceBySerial(const std::string& strSerialNumber)
{
	int ret;
	struct usb_bus* bus;
	struct usb_device* dev;
	struct usb_dev_handle* dev_handle = NULL;

	assert(QuickTime::isInit());

	usb_init();
	usb_find_busses();
	usb_find_devices();
	for (bus = usb_get_busses(); bus; bus = bus->next)
	{
		for (dev = bus->devices; dev; dev = dev->next)
		{
			if (dev->descriptor.idVendor != VID_APPLE) continue;
			dev_handle = usb_open(dev);
			if (!dev_handle) continue;

			char szSerialNumber[256];
			ret = usb_get_string_simple(dev_handle, dev->descriptor.iSerialNumber, szSerialNumber, sizeof(szSerialNumber));  //ecf4cd291449dae04f252573589ddc1d4733d118
			if (ret < 0) continue;

			printf("szSerialNumber : %s\n", szSerialNumber);
			std::string strDeviceSerialNumber = szSerialNumber;

			//serial 24 to 25("0000211000060D5822B98012" to "00002110-00060D5822B98012")
			if (strDeviceSerialNumber.length() == 24
				&& strDeviceSerialNumber.find("&") == std::string::npos
				&& strDeviceSerialNumber.find("-") == std::string::npos)
			{
				strDeviceSerialNumber.insert(8, "-");
				RW::StrToUpper(strDeviceSerialNumber);
			}
			else
			{
				//serial to lower
				RW::StrToLower(strDeviceSerialNumber);
			}

			if (strSerialNumber == strDeviceSerialNumber)
			{
				this->libusb_dev = dev;
				this->libusb_dev_handle = dev_handle;
				return QT_E_SUCCESS;
			}
			usb_close(dev_handle);
		}
	}
	return QT_E_LIBUSB_OPEN_FAIL;
}

void QuickTime::Close()
{
	if (this->libusb_dev_handle)
	{
		usb_close(this->libusb_dev_handle);
		this->libusb_dev_handle = NULL;
	}
	if (this->libusb_dev)
	{
		this->libusb_dev = NULL;
	}
}

void QuickTime::Clear()
{
	this->Close();

	this->qwNeedClockRef = 0;
	this->vecNeedMessage.clear();  //�����need�ظ�����
	this->bFirstAudioTimeTaken = false;
	this->StartTimeDeviceAudioMillSecond = 0;             //����
	this->StartTimeLocalAudioMillSecond = 0;              //����
	this->LastEatFrameReceivedDeviceAudioMillSecond = 0;  //����
	this->LastEatFrameReceivedLocalAudioMillSecond = 0;   //����
	this->deviceAudioClockRef = 0;//�豸��Ƶʱ��ID
	this->bFirstProcessAudio = true;
	this->bFirstProcessVideo = true;
	this->bSendSPS_and_PPS = false;
	this->bSendAudioHead = false;
	this->bConnect = false;
	this->clok_Clock.SetID(0);
	this->localAudioClock.SetID(0);


	this->end_point_mux.nConfigIndex = -1;				//config index
	this->end_point_mux.nInterfaceIndex = -1;			//interface index
	this->end_point_mux.btInEndPointAddress = 0x00;		//in  ep address ( >0x80 )
	this->end_point_mux.nInEndPointPacketSize = -1;		//in  ep packet size
	this->end_point_mux.btOutEndPointAddress = 0x00;	//out ep address ( <0x80 )
	this->end_point_mux.nOutEndPointPacketSize = -1;	//out ep packet size

	this->end_point_qt.nConfigIndex = -1;				//config index
	this->end_point_qt.nInterfaceIndex = -1;			//interface index
	this->end_point_qt.btInEndPointAddress = 0x00;		//in  ep address ( >0x80 )
	this->end_point_qt.nInEndPointPacketSize = -1;		//in  ep packet size
	this->end_point_qt.btOutEndPointAddress = 0x00;		//out ep address ( <0x80 )
	this->end_point_qt.nOutEndPointPacketSize = -1;		//out ep packet size

	this->dwReadCacheOffset1 = 0;
	this->dwReadCacheOffset2 = 0;
}

bool QuickTime::CheckStatus()
{
	switch (this->running_status)
	{
	case EM_RUN_STATE_STOP:			return false;	//ֹͣ
	case EM_RUN_STATE_RUNNING:		return true;	//��������
	case EM_RUN_STATE_READY_STOP:	return false;	//ֹ֪ͨͣ
	default: break;
	}

	this->mLog("�ڲ�����,δ֪������״̬[%d]\n", this->running_status);
	return false;
}








void QuickTime::mAudioInit(int bits, int channels, int samplerate, int isaudio)
{
	if (!QuickTime::g_FuncCallback.audio_init) return;
	QuickTime::g_FuncCallback.audio_init(this->user_input_data, bits, channels, samplerate, isaudio);
}

void QuickTime::mAudioProcess(unsigned char* buffer, int length)
{
	if (!QuickTime::g_FuncCallback.audio_process) return;
	QuickTime::g_FuncCallback.audio_process(this->user_input_data, buffer, length);
}

void QuickTime::mVideoConnect()
{
	if (!QuickTime::g_FuncCallback.video_connect) return;
	QuickTime::g_FuncCallback.video_connect(this->user_input_data);
}

void QuickTime::mVideoDisConnect()
{
	if (!QuickTime::g_FuncCallback.video_dis_connect) return;
	QuickTime::g_FuncCallback.video_dis_connect(this->user_input_data);
}

void QuickTime::mVideoH264Extradata(unsigned char* buffer, unsigned int length)
{
	if (!QuickTime::g_FuncCallback.video_h264_extradata) return;
	QuickTime::g_FuncCallback.video_h264_extradata(this->user_input_data, buffer, length);
}

void QuickTime::mVideoH264Slice(unsigned char* buffer, unsigned int length)
{
	if (!QuickTime::g_FuncCallback.video_h264_slice) return;
	QuickTime::g_FuncCallback.video_h264_slice(this->user_input_data, buffer, length);
}

void QuickTime::mMessage(const char* fmt, ...)
{
	if (!QuickTime::g_FuncCallback.message) return;

	char buffer[2048] = { 0 };
	va_list args;
	va_start(args, fmt);
	vsprintf_s(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	QuickTime::g_FuncCallback.message(this->user_input_data, buffer);
}

void QuickTime::mLog(const char* fmt, ...)
{
	if (!QuickTime::g_FuncCallback.log) return;

	char buffer[2048] = { 0 };
	va_list args;
	va_start(args, fmt);
	vsprintf_s(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	QuickTime::g_FuncCallback.log(this->user_input_data, buffer);
}


DWORD WINAPI QuickTime::cast_screen_thread(void* args)
{
	assert(args);
	srand((unsigned int)time(NULL));
	QuickTime* quick_time = (QuickTime*)args;
	quick_time->SyncExec();
	return 0;
}

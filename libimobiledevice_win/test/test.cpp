
#include <stdio.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>

#include <windows.h>
#include <iostream>
#include <vector>
#include <Setupapi.h>
#pragma comment(lib,"Setupapi.lib")
#include <newdev.h>
#pragma comment(lib,"Newdev.lib")
#include <cfgmgr32.h>
#pragma comment(lib,"cfgmgr32.lib")

#define OUT_ENDPOINT 0x03
#define IN_ENDPOINT  0x83

typedef struct AdfuDeviceInfo
{
    unsigned short vid;         //01-02 vid
    unsigned short pid;         //03-04 pid
    DWORD Unknowm04_08;         //04-08 未知 000001A0
    char szNONC_SNON[0xFF];     //0x08  - 0x106(\0终止) NONC:XXXXXX SNON:XXXXXX
    char szEcidData[0xFF];      //0x107 - 0x205(\0终止) SDOM:01 CPID:8120 CPRV:11 CPFM:03 SCEP:01 BDID:0E ECID:XXXXXX IBFL:3C SIKA:00 SRTG:[iBoot-7195.0.0.300.25]
    char szManufacturer[0xFF];  //0x206 - 0x304(\0终止) Apple Inc.
    char szDeviceName[0xFF];    //0x305 - 0x404(\0终止) Apple Mobile Device (DFU Mode)
}AdfuDeviceInfo;


int libusb_get14info()
{
	int r;
	libusb_device** devs = nullptr;

	r = libusb_init(NULL);
	if (r < 0)
		return r;

	libusb_device_handle* dev_hd = libusb_open_device_with_vid_pid(NULL, 0x05AC, 0x1227);
	if (!dev_hd)
	{
		printf("libusb 打开设备失败\n");
		system("pause");
		return 0;
	}

	libusb_reset_device(dev_hd);
	libusb_set_configuration(dev_hd, 1);
	libusb_claim_interface(dev_hd, 0);
	libusb_clear_halt(dev_hd, OUT_ENDPOINT);
	libusb_clear_halt(dev_hd, IN_ENDPOINT);


	unsigned char data[] = { 0x01, 0x10, 0xa0, 0x10, 0x00, 0x00, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00 };
	int bytes = 0;
	r = libusb_bulk_transfer(dev_hd, OUT_ENDPOINT, data, sizeof(data), &bytes, 2000);
	printf("send r : %d  %d\n", r, bytes);
	
	
	unsigned char data_res[2048] = { 0 };
	int bytes_res = 0;
	r = libusb_bulk_transfer(dev_hd, IN_ENDPOINT, data_res, sizeof(data_res), &bytes_res, 2000);
	printf("read r : %d  %d\n", r, bytes_res);
	wprintf(L"read : %s\n", (const wchar_t*)data_res);


	libusb_close(dev_hd);
	system("pause");
	return 0;
}


//取所有ADFU设备路径--05AC-0001
std::vector<std::string> GetAdfuDevicePaths()
{
    static const GUID GUID_DEVINTERFACE_ADFU = { 0xB36f4137, 0xF4EF, 0x4BFC, {0xA2, 0x5A, 0xC2, 0x41, 0x07, 0x68, 0xEE, 0x37} };
    std::vector<std::string> vecPaths;
    HDEVINFO DeviceInfoSet;
    DWORD MemberIndex;
    struct _SP_DEVINFO_DATA DeviceInfoData;
    SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
    DWORD dwRequiredSize;

    DeviceInfoSet = SetupDiGetClassDevsW(&GUID_DEVINTERFACE_ADFU, 0i64, 0i64, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (DeviceInfoSet == (HDEVINFO)-1)
    {
        printf("SetupDiGetClassDevs() failed\n");
        return vecPaths;
    }
    DeviceInfoData.cbSize = sizeof(struct _SP_DEVINFO_DATA);

    for (MemberIndex = 0; SetupDiEnumDeviceInfo(DeviceInfoSet, MemberIndex, &DeviceInfoData); ++MemberIndex)
    {
        DeviceInterfaceData.cbSize = sizeof(struct _SP_DEVICE_INTERFACE_DATA);
        if (!SetupDiEnumDeviceInterfaces(DeviceInfoSet, NULL, &GUID_DEVINTERFACE_ADFU, MemberIndex, &DeviceInterfaceData))
        {
            printf("SetupDiEnumDeviceInterfaces() failed.  lasterror[%d]\n", GetLastError());
            continue;
        }
        //获取缓冲区大小
        dwRequiredSize = 0;
        SetupDiGetDeviceInterfaceDetailA(DeviceInfoSet, &DeviceInterfaceData, NULL, 0, &dwRequiredSize, NULL);
        //申请内存
        SP_DEVICE_INTERFACE_DETAIL_DATA_A* pDeviceInterfaceDetailData = (SP_DEVICE_INTERFACE_DETAIL_DATA_A*)malloc(dwRequiredSize);
        if (!pDeviceInterfaceDetailData) break;
        pDeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
        //获取数据
        if (!SetupDiGetDeviceInterfaceDetailA(DeviceInfoSet, &DeviceInterfaceData, pDeviceInterfaceDetailData, dwRequiredSize, NULL, NULL))
        {
            printf("SetupDiGetDeviceInterfaceDetailA failed.  lasterror[%d]\n", GetLastError());
        }
        //printf("DevicePath : %s\n", pDeviceInterfaceDetailData->DevicePath);
        //加入列表
        vecPaths.push_back(pDeviceInterfaceDetailData->DevicePath);
        //释放内存
        free(pDeviceInterfaceDetailData);
    }
    SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    return vecPaths;
}

static void my_copy_nonce_with_tag(const char* descriptor, const char* tag, unsigned char** nonce, unsigned int* nonce_size)
{
	if (!tag) return;
	*nonce = NULL;
	*nonce_size = 0;
	int taglen = strlen(tag);
	int nlen = 0;
	const char* nonce_string = NULL;
	const char* p = descriptor;
	const char* colon = NULL;
	do {
		colon = strchr(p, ':');
		if (!colon)
			break;
		if (colon - taglen < p) {
			break;
		}
		const char* space = strchr(colon, ' ');
		if (strncmp(colon - taglen, tag, taglen) == 0) {
			p = colon + 1;
			if (!space) {
				nlen = strlen(p);
			}
			else {
				nlen = space - p;
			}
			nonce_string = p;
			nlen /= 2;
			break;
		}
		else {
			if (!space) {
				break;
			}
			else {
				p = space + 1;
			}
		}
	} while (colon);

	if (nlen == 0) {
		//debug("%s: WARNING: couldn't find tag %s in string %s\n", __func__, tag, buf);
		return;
	}

	unsigned char* nn = (unsigned char*)malloc(nlen);
	if (!nn) {
		return;
	}

	int i = 0;
	for (i = 0; i < nlen; i++) {
		int val = 0;
		if (sscanf(nonce_string + (i * 2), "%02X", &val) == 1) {
			nn[i] = (unsigned char)val;
		}
		else {
			//debug("%s: ERROR: unexpected data in nonce result (%2s)\n", __func__, nonce_string + (i * 2));
			break;
		}
	}

	if (i != nlen) {
		//debug("%s: ERROR: unable to parse nonce\n", __func__);
		free(nn);
		return;
	}

	*nonce = nn;
	*nonce_size = nlen;
}

int adfu_send_buffer(HANDLE hd, const void* buffer, unsigned int length)
{
	unsigned int total = 0;
	unsigned char* packadfu = (unsigned char*)malloc(0x4010);
	if (!packadfu) return -1;
	while (total < length)
	{
		unsigned int one_length = length - total > 0x4000 ? 0x4000 : length - total;
		memset(packadfu, 0, 0x4010);
		memcpy(packadfu, (unsigned char*)buffer + total, one_length);
		*(unsigned long long*)(packadfu + 0x4000) = (unsigned long long)one_length;
		*(unsigned long long*)(packadfu + 0x4008) = (unsigned long long)total;
		if (!DeviceIoControl(hd, 0x220008, packadfu, 0x4010, 0, 0, 0, 0)) 
		{
			free(packadfu);
			return -2;
		}
		total += one_length;
	}
	if (!DeviceIoControl(hd, 0x22000C, &length, 4, 0, 0, 0, 0))
	{
		free(packadfu);
		return -3;
	}
	free(packadfu);
	return 0;
}

UINT EnumeAllDeviceByCM()
{
	DEVINST     devInst = { 0 };
	DEVINST     devInstNext = { 0 };
	CONFIGRET   cr = { 0 };
	ULONG       walkDone = 0;
	ULONG       len = 0;
	char		buf[1024] = { 0 };

	int CM_PROPERTY[] = { 
	CM_DRP_DEVICEDESC,
	CM_DRP_HARDWAREID,CM_DRP_BUSNUMBER
	};
	cr = CM_Locate_DevNode(&devInst, NULL, 0);
	// 可以指定根设备ID
	//cr = CM_Locate_DevNode(&devInst, "PCI\\VEN_8086&DEV_9D2F&SUBSYS_17091D72&REV_21\\3&11583659&1&A0", 0);
	if (cr != CR_SUCCESS)
	{
		return 1;
	}

	// Do a depth first search for the DevNode with a matching
	// DriverName value
	while (!walkDone)
	{
		char szDeviceID[1024] = { 0 };
		char szProperty[1024] = { 0 };

		len = 1024;
		cr = CM_Get_Device_ID(devInst, szDeviceID, len, 0);
		
		if (cr == CR_SUCCESS)
		{
			for (UINT i = 0; i < _countof(CM_PROPERTY); i++)
			{
				len = 1024;

				cr = CM_Get_DevNode_Registry_Property(devInst,
					CM_PROPERTY[i],
					NULL,
					szProperty,
					&len,
					0);
				if (cr == CR_SUCCESS)
				{
					printf("%x : %s\n", CM_PROPERTY[i], szProperty);
				}
			}
		}
		else
		{
			return 1;
		}

		// This DevNode didn't match, go down a level to the first child.
		cr = CM_Get_Child(&devInstNext,
			devInst,
			0);

		if (cr == CR_SUCCESS)
		{
			devInst = devInstNext;
			continue;
		}

		// Can't go down any further, go across to the next sibling.  If
		// there are no more siblings, go back up until there is a sibling.
		// If we can't go up any further, we're back at the root and we're
		// done.
		for (;;)
		{
			cr = CM_Get_Sibling(&devInstNext, devInst, 0);
			if (cr == CR_SUCCESS)
			{
				devInst = devInstNext;
				break;
			}

			cr = CM_Get_Parent(&devInstNext, devInst, 0);
			if (cr == CR_SUCCESS)
			{
				devInst = devInstNext;
			}
			else
			{
				walkDone = 1;
				break;
			}
		}
	}

	return 0;
}



//取所有ADFU设备路径--05AC-1881
std::vector<std::string> GetaaaAdfuDevicePaths()
{
	static const GUID GUID_DEVINTERFACE_USB_DEVICE = { 0xA5DCBF10L, 0x6530, 0x11D2, { 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED } };

	std::vector<std::string> vecPaths;
	HDEVINFO DeviceInfoSet;
	DWORD MemberIndex;
	struct _SP_DEVINFO_DATA DeviceInfoData;
	SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
	DWORD dwRequiredSize;

	DeviceInfoSet = SetupDiGetClassDevsW(&GUID_DEVINTERFACE_USB_DEVICE, 0i64, 0i64, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (DeviceInfoSet == (HDEVINFO)-1)
	{
		printf("SetupDiGetClassDevs() failed\n");
		return vecPaths;
	}
	DeviceInfoData.cbSize = sizeof(struct _SP_DEVINFO_DATA);

	for (MemberIndex = 0; SetupDiEnumDeviceInfo(DeviceInfoSet, MemberIndex, &DeviceInfoData); ++MemberIndex)
	{
		char szDeviceInstanceId[256];
		SetupDiGetDeviceInstanceIdA(DeviceInfoSet, &DeviceInfoData, szDeviceInstanceId, sizeof(szDeviceInstanceId), NULL);
		printf("szDeviceInstanceId : %s\n", szDeviceInstanceId);

		if (strstr(szDeviceInstanceId, "USB\\VID_05AC&PID_1881"))
		{
			DEVINST devInst = { 0 };
			DEVINST devChildInst = { 0 };
			ULONG len;
			CONFIGRET cret = CM_Locate_DevInstA(&devInst, szDeviceInstanceId, 0);
			if (cret == CR_SUCCESS)
			{
				cret = CM_Get_Child(&devChildInst, devInst, 0);
				if (cret == CR_SUCCESS)
				{
					while (true)
					{
						char szDeviceID[1024] = { 0 };
						len = sizeof(szDeviceID);
						cret = CM_Get_Device_ID(devChildInst, szDeviceID, len, 0);
						if (cret == CR_SUCCESS)
						{
							printf("Child szDeviceID == %s\n", szDeviceID);
						}

						char szProperty[1024] = { 0 };
						len = sizeof(szProperty);
						cret = CM_Get_DevNode_Registry_Property(devChildInst, CM_DRP_DEVICEDESC, NULL, szProperty, &len, 0);
						if (cret == CR_SUCCESS)
						{
							printf("Child DEVICEDESC == %s\n", szProperty);
						}
						cret = CM_Get_Sibling(&devChildInst, devChildInst, 0);
						if (cret != CR_SUCCESS)
						{
							break;
						}
					}
				}
			}
		}


		DeviceInterfaceData.cbSize = sizeof(struct _SP_DEVICE_INTERFACE_DATA);
		if (!SetupDiEnumDeviceInterfaces(DeviceInfoSet, NULL, &GUID_DEVINTERFACE_USB_DEVICE, MemberIndex, &DeviceInterfaceData))
		{
			printf("SetupDiEnumDeviceInterfaces() failed.  lasterror[%d]\n", GetLastError());
			continue;
		}
		//获取缓冲区大小
		dwRequiredSize = 0;
		SetupDiGetDeviceInterfaceDetailA(DeviceInfoSet, &DeviceInterfaceData, NULL, 0, &dwRequiredSize, NULL);
		//申请内存
		SP_DEVICE_INTERFACE_DETAIL_DATA_A* pDeviceInterfaceDetailData = (SP_DEVICE_INTERFACE_DETAIL_DATA_A*)malloc(dwRequiredSize);
		if (!pDeviceInterfaceDetailData) break;
		pDeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
		//获取数据
		if (!SetupDiGetDeviceInterfaceDetailA(DeviceInfoSet, &DeviceInterfaceData, pDeviceInterfaceDetailData, dwRequiredSize, NULL, NULL))
		{
			printf("SetupDiGetDeviceInterfaceDetailA failed.  lasterror[%d]\n", GetLastError());
		}
		if (strstr(pDeviceInterfaceDetailData->DevicePath, "usb#vid_05ac&pid_1881"))
		{
			printf("DevicePath : %s\n", pDeviceInterfaceDetailData->DevicePath);
			//加入列表
			vecPaths.push_back(pDeviceInterfaceDetailData->DevicePath);
		}


		//释放内存
		free(pDeviceInterfaceDetailData);
	}
	SetupDiDestroyDeviceInfoList(DeviceInfoSet);
	return vecPaths;
}

int main()
{
#if 0
    //libusb_get14info();
    std::vector<std::string> vecPaths = GetAdfuDevicePaths();
    printf("ADFU设备数量 : %u\n", (DWORD)vecPaths.size());
    for (size_t i = 0; i < vecPaths.size(); i++)
    {
        const std::string& cstr_DevicePath = vecPaths[i];
        printf("%s\n", cstr_DevicePath.c_str());
        //打开
        HANDLE hDevice = CreateFileA(cstr_DevicePath.c_str(),
            FILE_FLAG_WRITE_THROUGH | FILE_FLAG_OVERLAPPED,
            FILE_SHARE_WRITE | FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        if (hDevice == INVALID_HANDLE_VALUE)
        {
            printf("打开设备失败 : %s, lasterror : %d\n", cstr_DevicePath.c_str(), GetLastError());
            continue;
        }
        //读取信息
        AdfuDeviceInfo adfuDeviceInfo;
        DWORD dwDytesReturn = 0;
        memset(&adfuDeviceInfo, 0, sizeof(AdfuDeviceInfo));
        if (!DeviceIoControl(hDevice, 0x220004, NULL, NULL, &adfuDeviceInfo, sizeof(AdfuDeviceInfo), &dwDytesReturn, NULL))
        {
            printf("读取信息失败 : %s, lasterror : %d\n", cstr_DevicePath.c_str(), GetLastError());
            continue;
        }
        printf("读取信息成功 : %s\n", cstr_DevicePath.c_str());
        printf("vid : 0x%04X\n", adfuDeviceInfo.vid);
        printf("pid : 0x%04X\n", adfuDeviceInfo.pid);
        printf("Unknowm04_08 : 0x%08X\n", adfuDeviceInfo.Unknowm04_08);
        printf("szNONC_SNON : %s\n", adfuDeviceInfo.szNONC_SNON);

		unsigned char* szNonc = NULL;
		unsigned int dwNoncSize = 0;
		my_copy_nonce_with_tag(adfuDeviceInfo.szNONC_SNON, "NONC", &szNonc, &dwNoncSize);

		unsigned char* szSnon = NULL;
		unsigned int dwSnonSize = 0;
		my_copy_nonce_with_tag(adfuDeviceInfo.szNONC_SNON, "SNON", &szSnon, &dwSnonSize);

        printf("szEcidData : %s\n", adfuDeviceInfo.szEcidData);
        printf("szManufacturer : %s\n", adfuDeviceInfo.szManufacturer);
        printf("szDeviceName : %s\n", adfuDeviceInfo.szDeviceName);

		//测试DFU发送BUFF
		int send_ret = dfu_send_buffer(hDevice, "echo 123", strlen("echo 123") + 1);
		printf("send_ret : %d\n", send_ret);

        //关闭设备
        CloseHandle(hDevice);
    }
#endif

	//GetaaaAdfuDevicePaths();


	printf("\n");
	system("pause");
	return 0;
}


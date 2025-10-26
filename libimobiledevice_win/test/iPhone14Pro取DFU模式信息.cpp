
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


//取所有ADFU设备路径
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

int main()
{
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
        printf("szEcidData : %s\n", adfuDeviceInfo.szEcidData);
        printf("szManufacturer : %s\n", adfuDeviceInfo.szManufacturer);
        printf("szDeviceName : %s\n", adfuDeviceInfo.szDeviceName);
        //关闭设备
        CloseHandle(hDevice);
    }


	system("pause");
	return 0;
}


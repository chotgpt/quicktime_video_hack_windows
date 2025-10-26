#include "gcusb.h"
#include "RW.h"
#include "libwdi.h"

#include <windows.h>
#include <iostream>
#include <stdio.h>
#include <SetupAPI.h>
#include <newdev.h>
#include <assert.h>
#include <algorithm>
#pragma comment(lib,"SetupAPI.lib")
#pragma comment(lib,"Newdev.lib")
#pragma comment(lib,"Advapi32.lib")
#pragma comment(lib,"User32.lib")
#pragma comment(lib,"Ole32.lib")


//解析VID PID 序列号
static bool SplitDeviceID(std::string strDeviceID, UsbDeviceData& usbData)
{
    std::string str_vid_pid_mi;
    std::string str_serial;

    std::string str_vid;
    std::string str_pid;
    std::string str_mi;

    usbData.isCompositeParent = true;
    usbData.isComposite = false;
    usbData.strSerial.clear();
    usbData.strVid.clear();
    usbData.strPid.clear();
    usbData.vid = 0;
    usbData.pid = 0;

    //USB\VID_0C45&PID_6613&MI_01\6&29D7F215&0&0001
    //USB\VID_05AC&PID_12A8\0000811000060D5822B9801E
    std::vector<std::string> TextList = RW::split(strDeviceID, "\\");
    if (TextList.size() < 3) return false;
    str_vid_pid_mi = TextList[1];   //"VID_0C45&PID_6613&MI_01" or "VID_05AC&PID_12A8"
    str_serial = TextList[2];       //"6&29D7F215&0&0001" or "0000811000060D5822B9801E"
    usbData.strSerial = str_serial;

    std::vector<std::string> vec_vid_pid_mi = RW::split(str_vid_pid_mi, "&");
    if (vec_vid_pid_mi.size() < 2) return false;
    str_vid = vec_vid_pid_mi[0];    //VID_0C45
    str_vid = str_vid.substr(4);    //0C45
    str_pid = vec_vid_pid_mi[1];    //PID_6613
    str_pid = str_pid.substr(4);    //6613
    if (vec_vid_pid_mi.size() > 2)
    {
        str_mi = vec_vid_pid_mi[2]; //MI_01
        if (str_mi.find("MI_") != -1)
        {
            usbData.isComposite = true; //表示是复合设备的子节点
            usbData.isCompositeParent = false;
        }
    }
    usbData.strVid = str_vid;
    usbData.strPid = str_pid;
    usbData.vid = (unsigned short)RW::hexstr_to_long(str_vid);
    usbData.pid = (unsigned short)RW::hexstr_to_long(str_pid);
    return true;
}

static std::string GetHdevInfoString(HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData, DWORD Property)
{
    DWORD dataType = 0;
    DWORD buffSize = 0;
    std::string strRet;
    SetupDiGetDeviceRegistryPropertyA(DeviceInfoSet, DeviceInfoData, Property, &dataType, NULL, buffSize, &buffSize);
    unsigned char* szBuffer = (unsigned char*)calloc(1, buffSize + 1);
    if (!szBuffer) return strRet;
    if (SetupDiGetDeviceRegistryPropertyA(DeviceInfoSet, DeviceInfoData, Property, &dataType, (PBYTE)szBuffer, buffSize, &buffSize))
    {
        strRet = (const char*)szBuffer;
    }
    free(szBuffer);
    return strRet;
}

std::vector<UsbDeviceData> GCUSB::GetUsbDeviceList()
{
    UsbDeviceData usbData;
    std::vector<UsbDeviceData> AndroidList;
    HDEVINFO hDevInfo = SetupDiGetClassDevsA(NULL, "USB", NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);
    SP_DEVINFO_DATA deviceInfoData;
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        SetupDiDestroyDeviceInfoList(hDevInfo);
        printf("SetupDiGetClassDevs failed\n");
        return AndroidList;
    }

    //对USB设备集进行枚举
    for (int i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &deviceInfoData); i++)
    {
        //vid pid 序列号 是否复合设备
        DWORD dwTemp = 0;
        char szDeviceID[1024] = { 0 };
        SetupDiGetDeviceInstanceIdA(hDevInfo, &deviceInfoData, szDeviceID, sizeof(szDeviceID), &dwTemp);
        //USB\VID_0C45&PID_6613&MI_01\6&29D7F215&0&0001
        //USB\VID_05AC&PID_12A8\0000811000060D5822B9801E
        SplitDeviceID(szDeviceID, usbData);

        //serial 24 to 25("0000211000060D5822B98012" to "00002110-00060D5822B98012")
        if (usbData.strSerial.length() == 24
            && usbData.strSerial.find("&") == std::string::npos
            && usbData.strSerial.find("-") == std::string::npos)
        {
            usbData.strSerial.insert(8, "-");
            RW::StrToUpper(usbData.strSerial);
        }
        else
        {
            //serial to lower
            RW::StrToLower(usbData.strSerial);
        }
        
        //驱动名
        usbData.strDriveName = GetHdevInfoString(hDevInfo, &deviceInfoData, SPDRP_SERVICE);
        //设备名
        usbData.strDeviceName = GetHdevInfoString(hDevInfo, &deviceInfoData, SPDRP_DEVICEDESC);
        //位置路径
        usbData.strLocationPath = GetHdevInfoString(hDevInfo, &deviceInfoData, SPDRP_LOCATION_PATHS);
        //位置信息
        usbData.strLocationInfo = GetHdevInfoString(hDevInfo, &deviceInfoData, SPDRP_LOCATION_INFORMATION);
        //兼容ID
        usbData.strCompatibleID = GetHdevInfoString(hDevInfo, &deviceInfoData, SPDRP_COMPATIBLEIDS);

        AndroidList.push_back(usbData);
    }
    SetupDiDestroyDeviceInfoList(hDevInfo);
    return AndroidList;
}

std::vector<UsbDeviceData> GCUSB::GetUsbDeviceList_ByVidAndComposite(unsigned short vid, bool isCompositeParent)
{
    std::vector<UsbDeviceData> NewUsbDeviceDataList;
    std::vector<UsbDeviceData> UsbDeviceDataList = GCUSB::GetUsbDeviceList();
    for (unsigned int i = 0; i < (unsigned int)UsbDeviceDataList.size(); i++)
    {
        const UsbDeviceData& device = UsbDeviceDataList[i];
        if (device.vid != vid)continue;
        if (device.isCompositeParent != isCompositeParent)continue;
        NewUsbDeviceDataList.push_back(device);
    }
    return NewUsbDeviceDataList;
}

bool GCUSB::InstallDriver(unsigned short vid, unsigned short pid, std::string strSerial, const std::string& crstrDriverType)
{
    assert(vid != 0);
    assert(pid != 0);

    bool install_driver_success = false;
    int ret;
    struct wdi_options_create_list options;
    options.list_all = TRUE;
    options.list_hubs = TRUE;
    options.trim_whitespaces = TRUE;
    transform(strSerial.begin(), strSerial.end(), strSerial.begin(), ::toupper);    //转大写

    ret = wdi_set_log_level(WDI_LOG_LEVEL_WARNING);

    struct wdi_device_info* wdi_dev_list = NULL;
    ret = wdi_create_list(&wdi_dev_list, &options);
    if (ret != WDI_SUCCESS || !wdi_dev_list) return false;

    struct wdi_device_info* wdi_dev_info;
    for (wdi_dev_info = wdi_dev_list; wdi_dev_info != NULL; wdi_dev_info = wdi_dev_info->next)
    {
        //if (wdi_dev_info->is_composite) continue;
        if (wdi_dev_info->vid != vid) continue;
        if (wdi_dev_info->pid != pid) continue;

        char* szTmpSerial = strrchr(wdi_dev_info->device_id, '\\') + 1;
        if (!szTmpSerial) continue;

        std::string strTmpSerial = szTmpSerial;
        transform(strTmpSerial.begin(), strTmpSerial.end(), strTmpSerial.begin(), ::toupper);    //转大写

        //serial 24 to 25("0000211000060D5822B98012" to "00002110-00060D5822B98012")
        if (strTmpSerial.length() == 24
            && strTmpSerial.find("&") == std::string::npos
            && strTmpSerial.find("-") == std::string::npos)
        {
            strTmpSerial.insert(8, "-");
            RW::StrToUpper(strTmpSerial);
        }
        else
        {
            //serial to lower
            RW::StrToLower(strTmpSerial);
        }

        if (strSerial != strTmpSerial && strSerial != szTmpSerial) continue;


        printf("desc[%s]\n", wdi_dev_info->desc);
        printf("driver[%s]\n", wdi_dev_info->driver);
        printf("device_id[%s]\n", wdi_dev_info->device_id);		//USB\VID_05AC&PID_12A8\6F5284A32B35F9739FE368CA471FBA818700030C
        printf("hardware_id[%s]\n", wdi_dev_info->hardware_id);
        printf("compatible_id[%s]\n", wdi_dev_info->compatible_id);
        printf("upper_filter[%s]\n", wdi_dev_info->upper_filter);
        printf("lower_filter[%s]\n", wdi_dev_info->lower_filter);
        printf("\n");

        if ("LIBUSB0_FILTER" == crstrDriverType)
        {
            if (
                 (wdi_dev_info->upper_filter && strcmp(wdi_dev_info->upper_filter, "libusb0") == 0)
                 ||
                 (wdi_dev_info->lower_filter && strcmp(wdi_dev_info->lower_filter, "libusb0") == 0)
               )
            {
                printf("[%s]过滤层驱动已存在，不重复安装\n", wdi_dev_info->desc);
                install_driver_success = true;
                break;
            }
        }

        //准备驱动
        srand((unsigned int)time(0));
        std::string szTmpPath = RW::GetSystemTempPath().append("tmp_driver_");
        struct wdi_options_prepare_driver prepare_options;
        memset(&prepare_options, 0, sizeof(prepare_options));
        struct wdi_options_install_driver install_options;
        memset(&install_options, 0, sizeof(install_options));
        install_options.hWnd = nullptr;

        if (crstrDriverType == "WINUSB")
        {
            prepare_options.driver_type = WDI_WINUSB;
            szTmpPath.append("winusb_");
        }
        else if (crstrDriverType == "LIBUSB0")
        {
            prepare_options.driver_type = WDI_LIBUSB0;
            szTmpPath.append("libusb-win32_");
        }
        else if (crstrDriverType == "LIBUSB0_FILTER")
        {
            prepare_options.driver_type = WDI_LIBUSB0;
            szTmpPath.append("libusb-win32-filter_");

            install_options.install_filter_driver = TRUE;
        }
        else if (crstrDriverType == "LIBUSBK")
        {
            prepare_options.driver_type = WDI_LIBUSBK;
            szTmpPath.append("libusbK_");
        }
        else
        {
            printf("driver_type : (%s), not support!\n", crstrDriverType.c_str());
            return false;
        }

        szTmpPath.append(RW::GetRandNumber(5, 5));
        prepare_options.vendor_name = (char*)wdi_get_vendor_name(wdi_dev_info->vid);
        printf("vendor_name[%s]\n", prepare_options.vendor_name);
        printf("szTmpPath[%s]\n", szTmpPath.c_str());

        if ((ret = wdi_prepare_driver(wdi_dev_info, szTmpPath.c_str(), "libusb_device.inf", &prepare_options) != WDI_SUCCESS))
        {
            printf("Error Preparing Driver[%d | %s]\n", ret, wdi_strerror(ret));
            return false;
        }

        //安装驱动
        if ((ret = wdi_install_driver(wdi_dev_info, szTmpPath.c_str(), (char*)"libusb_device.inf", &install_options) != WDI_SUCCESS))
        {
            printf("Error Installing Driver[%d | %s]\n", ret, wdi_strerror(ret));
            return false;
        }
        printf("Success Installing Driver\n");
        install_driver_success = true;
        break;
    }

    wdi_destroy_list(wdi_dev_list);
    return install_driver_success;
}

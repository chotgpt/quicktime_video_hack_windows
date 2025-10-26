#pragma once
#include <iostream>
#include <vector>

struct UsbDeviceData
{
	bool isCompositeParent;	//是否是复合设备的父节点
	bool isComposite;		//是否是复合设备的子节点
	std::string strDeviceName;	//设备名
	unsigned short vid;		//厂商ID (苹果为0x05AC)
	std::string strVid;
	unsigned short pid;		//厂商自定义的设备ID
	std::string strPid;
	std::string strSerial;		//序列号
	std::string strDriveName;	//驱动名
	std::string strLocationPath;//位置路径(Port_#0002.Hub_#0001)
	std::string strLocationInfo;//位置信息()
	std::string strCompatibleID;//兼容ID
};

class GCUSB
{
public:
	//取所有usb设备列表
	static std::vector<UsbDeviceData> GetUsbDeviceList();
	//取所有usb设备列表(带条件)
	static std::vector<UsbDeviceData> GetUsbDeviceList_ByVidAndComposite(unsigned short vid, bool isCompositeParent);
	//安装libusb驱动
	// crstrDriverType: WINUSB, LIBUSB0, LIBUSB0_FILTER, LIBUSBK
	static bool InstallDriver(unsigned short vid, unsigned short pid, std::string strSerial, const std::string& crstrDriverType);
	//卸载libusb驱动
	//安装libusb筛选器驱动
	//卸载libusb筛选器驱动
};
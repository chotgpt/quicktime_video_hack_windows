#pragma once
#include <iostream>
#include <map>
#include <mutex>

typedef struct
{
	std::string device_name;		//设备名("某某的iPhone13" 或 "未知(未信任)" 或 "未知(未安装驱动)")
	std::string device_model;		//设备型号("iPhone13" 或 "未知(未信任)" 或 "未知(未安装驱动)")
	std::string system_version;		//系统版本
}device_info;

class GetDeviceInfo
{
public:
	static void init();
	static bool isInit();
	static device_info get_device_info(const std::string& serial_number);
private:
	static void get_device_info_thread();
	static bool g_isInit;
	static std::map<std::string, device_info> g_device_info_list;
	static std::mutex _mutex;
};

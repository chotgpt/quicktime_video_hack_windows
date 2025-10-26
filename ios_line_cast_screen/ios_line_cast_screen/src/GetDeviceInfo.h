#pragma once
#include <iostream>
#include <map>
#include <mutex>

typedef struct
{
	std::string device_name;		//�豸��("ĳĳ��iPhone13" �� "δ֪(δ����)" �� "δ֪(δ��װ����)")
	std::string device_model;		//�豸�ͺ�("iPhone13" �� "δ֪(δ����)" �� "δ֪(δ��װ����)")
	std::string system_version;		//ϵͳ�汾
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

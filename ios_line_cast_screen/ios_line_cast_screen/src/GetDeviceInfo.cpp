#include "GetDeviceInfo.h"
#include "RW.h"

bool GetDeviceInfo::g_isInit;
std::map<std::string, device_info> GetDeviceInfo::g_device_info_list;
std::mutex GetDeviceInfo::_mutex;

void GetDeviceInfo::init()
{
	if (!g_isInit)
	{
		g_isInit = true;
		CloseHandle(CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)get_device_info_thread, NULL, 0, NULL));
	}
}

bool GetDeviceInfo::isInit()
{
	return GetDeviceInfo::g_isInit;
}

device_info GetDeviceInfo::get_device_info(const std::string& serial_number)
{
	if(!GetDeviceInfo::isInit()) return device_info();

	device_info info;
	_mutex.lock();
	if (g_device_info_list.find(serial_number) == g_device_info_list.end())
	{
		info = device_info();
		g_device_info_list[serial_number] = info;
	}
	else
	{
		info = g_device_info_list[serial_number];
	}
	_mutex.unlock();
	return info;
}

void GetDeviceInfo::get_device_info_thread()
{
	while (true)
	{
		for (auto& data : GetDeviceInfo::g_device_info_list)
		{
			device_info info;
			if (data.second.device_name.empty())
			{
				RW::ExecProcess("Frameworks\\ideviceinfo.exe", std::string("-u ").append(data.first).append(" -k DeviceName"), info.device_name, -1);
				RW::ExecProcess("Frameworks\\ideviceinfo.exe", std::string("-u ").append(data.first).append(" -k ProductType"), info.device_model, -1);
				RW::ExecProcess("Frameworks\\ideviceinfo.exe", std::string("-u ").append(data.first).append(" -k ProductVersion"), info.system_version, -1);
				RW::StrDelStr(info.device_name, "\r\n");
				RW::StrDelStr(info.device_model, "\r\n");
				RW::StrDelStr(info.system_version, "\r\n");
				info.device_name = RW::UTF8ToANSI(info.device_name);
				info.device_model = RW::UTF8ToANSI(info.device_model);
				info.system_version = RW::UTF8ToANSI(info.system_version);
				_mutex.lock();
				data.second = info;
				_mutex.unlock();
			}
		}
		Sleep(500);
	}
}
#pragma once
#include <windows.h>
#include <iostream>
#include <vector>

class RW
{
public:

	static std::wstring AnsiToUnicode(const std::string& buf);
	static std::string  ANSIToUTF8(const std::string& str);
	static std::string  UnicodeToAnsi(const std::wstring& buf);
	static std::string  UnicodeToUtf8(const std::wstring& buf);
	static std::string  UTF8ToANSI(const std::string& str);
	static std::wstring Utf8ToUnicode(const std::string& buf);
	static void StrToUpper(std::string &str);
	static void StrToLower(std::string &str);
	static void StrDelStr(std::string& text, std::string str);
	static std::string IntToStr(int num);
    static std::vector<std::string> split(std::string str, std::string pattern);
    static unsigned long long hexstr_to_long(const std::string& s);
	static std::string GetSystemTempPath();
	static int GetRandNum(int min, int max);
	static std::string GetRandString(int min = 5, int max = 10);
	static std::string GetRandNumber(int min = 5, int max = 10);
	static std::string ReadIni(const std::string& strFileName, const std::string& strProjectName, const std::string& stKeyName);
	static std::string GetProgramDir();
	static HANDLE CreateProcessAndCreateReadPipe(const char* lpApplicationName
		, const char* lpCommandLine
		, PHANDLE hReadPipe
		, PHANDLE hWritePipe
		, DWORD dwMilliseconds
		, bool isShow);
	static bool ExecProcess(const char* process_path, const std::string& command_line, std::string& read_text, DWORD dwMilliseconds);
	static bool ExecProcessDetached(const char* process_path, const std::string& command_line);
	static std::string GetVectorHexString(const std::vector<unsigned char>& vecArr);
	//进程是否存在
	static bool isProcessExist(const char* szProcessName);
	//结束指定进程
	static bool ExitProcess(const char* szProcessName);
};


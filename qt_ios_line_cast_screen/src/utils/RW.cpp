#include "RW.h"

#include <assert.h>
#include <windows.h>
#include <algorithm>
#include <tlhelp32.h>

std::wstring RW::AnsiToUnicode(const std::string& buf)
{
    int len = ::MultiByteToWideChar(CP_ACP, 0, buf.data(), (int)buf.size(), NULL, 0);
    if (len == 0) return L"";
    std::vector<wchar_t> unicode(len);
    len = ::MultiByteToWideChar(CP_ACP, 0, buf.data(), (int)buf.size(), unicode.data(), len);
    return std::wstring(unicode.data(), len);
}
std::string RW::ANSIToUTF8(const std::string& str)
{
    return RW::UnicodeToUtf8(AnsiToUnicode(str));
}

std::string RW::UnicodeToAnsi(const std::wstring& buf)
{
    int len = ::WideCharToMultiByte(CP_ACP, 0, buf.data(), (int)buf.size(), NULL, 0, NULL, NULL);
    if (len == 0) return "";
    std::vector<char> utf8(len);
    len = ::WideCharToMultiByte(CP_ACP, 0, buf.data(), (int)buf.size(), utf8.data(), len, NULL, NULL);
    return std::string(utf8.data(), len);
}
std::string RW::UnicodeToUtf8(const std::wstring& buf)
{
    int len = ::WideCharToMultiByte(CP_UTF8, 0, buf.data(), (int)buf.size(), NULL, 0, NULL, NULL);
    if (len == 0) return "";
    std::vector<char> utf8(len);
    len = ::WideCharToMultiByte(CP_UTF8, 0, buf.data(), (int)buf.size(), utf8.data(), len, NULL, NULL);
    return std::string(utf8.data(), len);
}
std::string RW::UTF8ToANSI(const std::string& str)
{
    return RW::UnicodeToAnsi(Utf8ToUnicode(str));
}
std::wstring RW::Utf8ToUnicode(const std::string& buf)
{
    int len = ::MultiByteToWideChar(CP_UTF8, 0, buf.data(), (int)buf.size(), NULL, 0);
    if (len == 0) return L"";
    std::vector<wchar_t> unicode(len);
    len = ::MultiByteToWideChar(CP_UTF8, 0, buf.data(), (int)buf.size(), unicode.data(), len);
    return std::wstring(unicode.data(), len);
}








void RW::StrToUpper(std::string &str)
{
    transform(str.begin(), str.end(), str.begin(), ::toupper);    //转大写
}

void RW::StrToLower(std::string &str)
{
    transform(str.begin(), str.end(), str.begin(), ::tolower);    //转小写
}

void RW::StrDelStr(std::string& text, std::string str)
{
    size_t begin = text.find(str, 0);
    while (begin != std::string::npos)
    {
        text.replace(begin, str.length(), "");
        begin = text.find(str, begin);
    }
}

std::string RW::IntToStr(int num)
{
    char buffer[128] = { 0 };
    _itoa_s(num, buffer, sizeof(buffer), 10);
    return buffer;
}

//字符串分割函数
std::vector<std::string> RW::split(std::string str, std::string pattern)
{
    std::string::size_type pos;
    std::vector<std::string> result;

    if (str.empty())
    {
        return result;
    }

    if (str.rfind(pattern) != str.length() - pattern.length())
    {
        //最后一位不是匹配串
        str += pattern;//扩展字符串以方便操作
    }

    size_t size = str.size();
    for (size_t i = 0; i < size; i++) {
        pos = str.find(pattern, i);
        if (pos < size) {
            std::string s = str.substr(i, pos - i);
            result.push_back(s);
            i = (int)pos + (int)pattern.size() - 1;
        }
    }
    return result;
}

unsigned long long RW::hexstr_to_long(const std::string& s)
{
    size_t n = s.length();
    unsigned long long num = 0;
    for (size_t i = 0; i < n; i++) {
        if (s[i] >= '0' && s[i] <= '9') {
            num += (s[i] - '0');
        }
        else if (s[i] >= 'a' && s[i] <= 'f') {
            num += (s[i] - 'a' + 10);
        }
        else if (s[i] >= 'A' && s[i] <= 'F') {
            num += (s[i] - 'A' + 10);
        }
        if (i < n - 1)
        {
            num *= 16;
        }
    }
    return num;
}

std::string RW::GetSystemTempPath()
{
    char szTmpPath[MAX_PATH] = { 0 };
    GetTempPathA(sizeof(szTmpPath), szTmpPath);
    return szTmpPath;
}

int RW::GetRandNum(int min, int max)
{
    //每个线程都要调用一次 srand((unsigned int)time(0));
    if (max < min) return 0;
    if (max == min) return max;
    return (rand() % (max - min + 1)) + min;
}

std::string RW::GetRandString(int min, int max)
{
    std::string str;
    int num = GetRandNum(min, max);
    for (int i = 0; i < num; i++)
    {
        int TmpRand = GetRandNum(1, 3);
        switch (TmpRand)
        {
        case 1:	//a-z
            str += GetRandNum('a', 'z');
            break;
        case 2:	//A-Z
            str += GetRandNum('A', 'Z');
            break;
        case 3:	//0-9
            str += GetRandNum('0', '9');
            break;
        default:
            break;
        }
    }
    return str;
}

std::string RW::GetRandNumber(int min, int max)
{
    std::string str;
    int num = GetRandNum(min, max);
    for (int i = 0; i < num; i++)
    {
        str += GetRandNum('0', '9');
    }
    return str;
}

std::string RW::ReadIni(const std::string& strFileName, const std::string& strProjectName, const std::string& stKeyName)
{
    char szBuffer[1024];
    memset(szBuffer, 0, sizeof(szBuffer));
    GetPrivateProfileStringA(strProjectName.c_str(), stKeyName.c_str(), "", szBuffer, sizeof(szBuffer) - 1, strFileName.c_str());
    return szBuffer;
}



//取库目录
std::string RW::GetProgramDir()
{
    char exeFullPath[MAX_PATH]; // Full path
    std::string strPath = "";

    GetModuleFileNameA(NULL, exeFullPath, MAX_PATH); //获取带有可执行文件名路径
    strPath = (std::string)exeFullPath;
    size_t pos = strPath.find_last_of('\\', strPath.length());
    strPath = strPath.substr(0, pos);  // 返回不带有可执行文件名的路径
    //strPath += "\\Frameworks";
    return strPath;
}

//创建进程并重定向读取管道,返回进程句柄
HANDLE RW::CreateProcessAndCreateReadPipe(const char* lpApplicationName
    , const char* lpCommandLine
    , PHANDLE hReadPipe
    , PHANDLE hWritePipe
    , DWORD dwMilliseconds
    , bool isShow)
{
    HANDLE hRead, hWrite;
    SECURITY_ATTRIBUTES sec;
    sec.nLength = sizeof(SECURITY_ATTRIBUTES);
    sec.bInheritHandle = TRUE;
    sec.lpSecurityDescriptor = NULL;
    BOOL fResult = CreatePipe(&hRead, &hWrite, &sec, 0);

    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hWrite;
    si.hStdInput = hRead;
    si.hStdError = hWrite;
    si.wShowWindow = SW_HIDE;
    if (isShow)
        si.wShowWindow = SW_SHOW;

    PROCESS_INFORMATION pi;

    char buffer[1024] = { 0 };
    GetCurrentDirectoryA(1024, buffer);
    fResult = CreateProcessA(lpApplicationName,
        (char*)lpCommandLine,
        &sec,
        &sec,
        TRUE,
        NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW,
        NULL,
        NULL,
        &si,
        &pi);
    if (!fResult)
    {
        *hReadPipe = 0;
        *hWritePipe = 0;
        return NULL;
    }

    // Waiting for the process to finish.
    if (dwMilliseconds != 0)
    {
        WaitForSingleObject(pi.hProcess, dwMilliseconds);
    }

    *hReadPipe = hRead;
    *hWritePipe = hWrite;
    CloseHandle(pi.hThread);
    return pi.hProcess;
}

bool RW::ExecProcess(const char* process_path, const std::string& command_line, std::string& read_text, DWORD dwMilliseconds)
{
    HANDLE hRead = nullptr;
    HANDLE hWrite = nullptr;
    HANDLE hProcess = nullptr;

    read_text.clear();
    std::string strApplicationName = GetProgramDir() + "\\" + process_path;
    std::string strCommandLine = "\"" + GetProgramDir() + "\\" + process_path + "\"" + " " + command_line;
    hProcess = CreateProcessAndCreateReadPipe(strApplicationName.c_str(), strCommandLine.c_str(), &hRead, &hWrite, dwMilliseconds, false);
    if (!hProcess || !hRead) return false;

    char* szBuffer = (char*)calloc(1, 10240);
    if (!szBuffer) return false;

    DWORD dwReadLen = 0;
    ReadFile(hRead, szBuffer, 10240, &dwReadLen, NULL);
    read_text = szBuffer;
    if (hWrite) CloseHandle(hWrite);
    CloseHandle(hRead);
    CloseHandle(hProcess);
    free(szBuffer);
    return true;
}

bool RW::ExecProcessDetached(const char* process_path, const std::string& command_line)
{
    HANDLE hRead = nullptr;
    HANDLE hWrite = nullptr;
    HANDLE hProcess = nullptr;

    std::string strApplicationName = GetProgramDir() + "\\" + process_path;
    std::string strCommandLine = "\"" + GetProgramDir() + "\\" + process_path + "\"" + " " + command_line;
    hProcess = CreateProcessAndCreateReadPipe(strApplicationName.c_str(), strCommandLine.c_str(), &hRead, &hWrite, 0, false);
    if (!hProcess || !hRead) return false;
    if (hWrite) CloseHandle(hWrite);
    CloseHandle(hRead);
    CloseHandle(hProcess);
    return true;
}

std::string RW::GetVectorHexString(const std::vector<unsigned char>& vecArr)
{
    char tmpBuffer[10];
    size_t nBufferLength = vecArr.size() * 10 + 200;
    char* buffer = (char*)calloc(1, nBufferLength);
    if (!buffer)
    {
        return "";
    }
    for (size_t i = 0; i < vecArr.size(); i++)
    {
        sprintf_s(tmpBuffer, sizeof(tmpBuffer), "%02X", vecArr[i]);
        strcat_s(buffer, nBufferLength, tmpBuffer);
    }
    std::string szRet = buffer;
    free(buffer);
    return szRet;
}

//进程是否存在
bool RW::isProcessExist(const char* szProcessName)
{
    bool bRet = false;
    HANDLE hSnapshort = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshort == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32 stcProcessInfo;
    stcProcessInfo.dwSize = sizeof(stcProcessInfo);
    BOOL BRet = Process32First(hSnapshort, &stcProcessInfo);
    while (BRet)
    {
        if (strcmp(stcProcessInfo.szExeFile, szProcessName) == 0)
        {
            bRet = true;
            break;
        }
        BRet = Process32Next(hSnapshort, &stcProcessInfo);
    }
    CloseHandle(hSnapshort);
    return bRet;
}

bool RW::ExitProcess(const char* szProcessName)
{
    HANDLE hSnapshort = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshort == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32 stcProcessInfo;
    stcProcessInfo.dwSize = sizeof(stcProcessInfo);
    BOOL  bRet = Process32First(hSnapshort, &stcProcessInfo);
    while (bRet)
    {
        if (strcmp(stcProcessInfo.szExeFile, szProcessName) == 0)
        {
            HANDLE hProcess = ::OpenProcess(PROCESS_TERMINATE, FALSE, stcProcessInfo.th32ProcessID);	//获取进程句柄
            ::TerminateProcess(hProcess, 0);
            CloseHandle(hProcess);
        }
        bRet = Process32Next(hSnapshort, &stcProcessInfo);
    }
    CloseHandle(hSnapshort);
    return true;
}









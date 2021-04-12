#define DLLAPI_EXPORTS
#include "SimpleLog.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <fstream>
#ifdef _WIN32
#include <io.h>
#include <direct.h>
#include <Windows.h>
#else
#include <unistd.h>
#include <dirent.h>
#endif


using namespace std;

namespace QQ
{

LogManager::LogManager()
{

}

LogManager::~LogManager()
{

}

void LogManager::Initialize(const string &parentPath,const string &_logName)
{
    // print to stdout
    if(_logName.size()==0)
        return;

    if(!Exists(parentPath))
    {
        CreateDirectories(parentPath);
    }

    // check whether exist
    std::map<string, FILE*>::const_iterator iter = logMap.find(_logName);
    if (iter == logMap.end())
    {
        string logName = parentPath+ _logName + ".log";
        FILE *logFile = fopen(logName.c_str(), "a"); // w,a 根据实际情况，决定使用w还是a
        if(logFile!=NULL)
        {
            logMap.insert(std::make_pair(_logName, logFile));
        }
    }

}

FILE* LogManager::GetLogFile(const string &logName)
{
    std::map<string, FILE*>::const_iterator iter=logMap.find(logName);
    if(iter==logMap.end())
    {
        return NULL;
    }

    return (*iter).second;
}
void LogManager::Close(const string &logName)
{
    std::map<string, FILE*>::const_iterator iter=logMap.find(logName);
    if(iter==logMap.end())
    {
        return;
    }

    // close and delete
    fclose((*iter).second);
    logMap.erase(iter);
}
std::mutex &LogManager::GetLogMutex()
{
    return logMutex;
}
LogTime LogManager::GetTime()
{
    LogTime currentTime;

#if (defined WIN32 || defined _WIN32)
    SYSTEMTIME systemTime;
    GetLocalTime(&systemTime);

    char temp[8] = { 0 };
    sprintf(temp, "%04d", systemTime.wYear);
    currentTime.year=string(temp);
    sprintf(temp, "%02d", systemTime.wMonth);
    currentTime.month=string(temp);
    sprintf(temp, "%02d", systemTime.wDay);
    currentTime.day=string(temp);
    sprintf(temp, "%02d", systemTime.wHour);
    currentTime.hour=string(temp);
    sprintf(temp, "%02d", systemTime.wMinute);
    currentTime.minute=string(temp);
    sprintf(temp, "%02d", systemTime.wSecond);
    currentTime.second=string(temp);
    sprintf(temp, "%03d", systemTime.wMilliseconds);
    currentTime.millisecond=string(temp);
    sprintf(temp, "%d", systemTime.wDayOfWeek);
    currentTime.weekDay=string(temp);
#else
    struct timeval    tv;
    struct tm         *p;
    gettimeofday(&tv, NULL);
    p = localtime(&tv.tv_sec);

    char temp[8]={0};
    sprintf(temp,"%04d",1900+p->tm_year);
    currentTime.year=string(temp);
    sprintf(temp,"%02d",1+p->tm_mon);
    currentTime.month=string(temp);
    sprintf(temp,"%02d",p->tm_mday);
    currentTime.day=string(temp);
    sprintf(temp,"%02d",p->tm_hour);
    currentTime.hour=string(temp);
    sprintf(temp,"%02d",p->tm_min);
    currentTime.minute=string(temp);
    sprintf(temp,"%02d",p->tm_sec);
    currentTime.second=string(temp);
    sprintf(temp,"%03d",tv.tv_usec/1000);
    currentTime.millisecond = string(temp);
    sprintf(temp, "%03d", tv.tv_usec % 1000);
    currentTime.microsecond = string(temp);
    sprintf(temp, "%d", p->tm_wday);
    currentTime.weekDay = string(temp);
#endif
    return currentTime;
}

#if defined _WIN32 || defined WINCE
# include <windows.h>
struct dirent
{
	const char* d_name;
};

struct DIR
{
#ifdef WINRT
	WIN32_FIND_DATAW data;
#else
	WIN32_FIND_DATA data;
#endif
	HANDLE handle;
	dirent ent;
#ifdef WINRT
	DIR() { }
	~DIR()
	{
		if (ent.d_name)
			delete[] ent.d_name;
	}
#endif
};

#else
# include <dirent.h>
# include <sys/stat.h>
#endif

static bool IsPathSeparator(char c)
{
    return c == '/' || c == '\\';
}

static bool isDir(const string &path, DIR* dir)
{
#if defined _WIN32 || defined WINCE
    DWORD attributes;
    BOOL status = TRUE;
    if (dir)
        attributes = dir->data.dwFileAttributes;
    else
    {
        WIN32_FILE_ATTRIBUTE_DATA all_attrs;
#ifdef WINRT
        wchar_t wpath[MAX_PATH];
        size_t copied = mbstowcs(wpath, path.c_str(), MAX_PATH);
        CV_Assert((copied != MAX_PATH) && (copied != (size_t)-1));
        status = ::GetFileAttributesExW(wpath, GetFileExInfoStandard, &all_attrs);
#else
        status = ::GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &all_attrs);
#endif
        attributes = all_attrs.dwFileAttributes;
    }

    return status && ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
#else
    (void)dir;
    struct stat stat_buf;
    if (0 != stat(path.c_str(), &stat_buf))
        return false;
    int is_dir = S_ISDIR(stat_buf.st_mode);
    return is_dir != 0;
#endif
}

static bool IsDirectory(const string &path)
{
    return isDir(path, NULL);
}
static bool CreateDirectory(const string &path)
{
#if defined WIN32 || defined _WIN32 || defined WINCE
#ifdef WINRT
            wchar_t wpath[MAX_PATH];
        size_t copied = mbstowcs(wpath, path.c_str(), MAX_PATH);
        CV_Assert((copied != MAX_PATH) && (copied != (size_t)-1));
        int result = CreateDirectoryA(wpath, NULL) ? 0 : -1;
#else
            int result = _mkdir(path.c_str());
#endif
#elif defined __linux__ || defined __APPLE__
            int result = mkdir(path.c_str(), 0777);
#else
            int result = -1;
#endif

        if (result == -1)
        {
            return IsDirectory(path);
        }
        return true;
}

bool LogManager::CreateDirectories(const string &directoryPath)
{
    string path = directoryPath;

    for (;;)
    {
        char last_char = path.empty() ? 0 : path[path.length() - 1];
        if (IsPathSeparator(last_char))
        {
            path = path.substr(0, path.length() - 1);
            continue;
        }
        break;
    }

    if (path.empty() || path == "./" || path == ".\\" || path == ".")
        return true;
    if (IsDirectory(path))
        return true;

    size_t pos = path.rfind('/');
    if (pos == string::npos)
        pos = path.rfind('\\');
    if (pos != string::npos)
    {
        string parent_directory = path.substr(0, pos);
        if (!parent_directory.empty())
        {
            if (!CreateDirectories(parent_directory))
                return false;
        }
    }

    return CreateDirectory(path);
}

bool LogManager::Exists(const string& path)
{

#if defined _WIN32 || defined WINCE
        BOOL status = TRUE;
    {
        WIN32_FILE_ATTRIBUTE_DATA all_attrs;
#ifdef WINRT
        wchar_t wpath[MAX_PATH];
        size_t copied = mbstowcs(wpath, path.c_str(), MAX_PATH);
        CV_Assert((copied != MAX_PATH) && (copied != (size_t)-1));
        status = ::GetFileAttributesExW(wpath, GetFileExInfoStandard, &all_attrs);
#else
        status = ::GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &all_attrs);
#endif
    }

    return !!status;
#else
    struct stat stat_buf;
    return (0 == stat(path.c_str(), &stat_buf));
#endif
}


}

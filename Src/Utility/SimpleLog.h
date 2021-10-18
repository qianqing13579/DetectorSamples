//////////////////////////////////////////////////////////////////////////
// 简易日志
//
// Please contact me if you find any bugs, or have any suggestions.
// Contact:
//		Email:654393155@qq.com
//		Blog:http://blog.csdn.net/qianqing13579
//////////////////////////////////////////////////////////////////////////

#ifndef __SIMPLE_LOG_H__
#define __SIMPLE_LOG_H__

#include <time.h>
#include <string>
#include <map>
#include <thread>
#include <mutex>
#if (defined WIN32 || defined _WIN32)
#include <Windows.h>
#else
#include <sys/time.h>
#endif

using namespace std;


/* 简易日志简介
轻量级日志系统，不依赖于其他第三方库，只需要包含一个头文件就可以使用。提供了4种日志级别，包括INFO,DEBUG,WARN和ERROR。

示例1:
    // 初始化日志，在./Log/目录下创建两个日志文件log1.log和log2.log(注意：目录./Log/需要存在，否则日志创建失败)
    LogManager::GetInstance()->Initialize("./Log/","log1");
    LogManager::GetInstance()->Initialize("./Log/","log2");

    // 写日志
    string log = "Hello World";
    LOG_INFO(LogManager::GetInstance()->GetLogFile("log1"), "%s\n", log.c_str()); // 写入log1.log
    LOG_INFO(LogManager::GetInstance()->GetLogFile("log2"), "%s\n", log.c_str()); // 写入log2.log

    // 关闭日志
    LogManager::GetInstance()->Close("log1");
    LogManager::GetInstance()->Close("log2");

示例2:
    // 将日志输出到控制台
    string log = "Hello World";
    LOG_INFO(stdout, "%s\n", log.c_str()); 

注意：
    1. 需要C++11
    2. 多线程的时候需要加锁(打开#define LOG_MUTEX)，否则会导致日志显示混乱

*/

// #define LOG_MUTEX  // 加锁

class LogManager
{
private:
    LogManager(){}

public:
    ~LogManager(){}
    inline void Initialize(const string &parentPath,const string &logName)
    {
        // 日志名为空表示输出到控制台
        if(logName.size()==0)
            return;

        // 查找该日志文件，如果没有则创建
        std::map<string, FILE*>::const_iterator iter = logMap.find(logName);
        if (iter == logMap.end())
        {
            string pathOfLog = parentPath+ logName + ".log";
            FILE *logFile = fopen(pathOfLog.c_str(), "a"); // w:覆盖原有文件,a:追加
            if(logFile!=NULL)
            {
                logMap.insert(std::make_pair(logName, logFile));
            }
        }

    }
    inline FILE* GetLogFile(const string &logName)
    {
        std::map<string, FILE*>::const_iterator iter=logMap.find(logName);
        if(iter==logMap.end())
        {
            return NULL;
        }

        return (*iter).second;
    }
    inline void Close(const string &logName)
    {
        std::map<string, FILE*>::const_iterator iter=logMap.find(logName);
        if(iter==logMap.end())
        {
            return;
        }

        fclose((*iter).second);
        logMap.erase(iter);
    }
    inline std::mutex &GetLogMutex()
    {
        return logMutex;
    }

    // Singleton(注意线程安全的问题)
    static LogManager* GetInstance()
    {
        static LogManager logManager;
        return &logManager;
    }
private:
    std::map<string, FILE*> logMap;
    std::mutex logMutex;
};

#ifdef LOG_MUTEX
    #define LOCK	LogManager::GetInstance()->GetLogMutex().lock()
    #define UNLOCK	LogManager::GetInstance()->GetLogMutex().unlock()
#else
    #define LOCK
    #define UNLOCK
#endif

// log time
typedef struct  _LogTime
{
    string year;
    string month;
    string day;
    string hour;
    string minute;
    string second;
    string millisecond; // ms
    string microsecond; // us
    string weekDay;
}LogTime;

inline LogTime GetTime()
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
    sprintf(temp,"%03d",(int)(tv.tv_usec/1000));
    currentTime.millisecond = string(temp);
    sprintf(temp, "%03d", (int)(tv.tv_usec % 1000));
    currentTime.microsecond = string(temp);
    sprintf(temp, "%d", p->tm_wday);
    currentTime.weekDay = string(temp);
#endif
    return currentTime;
}

#define LOG_TIME(logFile) \
do\
{\
    LogTime currentTime=GetTime(); \
    fprintf(((logFile == NULL) ? stdout : logFile), "%s-%s-%s %s:%s:%s.%s\t",currentTime.year.c_str(),currentTime.month.c_str(),currentTime.day.c_str(),currentTime.hour.c_str(),currentTime.minute.c_str(),currentTime.second.c_str(),currentTime.millisecond.c_str()); \
}while (0)


#define LOG_INFO(logFile,logInfo, ...) \
do\
{\
    LOCK; \
    LOG_TIME(logFile); \
    fprintf(((logFile == NULL) ? stdout : logFile), "INFO\t"); \
    fprintf(((logFile == NULL) ? stdout : logFile), "[%s:%d (%s) ]: ", __FILE__, __LINE__, __FUNCTION__); \
    fprintf(((logFile == NULL) ? stdout : logFile), logInfo, ## __VA_ARGS__); \
    fflush(logFile); \
    UNLOCK; \
} while (0)

#define LOG_DEBUG(logFile,logInfo, ...) \
do\
{\
    LOCK; \
    LOG_TIME(logFile);\
    fprintf(((logFile==NULL)?stdout:logFile), "DEBUG\t"); \
    fprintf(((logFile==NULL)?stdout:logFile), "[%s:%d (%s) ]: ", __FILE__, __LINE__, __FUNCTION__); \
    fprintf(((logFile==NULL)?stdout:logFile),logInfo, ## __VA_ARGS__); \
    fflush(logFile); \
    UNLOCK; \
} while (0)

#define LOG_ERROR(logFile,logInfo, ...) \
do\
{\
    LOCK; \
    LOG_TIME(logFile);\
    fprintf(((logFile==NULL)?stdout:logFile), "ERROR\t"); \
    fprintf(((logFile==NULL)?stdout:logFile), "[%s:%d (%s) ]: ", __FILE__, __LINE__, __FUNCTION__); \
    fprintf(((logFile==NULL)?stdout:logFile),logInfo, ## __VA_ARGS__); \
    fflush(logFile); \
    UNLOCK; \
} while (0)

#define LOG_WARN(logFile,logInfo, ...) \
do\
{\
    LOCK; \
    LOG_TIME(logFile);\
    fprintf(((logFile==NULL)?stdout:logFile), "WARN\t"); \
    fprintf(((logFile==NULL)?stdout:logFile), "[%s:%d (%s) ]: ", __FILE__, __LINE__, __FUNCTION__); \
    fprintf(((logFile==NULL)?stdout:logFile),logInfo, ## __VA_ARGS__); \
    fflush(logFile); \
    UNLOCK; \
} while (0)

#endif // __SIMPLE_LOG_H__


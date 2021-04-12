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

#include<time.h>
#include<string>
#include<map>
#include<thread>
#include<mutex>
#if (defined WIN32 || defined _WIN32)
#include<Windows.h>
#else
#include <sys/time.h>
#endif
#include"CommonDefinition.h"


using namespace std;

namespace QQ
{
	/* SimpleLog

	Example：
	// Initialize log (you can initialize several logs)
	LogManager::GetInstance()->Initialize("./","log1");
	LogManager::GetInstance()->Initialize("./","log2");

	// write log to file
	string age = "age";
	int value = 18;
	LOG_DEBUG(LogManager::GetInstance()->GetLogFile("log1"), "log1 %s:%d\n", age.c_str(), value); // 往哪个日志中写入哪些内容
	LOG_DEBUG(LogManager::GetInstance()->GetLogFile("log1"), "log1 %s:%d\n", age.c_str(), value);
	LOG_DEBUG(LogManager::GetInstance()->GetLogFile("log2"), "log2 %s:%d\n", age.c_str(), value);
	LOG_DEBUG(LogManager::GetInstance()->GetLogFile("log2"), "log2 %s:%d\n", age.c_str(), value);

	// write log to console
	LOG_DEBUG(stdout, "%s:%d\n", age.c_str(), value);
	LOG_DEBUG(stdout, "%s:%d\n", age.c_str(), value);

	// close log
	LogManager::GetInstance()->Close("log1");
	LogManager::GetInstance()->Close("log2");

	Note:
	1. 加锁用于解决多线程Log混乱问题，由于加锁是一个耗时的操作，所以需要根据实际项目决定是否采用加锁机制
	2. 需要C++11
	*/
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

	//#define LOG_MUTEX // 多线程调用的时候需要加锁

    class DLL_EXPORTS LogManager
	{
	public:
		virtual ~LogManager();

		void Initialize(const string &parentPath, const string &_logName);
		FILE* GetLogFile(const string &logName);
		void Close(const string &logName);
		std::mutex &GetLogMutex();
		static LogTime GetTime();

		// Singleton(注意线程安全的问题)
		static LogManager* GetInstance()
		{
			static LogManager logManager;
			return &logManager;
		}

	private:
		// 私有构造函数，单例模式
		LogManager();

		// utility
		bool CreateDirectories(const string &directoryPath);
		bool Exists(const string& path);

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


#define LOG_TIME(logFile) \
	do\
	{\
	LogTime currentTime = LogManager::GetTime(); \
	fprintf(((logFile == NULL) ? stdout : logFile), "%s-%s-%s %s:%s:%s.%s\t", currentTime.year.c_str(), currentTime.month.c_str(), currentTime.day.c_str(), currentTime.hour.c_str(), currentTime.minute.c_str(), currentTime.second.c_str(), currentTime.millisecond.c_str()); \
	}while (0)

#define LOG_INFO(logFile,logInfo, ...) \
	do\
	{\
	LOCK; \
	LOG_TIME(logFile); \
	fprintf(((logFile == NULL) ? stdout : logFile), "[%x]\t", std::this_thread::get_id()); \
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
	LOG_TIME(logFile); \
	fprintf(((logFile == NULL) ? stdout : logFile), "[%x]\t", std::this_thread::get_id()); \
	fprintf(((logFile == NULL) ? stdout : logFile), "DEBUG\t"); \
	fprintf(((logFile == NULL) ? stdout : logFile), "[%s:%d (%s) ]: ", __FILE__, __LINE__, __FUNCTION__); \
	fprintf(((logFile == NULL) ? stdout : logFile), logInfo, ## __VA_ARGS__); \
	fflush(logFile); \
	UNLOCK; \
	} while (0)

#define LOG_ERROR(logFile,logInfo, ...) \
	do\
	{\
	LOCK; \
	LOG_TIME(logFile); \
	fprintf(((logFile == NULL) ? stdout : logFile), "[%x]\t", std::this_thread::get_id()); \
	fprintf(((logFile == NULL) ? stdout : logFile), "ERROR\t"); \
	fprintf(((logFile == NULL) ? stdout : logFile), "[%s:%d (%s) ]: ", __FILE__, __LINE__, __FUNCTION__); \
	fprintf(((logFile == NULL) ? stdout : logFile), logInfo, ## __VA_ARGS__); \
	fflush(logFile); \
	UNLOCK; \
	} while (0)

#define LOG_WARN(logFile,logInfo, ...) \
	do\
	{\
	LOCK; \
	LOG_TIME(logFile); \
	fprintf(((logFile == NULL) ? stdout : logFile), "[%x]\t", std::this_thread::get_id()); \
	fprintf(((logFile == NULL) ? stdout : logFile), "WARN\t"); \
	fprintf(((logFile == NULL) ? stdout : logFile), "[%s:%d (%s) ]: ", __FILE__, __LINE__, __FUNCTION__); \
	fprintf(((logFile == NULL) ? stdout : logFile), logInfo, ## __VA_ARGS__); \
	fflush(logFile); \
	UNLOCK; \
	} while (0)

}

#endif // __SIMPLE_LOG_H__


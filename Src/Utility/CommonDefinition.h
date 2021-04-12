//////////////////////////////////////////////////////////////////////////
// 通用定义
//
// Please contact me if you find any bugs, or have any suggestions.
// Contact:
//		Email:654393155@qq.com
//		Blog:http://blog.csdn.net/qianqing13579
//////////////////////////////////////////////////////////////////////////

#ifndef __COMMON_DEFINITION_H__
#define __COMMON_DEFINITION_H__

#include<string>
#include"opencv2/opencv.hpp"

using namespace std;
using namespace cv;

namespace QQ
{
// DLL
//WIN32,_WIN32：平台判断
//DLLAPI_EXPORTS：条件宏
//DLL_EXPORTS:DLL导出符号
#ifdef DLLAPI_EXPORTS
#if (defined WIN32 || defined _WIN32 || defined WINCE || defined __CYGWIN__)
#  define DLL_EXPORTS __declspec(dllexport)
#elif defined __GNUC__ && __GNUC__ >= 4
#  define DLL_EXPORTS __attribute__((visibility ("default")))
#endif
#endif

#ifndef DLL_EXPORTS
#define DLL_EXPORTS
#endif

// memory deallocation
#define SAFE_DELETE(p)  { if ((p)) { delete (p); (p) = NULL; } }
#define SAFE_DELETE_ARRAY(p)  { if ((p)) { delete[] (p); (p) = NULL; } }

// 路径分隔符(Linux:‘/’,Windows:’\\’)
#ifdef _WIN32
#define  PATH_SEPARATOR '\\'
#else
#define  PATH_SEPARATOR '/'
#endif

#define CONFIG_FILE                                                     "../Resource/Configuration.xml"

// time
typedef struct DLL_EXPORTS __Time
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
}_Time;

// 错误码
typedef enum  DLL_EXPORTS _ErrorCode
{
    OK=0,  // 0
    MODEL_NOT_EXIST, // 模型不存在
    CONFIG_FILE_NOT_EXIST, // 配置文件不存在
    FAIL_TO_LOAD_MODEL, // 加载模型失败
    FAIL_TO_OPEN_CONFIG_FILE, // 加载配置文件失败
    EMPTY_IMAGE_OR_NOT_CV_8U, // 图像类型错误
}ErrorCode;

// 预测值:score label
typedef struct DLL_EXPORTS _ResultOfPrediction
{
    float confidence; // score
    int label;
}ResultOfPrediction;

typedef struct DLL_EXPORTS _InitializationParameterOfDetector
{
    std::string parentPath;
    std::string configFilePath;
    cv::Size inputSize;
    std::string nodeName;
    std::string logName;
}InitializationParameterOfDetector;

typedef struct _InitializationParameterOfDetector InitializationParameterOfClassifier;

typedef struct DLL_EXPORTS _ResultOfDetection
{
    Rect boundingBox;
    float confidence;
    int classID;
    string className;
    bool exist;

    _ResultOfDetection():exist(true){}

}ResultOfDetection;

}

#endif // COMMONDEFINITION


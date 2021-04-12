//////////////////////////////////////////////////////////////////////////
// 检测器接口
//
// Please contact me if you find any bugs, or have any suggestions.
// Contact:
//		Email:654393155@qq.com
//		Blog:http://blog.csdn.net/qianqing13579
//////////////////////////////////////////////////////////////////////////

#ifndef __DETECTOR_INTERFACE_H__
#define __DETECTOR_INTERFACE_H__

#include <vector>
#include "opencv2/opencv.hpp"
#include "CommonDefinition.h"

using namespace std;
using namespace cv;

namespace QQ
{


class DLL_EXPORTS DetectorInterface
{
public:
    DetectorInterface();
    virtual ~DetectorInterface();

    virtual ErrorCode Initialize(InitializationParameterOfDetector initializationParameterOfDetector)=0;

    virtual ErrorCode Detect(const Mat &srcImage,std::vector<ResultOfDetection> &resultsOfDetection)=0;

    virtual ErrorCode SetMiniSize(const int miniSize);

protected:
    ErrorCode DoCommonInitialization(InitializationParameterOfDetector initializationParameterOfDetector);

protected:
    FileStorage configurationFile;

    InitializationParameterOfDetector initializationParameter;

    // 检测器的输入大小
    int inputWidth;
    int inputHeight;

    FILE *logFile;

};

}

#endif // DETECTORINTERFACE


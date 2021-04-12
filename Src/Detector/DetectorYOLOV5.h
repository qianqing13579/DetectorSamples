//////////////////////////////////////////////////////////////////////////
// YOLOV5检测器
//
// Please contact me if you find any bugs, or have any suggestions.
// Contact:
//		Email:654393155@qq.com
//		Blog:http://blog.csdn.net/qianqing13579
//////////////////////////////////////////////////////////////////////////

#ifndef __DETECTOR_YOLOV5_H__
#define __DETECTOR_YOLOV5_H__

#include "DetectorInterface.h"

using namespace cv;
using namespace dnn;
using namespace std;

namespace QQ
{
// YOLOV5参数
typedef struct _YOLOV5Parameter
{
    int numberOfClasses;// 类别数
    float confidenceThreshold;// 置信度阈值
    float nmsThreshold;// NMS阈值
    float objectThreshold;// 目标置信度值

    int numberOfDetectionLayer;//检测层数量
    int numberOfAnchors;// 每个检测层anchor数量
    std::vector<float> stride;// 步长
    std::vector<int> heightOfDetectionLayer;// 检测层高
    std::vector<int> widthOfDetectionLayer;// 检测层宽
    std::vector<std::vector<float>> anchors; // anchor
}YOLOV5Parameter;

class DetectorYOLOV5:public DetectorInterface
{
public:
    DetectorYOLOV5();
    virtual ~DetectorYOLOV5();
    virtual ErrorCode Initialize(InitializationParameterOfDetector initializationParameterOfDetector);
    virtual ErrorCode Detect(const Mat &srcImage, std::vector<ResultOfDetection> &resultsOfDetection);

private:
    void sigmoid(Mat* out, int length);

private:
    Net net;

    cv::Size inputSize;
    vector<string> classNames;
    YOLOV5Parameter yolov5Parameter;
    
};

}



#endif // YOLOV5V5


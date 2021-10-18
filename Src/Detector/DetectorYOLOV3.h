//////////////////////////////////////////////////////////////////////////
// YOLOV3检测器
//
// Please contact me if you find any bugs, or have any suggestions.
// Contact:
//		Email:654393155@qq.com
//		Blog:http://blog.csdn.net/qianqing13579
//////////////////////////////////////////////////////////////////////////

#ifndef __DETECTOR_YOLOV3_H__
#define __DETECTOR_YOLOV3_H__

#include "DetectorInterface.h"

using namespace cv;
using namespace cv::dnn;

namespace QQ
{

// YOLOV3参数
typedef struct _YOLOV3Parameter
{
    int numberOfClasses;// 类别数
    int numberOfAnchors;// 每个检测层anchor数量
    float confidenceThreshold;// 置信度阈值
    float nmsThreshold;// NMS阈值
    std::vector<float> biases; // anchor
}YOLOV3Parameter;

typedef struct  _YOLOV3PredictionResult
{
	float x;
	float y;
	float w;
	float h;
	float objScore;
	float classScore;
	float confidence;
	int classType;
}YOLOV3PredictionResult;

class DetectorYOLOV3 : public DetectorInterface
{
public:
    DetectorYOLOV3();
    virtual ~DetectorYOLOV3();

    virtual ErrorCode Initialize(InitializationParameterOfDetector initializationParameterOfDetector);

    virtual ErrorCode Detect(const Mat &srcImage, std::vector<ResultOfDetection> &resultsOfDetection);

private:
    // 获取YOLOV3参数
    void GetYOLOV3Parameter();
    void GetResult(const std::vector<Mat> &bottom,const Mat &srcImage,std::vector<ResultOfDetection> &resultsOfDetection);
    
    // utility
    float sigmoid(float x);
    float overlap(float x1, float w1, float x2, float w2);
    float box_intersection(vector<float> a, vector<float> b);
    float box_union(vector<float> a, vector<float> b);
    float box_iou(vector<float> a, vector<float> b);
    void ApplyNms(vector< YOLOV3PredictionResult >& boxes, vector<int>& idxes, float threshold);
    void correct_yolo_boxes(YOLOV3PredictionResult &det, int w, int h, int netw, int neth, int relative);
    void get_region_box(vector<float> &b, float* x, vector<float> biases, int n, int index, int i, int j, int lw, int lh, int w, int h, int stride);

private:
    cv::dnn::Net net;
    std::vector<String> outputNodeNames;

    float scale;
    float meanValue1,meanValue2,meanValue3;
    bool swapRB;

    int backendId;
    int targetId;

    YOLOV3Parameter yolov3Parameter;

    
    
};

}

#endif

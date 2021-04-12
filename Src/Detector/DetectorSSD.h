//////////////////////////////////////////////////////////////////////////
// SSD检测器
//
// Please contact me if you find any bugs, or have any suggestions.
// Contact:
//		Email:654393155@qq.com
//		Blog:http://blog.csdn.net/qianqing13579
//////////////////////////////////////////////////////////////////////////

#ifndef __DETECTOR_SSD_H__
#define __DETECTOR_SSD_H__

#include "DetectorInterface.h"

using namespace cv;
using namespace cv::dnn;

namespace QQ
{

#define SSD_MAX_PRIORBOX_LAYER_NUM      10 // 能够支持的最大检测层数量

typedef struct _SSDParameter
{
    int numberOfPriorBoxLayer;

    // Model Parameters
    int convHeight[SSD_MAX_PRIORBOX_LAYER_NUM*2];
    int convWidth[SSD_MAX_PRIORBOX_LAYER_NUM*2];
    int convChannel[SSD_MAX_PRIORBOX_LAYER_NUM*2];

    // PriorBoxLayer Parameters
    int priorBoxWidth[SSD_MAX_PRIORBOX_LAYER_NUM];
    int priorBoxHeight[SSD_MAX_PRIORBOX_LAYER_NUM];
    vector<vector<float>> priorBoxMinSize;
    vector<vector<float>> priorBoxMaxSize;
    int minSizeNum[SSD_MAX_PRIORBOX_LAYER_NUM];
    int maxSizeNum[SSD_MAX_PRIORBOX_LAYER_NUM];
    int srcImageHeight;
    int srcImageWidth;
    int inputAspectRatioNum[SSD_MAX_PRIORBOX_LAYER_NUM];
    vector<vector<float>> priorBoxAspectRatio;
    float priorBoxStepWidth[SSD_MAX_PRIORBOX_LAYER_NUM];
    float priorBoxStepHeight[SSD_MAX_PRIORBOX_LAYER_NUM];
    float offset;
    int flip[SSD_MAX_PRIORBOX_LAYER_NUM];
    int clip[SSD_MAX_PRIORBOX_LAYER_NUM];
    int priorBoxVar[4];

    // SoftmaxLayer Parameters
    int softMaxInChn[SSD_MAX_PRIORBOX_LAYER_NUM];
    int softMaxInHeight;
    int concatNum;
    int softMaxOutWidth;
    int softMaxOutHeight;
    int softMaxOutChn;

    // DetectionOutLayer Parameters
    int classNum;
    int topK;
    int keepTopK;
    int NMSThresh;
    int confThresh;
    int detectInputChn[SSD_MAX_PRIORBOX_LAYER_NUM];
    int convStride[SSD_MAX_PRIORBOX_LAYER_NUM];

    // buffer
    int *buffer;
    int *classification[SSD_MAX_PRIORBOX_LAYER_NUM];// 分类数据
    int *regression[SSD_MAX_PRIORBOX_LAYER_NUM];// 回归
    int *priorboxOutputData;
    int *softMaxOutputData;
    int *getResultBuffer;
    int *dstScore;
    int *dstRoi;
    int *classRoiNum;
    _SSDParameter():srcImageHeight(0),
                    srcImageWidth(0),
                    offset(0.0),
                    softMaxInHeight(0),
                    concatNum(0),
                    softMaxOutWidth(0),
                    softMaxOutHeight(0),
                    softMaxOutChn(0),
                    buffer(NULL),
                    priorboxOutputData(NULL),
                    softMaxOutputData(NULL),
                    getResultBuffer(NULL),
                    dstScore(NULL),
                    dstRoi(NULL),
                    classRoiNum(NULL){}
}SSDParameter;

typedef struct _QuickSortStack
{
    int min;
    int max;
}QuickSortStack;

class DLL_EXPORTS DetectorSSD : public DetectorInterface
{
public:
    DetectorSSD();
    virtual ~DetectorSSD();

    virtual ErrorCode Initialize(InitializationParameterOfDetector initializationParameterOfDetector);

    virtual ErrorCode Detect(const Mat &srcImage, std::vector<ResultOfDetection> &resultsOfDetection);

protected:
    // 添加4个新层:PermuteLayer,PriorBoxLayer,SoftmaxLayer,DetectionOutputLayer
    vector<float> PermuteLayer(const vector<float> &data,int width,int height,int channels);
    void PriorBoxLayer(int indexOfLayer,int* priorboxOutputData);
    void SoftmaxLayer(int softMaxWidth[],int* softMaxInputData[], int* softMaxOutputData);
    void DetectionOutputLayer(int* allLocPreds[], int* allPriorBoxes[],int* confScores, int* assistMemPool);
    
    // 获取SSD参数,主要包括PriorBoxLayer,SoftmaxLayer和DetectionOutputLayer层的参数
    void GetSSDParameter();

    void GetResult(const vector<vector<float>> &classification,const vector<vector<float>> &regression,vector<ResultOfDetection> &resultsOfDetection);

    // SoftmaxLayer
    void ComputeSoftMax(int* src, int size, int* dst);

    // DetectionOutputLayer
    void QuickSort(int* src,int low, int high, QuickSortStack *stack,int maxNum);
    void NonMaxSuppression( int* proposals, int anchorsNum,int NMSThresh,int maxRoiNum);
    void Swap(int* src1, int* src2);
    void ComputeOverlap(int xMin1, int yMin1, int xMax1, int yMax1, int xMin2,int yMin2, int xMax2, int yMax2,  int* areaSum, int* areaInter);

    void CreateDetectionResults(std::vector<ResultOfDetection> &resultsOfDetection);

protected:
    cv::dnn::Net net;
    std::vector<String> outputNodeNames;

    float scale;
    float meanValue1,meanValue2,meanValue3;
    bool swapRB;

    int backendId;
    int targetId;

    SSDParameter ssdParameter;

};

}

#endif

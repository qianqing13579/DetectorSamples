#include "DetectorYOLOV5.h"
#include"CommonUtility.h"
#include"Filesystem.h"
#include"SimpleLog.h"

namespace QQ
{

DetectorYOLOV5::DetectorYOLOV5()
{

}

DetectorYOLOV5::~DetectorYOLOV5()
{

    
}

ErrorCode DetectorYOLOV5::Initialize(InitializationParameterOfDetector initializationParameterOfDetector)
{
    // 初始化(加载配置文件等)
    ErrorCode errorCodeOfCommonInitialization=DoCommonInitialization(initializationParameterOfDetector);
    if(errorCodeOfCommonInitialization!=OK)
    {
        LOG_ERROR(logFile,"fail to DoCommonInitialization\n");
        return errorCodeOfCommonInitialization;
    }
    LOG_INFO(logFile,"succeed to DoCommonInitialization\n");
    
    // 获取配置文件参数
    FileNode netNode = configurationFile["DetectorYOLOV5"];
    string modelPath=initializationParameter.parentPath+(string)netNode["ModelPath"];
    net = readNet(modelPath);
    if (net.empty())
    {
        LOG_ERROR(logFile,"fail to load model: %s\n",GetFileName(modelPath).c_str());
        return FAIL_TO_LOAD_MODEL;
    }
    LOG_INFO(logFile,"succeed to load model: %s\n",GetFileName(modelPath).c_str());

    // 参数设置
    yolov5Parameter.confidenceThreshold = (float)netNode["ConfidenceThreshold"];
    yolov5Parameter.nmsThreshold = (float)netNode["NMSThreshold"];
    yolov5Parameter.objectThreshold = (float)netNode["ObjectThreshold"];
    inputSize=cv::Size((int)netNode["InputWidth"],(int)netNode["InputHeight"]);
    yolov5Parameter.numberOfClasses=(int)netNode["NumberOfClasses"];
    yolov5Parameter.anchors={{10.0, 13.0, 16.0, 30.0, 33.0, 23.0},{30.0, 61.0, 62.0, 45.0, 59.0, 119.0},{116.0, 90.0, 156.0, 198.0, 373.0, 326.0}};// 每个检测层的anchor
    yolov5Parameter.stride={8.0,16.0,32.0}; // 每个检测层的步长
    yolov5Parameter.numberOfDetectionLayer=yolov5Parameter.anchors.size();
    yolov5Parameter.numberOfAnchors=yolov5Parameter.anchors[0].size()/2;
    
    // 计算每个检测层的大小
    for(int i=0;i<yolov5Parameter.numberOfDetectionLayer;++i)
    {
        yolov5Parameter.heightOfDetectionLayer.push_back(inputSize.height/yolov5Parameter.stride[i]);
        yolov5Parameter.widthOfDetectionLayer.push_back(inputSize.width/yolov5Parameter.stride[i]);
    }

    // 读取类别名
    string pathOfClassNameFile=(string)netNode["ClassNameFile"];
    if(!pathOfClassNameFile.empty())
    {
        ifstream classNameFile(pathOfClassNameFile);
        string line;
        while (getline(classNameFile, line))
        {
            classNames.push_back(line);
        }
    }
    else
    {
        classNames.resize(yolov5Parameter.numberOfClasses);
    }

    // debug
    LOG_INFO(logFile,"input size:%dx%d\n",inputSize.width,inputSize.height);
    LOG_INFO(logFile,"ConfidenceThreshold:%f\n",yolov5Parameter.confidenceThreshold);
    LOG_INFO(logFile,"NMSThreshold:%f\n",yolov5Parameter.nmsThreshold);
    LOG_INFO(logFile,"ObjectThreshold:%f\n",yolov5Parameter.objectThreshold);
    LOG_INFO(logFile,"numberOfClasses:%d\n",yolov5Parameter.numberOfClasses);

    return OK;

}

void DetectorYOLOV5::sigmoid(Mat* out, int length)
{
    float* pdata = (float*)(out->data);
    int i = 0;
    for (i = 0; i < length; i++)
    {
        pdata[i] = 1.0 / (1 + expf(-pdata[i]));
    }
}


ErrorCode DetectorYOLOV5::Detect(const Mat &srcImage, std::vector<ResultOfDetection> &resultsOfDetection)
{
    Mat blob;
    blobFromImage(srcImage, blob, 1 / 255.0, inputSize, Scalar(0, 0, 0), true, false);// 按照官方的数据预处理方式
    net.setInput(blob);
    vector<Mat> outs;
    net.forward(outs, net.getUnconnectedOutLayersNames());

    vector<int> classIds;
    vector<float> confidences;
    vector<Rect> boxes;
    std::vector<float> stride=yolov5Parameter.stride;
    
    float ratioh = (float)srcImage.rows / inputSize.height, ratiow = (float)srcImage.cols / inputSize.width;
    int channelsOfEachAnchor = 4 + 1 + yolov5Parameter.numberOfClasses;
    
    // 对每个检测层
    for (int n = 0; n < yolov5Parameter.numberOfDetectionLayer; ++n) 
    {
        int widthOfDetectionLayer = yolov5Parameter.widthOfDetectionLayer[n];
        int heightOfDetectionLayer = yolov5Parameter.heightOfDetectionLayer[n];
        int sizeOfEachChannel = widthOfDetectionLayer * heightOfDetectionLayer;

        // 计算sigmoid
        sigmoid(&outs[n], yolov5Parameter.numberOfAnchors * channelsOfEachAnchor * sizeOfEachChannel);

        // 处理每个检测层的每个anchor
        for (int q = 0; q < yolov5Parameter.numberOfAnchors; ++q) 
        {
            const float anchor_w = yolov5Parameter.anchors[n][q * 2];
            const float anchor_h = yolov5Parameter.anchors[n][q * 2 + 1];
            float* pdata = (float*)outs[n].data + q * channelsOfEachAnchor * sizeOfEachChannel;
            for (int i = 0; i < heightOfDetectionLayer; ++i)
            {
                for (int j = 0; j < widthOfDetectionLayer; ++j)
                {
                    float box_score = pdata[4 * sizeOfEachChannel + i * widthOfDetectionLayer + j];
                    if (box_score > yolov5Parameter.objectThreshold)
                    {
                        float max_class_socre = 0, class_socre = 0;
                        int max_class_id = 0;
                        for (int c = 0; c < yolov5Parameter.numberOfClasses; ++c) //// get max socre
                        {
                            class_socre = pdata[(c + 5) * sizeOfEachChannel + i * widthOfDetectionLayer + j];
                            if (class_socre > max_class_socre)
                            {
                                max_class_socre = class_socre;
                                max_class_id = c;
                            }
                        }

                        if (max_class_socre > yolov5Parameter.confidenceThreshold)
                        {
                            float cx = (pdata[i * widthOfDetectionLayer + j] * 2.f - 0.5f + j) * stride[n];  ///cx
                            float cy = (pdata[sizeOfEachChannel + i * widthOfDetectionLayer + j] * 2.f - 0.5f + i) * stride[n];   ///cy
                            float w = powf(pdata[2 * sizeOfEachChannel + i * widthOfDetectionLayer + j] * 2.f, 2.f) * anchor_w;   ///w
                            float h = powf(pdata[3 * sizeOfEachChannel + i * widthOfDetectionLayer + j] * 2.f, 2.f) * anchor_h;  ///h

                            int left = (cx - 0.5*w)*ratiow;
                            int top = (cy - 0.5*h)*ratioh;

                            classIds.push_back(max_class_id);
                            confidences.push_back(max_class_socre);
                            boxes.push_back(Rect(left, top, (int)(w*ratiow), (int)(h*ratioh)));
                        }
                    }
                }
            }
        }
    }

    vector<int> indices;
    NMSBoxes(boxes, confidences, yolov5Parameter.confidenceThreshold, yolov5Parameter.nmsThreshold, indices);
    for (size_t i = 0; i < indices.size(); ++i)
    {
        int idx = indices[i];
        int classID=classIds[idx];
        string className=classNames[classID];
        float confidence=confidences[idx];
        Rect box = boxes[idx];

        ResultOfDetection result;
        result.boundingBox=box;
        result.confidence=confidence;//confidence
        result.classID=classID; //label
        result.className=className;
        resultsOfDetection.push_back(result);
    }

    return OK;
}

}

#include"SimpleLog.h"
#include"Filesystem.h"
#include"DetectorSSD.h"
#include"DetectorYOLOV3.h"
#include"DetectorYOLOV5.h"

using namespace QQ;

void Sample_DetectorSSD()
{
    // DetectorSSD
    DetectorSSD detector;
    InitializationParameterOfDetector initParamOfDetectorSSD;
    initParamOfDetectorSSD.parentPath = "";
    initParamOfDetectorSSD.configFilePath = CONFIG_FILE;
    initParamOfDetectorSSD.logName = "";
    ErrorCode errorCode=detector.Initialize(initParamOfDetectorSSD);
    if(errorCode!=OK)
    {
        LOG_ERROR(stdout, "fail to initialize detector!\n");
        exit(-1);
    }
    LOG_INFO(stdout, "succeed to initialize detector\n");

    // read image
    Mat srcImage=imread("../Resource/TestImage/SafetyHelmet.jpg",1);

    // detect
    std::vector<ResultOfDetection> predictions;
    double time1 = getTickCount();
    detector.Detect(srcImage,predictions);
    double time2 = getTickCount();
    double elapsedTime = (time2 - time1)*1000 / getTickFrequency();
    LOG_INFO(stdout, "time:%f ms\n", elapsedTime);

    LOG_INFO(stdout,"////////////////Detection Results////////////////\n");
    for(int i=0;i<predictions.size();++i)
    {
        ResultOfDetection result=predictions[i];
        cv::rectangle(srcImage,result.boundingBox,Scalar(0,255,255),2);
        
        LOG_INFO(stdout,"box:%d %d %d %d,label:%d,confidence:%f\n",predictions[i].boundingBox.x,
        predictions[i].boundingBox.y,predictions[i].boundingBox.width,predictions[i].boundingBox.height,predictions[i].classID,predictions[i].confidence);
    }
    imwrite("Result.jpg",srcImage);
    LOG_INFO(stdout,"Detection results have been saved to ./Result.jpg\n");
}


void Sample_DetectorYOLOV3()
{
    // DetectorYOLOV3
    DetectorYOLOV3 detector;
    InitializationParameterOfDetector initParamOfDetectorYOLOV3;
    initParamOfDetectorYOLOV3.parentPath = "";
    initParamOfDetectorYOLOV3.configFilePath = CONFIG_FILE;
    initParamOfDetectorYOLOV3.logName = "";
    ErrorCode errorCode=detector.Initialize(initParamOfDetectorYOLOV3);
    if(errorCode!=OK)
    {
        LOG_ERROR(stdout, "fail to initialize detector!\n");
        exit(-1);
    }
    LOG_INFO(stdout, "succeed to initialize detector\n");

    // read image
    Mat srcImage=imread("../Resource/TestImage/SafetyHelmet.jpg",1);

    // detect
    std::vector<ResultOfDetection> predictions;
    double time1 = getTickCount();
    detector.Detect(srcImage,predictions);
    double time2 = getTickCount();
    double elapsedTime = (time2 - time1)*1000 / getTickFrequency();
    LOG_INFO(stdout, "time:%f ms\n", elapsedTime);

    LOG_INFO(stdout,"////////////////Detection Results////////////////\n");
    for(int i=0;i<predictions.size();++i)
    {
        ResultOfDetection result=predictions[i];
        cv::rectangle(srcImage,result.boundingBox,Scalar(0,255,255),2);

        LOG_INFO(stdout,"box:%d %d %d %d,label:%d,confidence:%f\n",predictions[i].boundingBox.x,
        predictions[i].boundingBox.y,predictions[i].boundingBox.width,predictions[i].boundingBox.height,predictions[i].classID,predictions[i].confidence);
    }
    imwrite("Result.jpg",srcImage);
    LOG_INFO(stdout,"Detection results have been saved to ./Result.jpg\n");
}

void Sample_DetectorYOLOV5()
{
    // DetectorYOLOV5
    DetectorYOLOV5 detector;
    InitializationParameterOfDetector initParamOfDetectorYOLOV5;
    initParamOfDetectorYOLOV5.parentPath = "";
    initParamOfDetectorYOLOV5.configFilePath = CONFIG_FILE;
    initParamOfDetectorYOLOV5.logName = "";
    ErrorCode errorCode=detector.Initialize(initParamOfDetectorYOLOV5);
    if(errorCode!=OK)
    {
        LOG_ERROR(stdout, "fail to initialize detector!\n");
        exit(-1);
    }
    LOG_INFO(stdout, "succeed to initialize detector\n");

    // read image
    Mat srcImage=imread("../Resource/TestImage/bus.jpg",1);

    // detect
    std::vector<ResultOfDetection> predictions;
    double time1 = getTickCount();
    detector.Detect(srcImage,predictions);
    double time2 = getTickCount();
    double elapsedTime = (time2 - time1)*1000 / getTickFrequency();
    LOG_INFO(stdout, "time:%f ms\n", elapsedTime);

    LOG_INFO(stdout,"////////////////Detection Results////////////////\n");
    for(int i=0;i<predictions.size();++i)
    {
        ResultOfDetection result=predictions[i];
        cv::rectangle(srcImage,result.boundingBox,Scalar(0,255,255),2);

        LOG_INFO(stdout,"box:%d %d %d %d,label:%d,confidence:%f\n",predictions[i].boundingBox.x,
        predictions[i].boundingBox.y,predictions[i].boundingBox.width,predictions[i].boundingBox.height,predictions[i].classID,predictions[i].confidence);
    }
    imwrite("Result.jpg",srcImage);
    LOG_INFO(stdout,"Detection results have been saved to ./Result.jpg\n");
}
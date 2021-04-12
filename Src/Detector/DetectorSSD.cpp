#define DLLAPI_EXPORTS
#include "DetectorSSD.h"
#include "CommonUtility.h"
#include "Filesystem.h"
#include "SimpleLog.h"

namespace QQ
{

#define SSD_QUANT_BASE     4096 // 基数
#define SSD_COORDI_NUM     4  // 坐标个数(x1,y1,x2,y2)
#define SSD_PROPOSAL_WIDTH 6
#define SSD_HALF           0.5
#define SSD_ASPECT_RATIO_NUM 6 // 默认最大的宽高比个数

#define SSD_MAX(a,b)    (((a) > (b)) ? (a) : (b))
#define SSD_MIN(a,b)    (((a) < (b)) ? (a) : (b))

// 16字节对齐
#define SSD_ALIGN_16 16
#define SSD_ALIGN16(number) ((number + SSD_ALIGN_16-1) / SSD_ALIGN_16*SSD_ALIGN_16)

DetectorSSD::DetectorSSD(){}


DetectorSSD::~DetectorSSD()
{
    // 释放SSD参数的内存空间(在同一个模块中申请和释放)
    delete[] ssdParameter.buffer;
}


ErrorCode DetectorSSD::Initialize(InitializationParameterOfDetector initializationParameterOfDetector)
{
    ErrorCode errorCodeOfCommonInitialization=DoCommonInitialization(initializationParameterOfDetector);
    if(errorCodeOfCommonInitialization!=OK)
    {
        LOG_ERROR(logFile,"fail to DoCommonInitialization\n");
        return errorCodeOfCommonInitialization;
    }
    LOG_INFO(logFile,"succeed to DoCommonInitialization\n");

    FileNode rootNode = configurationFile["DetectorSSD"];

    string modelPath=initializationParameter.parentPath + (string)rootNode["ModelPath"];
    net = dnn::readNet(modelPath);// 加载onnx模型
    if (net.empty())
    {
        LOG_ERROR(logFile,"fail to load model: %s\n",GetFileName(modelPath).c_str());
        return FAIL_TO_LOAD_MODEL;
    }
    LOG_INFO(logFile,"succeed to load model: %s\n",modelPath.c_str());
    inputWidth=(int)rootNode["InputWidth"];
    inputHeight=(int)rootNode["InputHeight"];
    scale=(float)rootNode["Scale"];
    meanValue1=(float)rootNode["MeanValue1"];
    meanValue2=(float)rootNode["MeanValue2"];
    meanValue3=(float)rootNode["MeanValue3"];
    swapRB=(bool)(int)rootNode["SwapRB"];
    backendId=(int)rootNode["BackendId"];
    targetId=(int)rootNode["TargetId"];

    /* 0: automatically (by default)
     * 1: Halide language (http://halide-lang.org/)
     * 2: Intel's Deep Learning Inference Engine (https://software.intel.com/openvino-toolkit)
     * 3: OpenCV implementation
     */
    net.setPreferableBackend(backendId);

    /* 0: CPU target (by default)
     * 1: OpenCL
     * 2: OpenCL fp16 (half-float precision)
     * 3: VPU
     */
    net.setPreferableTarget(targetId);

    // debug
    LOG_INFO(logFile,"input size:%dx%d\n",inputWidth,inputHeight);
    LOG_INFO(logFile,"scale:%.6f\n",scale);
    LOG_INFO(logFile,"mean:%.2f,%.2f,%.2f\n",meanValue1,meanValue2,meanValue3);
    LOG_INFO(logFile,"swapRB:%d\n",(int)swapRB);
    LOG_INFO(logFile,"backendId:%d,targetId:%d\n",backendId,targetId);

    // 获取SSD参数
    GetSSDParameter();

    return OK;
}


ErrorCode DetectorSSD::Detect(const Mat &srcImage, std::vector<ResultOfDetection> &resultsOfDetection)
{
    // input blob
    Mat inputBlob;
	blobFromImage(srcImage,
                  inputBlob,
                  scale,
                  Size(inputWidth,inputHeight),
                  Scalar(meanValue1,meanValue2,meanValue3),
                  swapRB, false);
    net.setInput(inputBlob);

    // run model
    std::vector<Mat> outs;
    net.forward(outs,net.getUnconnectedOutLayersNames());
    
    // 转换Mat->vector<float>
    vector<vector<float>> regressions;
    vector<vector<float>> classifications;
    for(int i=0;i<ssdParameter.numberOfPriorBoxLayer;++i)
    {
        int numberOfPriorBox=ssdParameter.detectInputChn[i]/(4*(ssdParameter.priorBoxHeight[i] * ssdParameter.priorBoxWidth[i]));

        // regression
        {
            vector<float> regression;
            float *data=(float *)outs[2*i].data;
            int numberOfElement=outs[2*i].size[0]*outs[2*i].size[1]*outs[2*i].size[2]*outs[2*i].size[3];
            for(int j=0;j<numberOfElement;++j)
            {
               regression.push_back(data[j]);
            }
            regression=PermuteLayer(regression,ssdParameter.priorBoxWidth[i],ssdParameter.priorBoxHeight[i],numberOfPriorBox*4);
            regressions.push_back(regression);
        }

        // classification
        {
            vector<float> classification;
            float *data=(float *)outs[2*i+1].data;
            int numberOfElement=outs[2*i+1].size[0]*outs[2*i+1].size[1]*outs[2*i+1].size[2]*outs[2*i+1].size[3];
            for(int j=0;j<numberOfElement;++j)
            {
               classification.push_back(data[j]);
            }
            classification=PermuteLayer(classification,ssdParameter.priorBoxWidth[i],ssdParameter.priorBoxHeight[i],numberOfPriorBox*ssdParameter.classNum);
            classifications.push_back(classification);
        }
    }

    // 获取检测结果
    GetResult(classifications,regressions,resultsOfDetection);

    // 转换到原图坐标
    for(int i=0;i<resultsOfDetection.size();++i)
    {
        float ratioOfWidth=(1.0*srcImage.cols)/inputWidth;
        float ratioOfHeight=(1.0*srcImage.rows)/inputHeight;

        resultsOfDetection[i].boundingBox.x*=ratioOfWidth;
        resultsOfDetection[i].boundingBox.width*=ratioOfWidth;
        resultsOfDetection[i].boundingBox.y*=ratioOfHeight;
        resultsOfDetection[i].boundingBox.height*=ratioOfHeight;
    }

    // 按照置信度排序
    sort(resultsOfDetection.begin(), resultsOfDetection.end(),CompareConfidence);

    return OK;
}

void DetectorSSD::GetSSDParameter()
{
    FileNode rootNode = configurationFile["DetectorSSD"];
    ssdParameter.numberOfPriorBoxLayer=(int)rootNode["PriorBoxLayerNumber"];

    ssdParameter.srcImageHeight = inputHeight;
    ssdParameter.srcImageWidth = inputWidth;

    // MinSize,MaxSize
    ssdParameter.priorBoxMinSize.resize(ssdParameter.numberOfPriorBoxLayer);
    ssdParameter.priorBoxMaxSize.resize(ssdParameter.numberOfPriorBoxLayer);
    for (int i = 0; i < ssdParameter.numberOfPriorBoxLayer; ++i)
	{
		char nodeName[256] = { 0 };

		// miniSize
        {
            int j=0;
            while(true)
            {
               sprintf(nodeName, "MinSize%d%d", (i + 1),++j);
               FileNode miniSizeNode = rootNode[nodeName];
               if(miniSizeNode.empty())
               {
                   break;
               }
               else
               {
                 ssdParameter.priorBoxMinSize[i].push_back((float)rootNode[nodeName]);
               }
            }
        }

		// maxSize
        {
            int j=0;
            while(true)
            {
               sprintf(nodeName, "MaxSize%d%d", (i + 1),++j);
               FileNode maxSizeNode = rootNode[nodeName];
               if(maxSizeNode.empty())
               {
                   break;
               }
               else
               {
                 ssdParameter.priorBoxMaxSize[i].push_back((float)rootNode[nodeName]);
               }
            }
        }
	}

    // MinSizeNumber,MaxSizeNumber
    for (int i = 0; i < ssdParameter.numberOfPriorBoxLayer; ++i)
    {
        ssdParameter.minSizeNum[i] = ssdParameter.priorBoxMinSize[i].size();
        ssdParameter.maxSizeNum[i] = ssdParameter.priorBoxMaxSize[i].size();;
    }

    for (int i = 0; i < ssdParameter.numberOfPriorBoxLayer; ++i)
	{
		char nodeName[256] = { 0 };

		// Flip
		sprintf(nodeName, "Flip%d", i + 1);
		int flip = (int)rootNode[nodeName];
		ssdParameter.flip[i] = flip;

		// Clip
		sprintf(nodeName, "Clip%d", i + 1);
		int clip = (int)rootNode[nodeName];
		ssdParameter.clip[i] = clip;
	}

    ssdParameter.priorBoxAspectRatio.resize(ssdParameter.numberOfPriorBoxLayer);
    for (int i = 0; i < ssdParameter.numberOfPriorBoxLayer; ++i)
	{
        char nodeName[256] = { 0 };
        int j=0;
        while(true)
        {
           sprintf(nodeName, "AspectRatio%d%d", (i + 1),++j);
           FileNode aspectRatioNode = rootNode[nodeName];
           if(aspectRatioNode.empty())
           {
               break;
           }
           else
           {
             ssdParameter.priorBoxAspectRatio[i].push_back((float)rootNode[nodeName]);
           }
        }
    }

    // aspect ratio number
    for (int i = 0; i < ssdParameter.numberOfPriorBoxLayer; ++i)
    {
        ssdParameter.inputAspectRatioNum[i] = ssdParameter.priorBoxAspectRatio[i].size();
    }

    for (int i = 0; i < ssdParameter.numberOfPriorBoxLayer; ++i)
	{
		char nodeName[256] = { 0 };

		// width
		sprintf(nodeName, "PriorBoxStepWidth%d", i + 1);
		int width = (int)rootNode[nodeName];
		ssdParameter.priorBoxStepWidth[i] = width;

		// height
		sprintf(nodeName, "PriorBoxStepHeight%d", i + 1);
		int height = (int)rootNode[nodeName];
		ssdParameter.priorBoxStepHeight[i] = height;

	}

    // PriorBoxWidth,PriorBoxHeight
    for (int i = 0; i < ssdParameter.numberOfPriorBoxLayer; ++i)
    {
        ssdParameter.priorBoxWidth[i] = ssdParameter.srcImageWidth/ssdParameter.priorBoxStepWidth[i];
        ssdParameter.priorBoxHeight[i] = ssdParameter.srcImageHeight/ssdParameter.priorBoxStepHeight[i];
    }

    ssdParameter.offset = (float)rootNode["Offset"];

    ssdParameter.priorBoxVar[0] = (int)(0.1f*SSD_QUANT_BASE);
    ssdParameter.priorBoxVar[1] = (int)(0.1f*SSD_QUANT_BASE);
    ssdParameter.priorBoxVar[2] = (int)(0.2f*SSD_QUANT_BASE);
    ssdParameter.priorBoxVar[3] = (int)(0.2f*SSD_QUANT_BASE);

    int classNumber = (int)rootNode["ClassNumber"];
    ssdParameter.softMaxInHeight = classNumber;

    ssdParameter.concatNum = ssdParameter.numberOfPriorBoxLayer;
    ssdParameter.softMaxOutWidth = 1;
    ssdParameter.softMaxOutHeight = classNumber;

    int totalSizeOfClasReg=0;// 分类和回归一共需要的内存空间大小
	for (int i = 0; i < ssdParameter.numberOfPriorBoxLayer; ++i)
	{
        int priorBoxNumber=0;
        priorBoxNumber+=1;// aspect ratio=1
        for (int j = 0; j < ssdParameter.inputAspectRatioNum[i]; j++)
        {
            ++priorBoxNumber;
            if (ssdParameter.flip[j]==1)
            {
                ++priorBoxNumber;
            }
        }
        priorBoxNumber = ssdParameter.minSizeNum[i] * priorBoxNumber + ssdParameter.maxSizeNum[i];

		int totalPriorBoxNumber = priorBoxNumber*ssdParameter.priorBoxHeight[i] * ssdParameter.priorBoxWidth[i];
		ssdParameter.softMaxInChn[i] = totalPriorBoxNumber * classNumber;
		ssdParameter.softMaxOutChn += totalPriorBoxNumber;
		ssdParameter.detectInputChn[i] = totalPriorBoxNumber * 4;

        totalSizeOfClasReg+=(ssdParameter.softMaxInChn[i]+ssdParameter.detectInputChn[i]);

	}

    // DetectionOut
    ssdParameter.classNum = classNumber;
    ssdParameter.topK = (int)rootNode["TopK"];;
    ssdParameter.keepTopK = (int)rootNode["KeepTopK"];
    ssdParameter.NMSThresh = (int)((float)rootNode["NMSThreshold"]* SSD_QUANT_BASE);
    ssdParameter.confThresh=(int)((float)rootNode["ConfidenceThreshold"]*SSD_QUANT_BASE);
    
    for (int i = 0; i < ssdParameter.numberOfPriorBoxLayer ; i++)
    {
        int numberOfPriorBox=ssdParameter.detectInputChn[i]/(4*(ssdParameter.priorBoxHeight[i] * ssdParameter.priorBoxWidth[i]));

        ssdParameter.convHeight[2*i]=ssdParameter.priorBoxHeight[i];
        ssdParameter.convWidth[2*i]=ssdParameter.priorBoxWidth[i];
        ssdParameter.convChannel[2*i]=numberOfPriorBox*4;

        ssdParameter.convHeight[2*i+1]=ssdParameter.priorBoxHeight[i];
        ssdParameter.convWidth[2*i+1]=ssdParameter.priorBoxWidth[i];
        ssdParameter.convChannel[2*i+1]=numberOfPriorBox*ssdParameter.classNum;

        ssdParameter.convStride[i] = SSD_ALIGN16(ssdParameter.convChannel[2*i+1] * sizeof(int)) / sizeof(int);
    }

    // 计算softMaxOutputData内存空间大小
    int softMaxSize=0;
    for(int i = 0; i < ssdParameter.concatNum; i++)
    {
        softMaxSize += ssdParameter.softMaxInChn[i];
    }

    // 计算getResultBuffer内存空间大小
    int priorNum = 0;
    int detectionSize = 0;
    for(int i = 0; i < ssdParameter.concatNum; i++)
    {
        priorNum+=ssdParameter.detectInputChn[i]/SSD_COORDI_NUM;
    }
    detectionSize+=priorNum*SSD_COORDI_NUM;
    detectionSize+=priorNum*SSD_PROPOSAL_WIDTH*2;
    detectionSize+=priorNum*2;

    // 计算dstRoi,classRoiNum,dstScore内存空间大小
    int dstRoiSize = 0;
    int dstScoreSize = 0;
    int classRoiNumSize = 0;
    dstRoiSize = SSD_ALIGN16(ssdParameter.classNum*ssdParameter.topK*SSD_COORDI_NUM);
    dstScoreSize = SSD_ALIGN16(ssdParameter.classNum*ssdParameter.topK);
    classRoiNumSize = SSD_ALIGN16(ssdParameter.classNum);

    // 申请内存，并分配
    int totalSize=totalSizeOfClasReg+SSD_COORDI_NUM*2*ssdParameter.softMaxOutChn+softMaxSize+detectionSize+dstRoiSize+classRoiNumSize+dstScoreSize;
    ssdParameter.buffer=new int[totalSize];
    int *data=ssdParameter.buffer;
    memset(data,0,totalSize*sizeof(int));// 初始化0
    int offset=0;
    for (int i = 0; i < ssdParameter.numberOfPriorBoxLayer; ++i)
	{
        
        int *dataOfClasReg=data+offset;
        ssdParameter.classification[i]=dataOfClasReg;
        ssdParameter.regression[i]=dataOfClasReg+ssdParameter.softMaxInChn[i];

        offset+=(ssdParameter.softMaxInChn[i]+ssdParameter.detectInputChn[i]);

    }
    ssdParameter.priorboxOutputData=data+totalSizeOfClasReg;
    ssdParameter.softMaxOutputData=ssdParameter.priorboxOutputData+SSD_COORDI_NUM*2*ssdParameter.softMaxOutChn;
    ssdParameter.getResultBuffer=ssdParameter.softMaxOutputData+softMaxSize;
    ssdParameter.dstRoi=ssdParameter.getResultBuffer+detectionSize;
    ssdParameter.classRoiNum=ssdParameter.dstRoi+dstRoiSize;
    ssdParameter.dstScore=ssdParameter.classRoiNum+classRoiNumSize;

}

void DetectorSSD::GetResult(const vector<vector<float>> &classifications,const vector<vector<float>> &regressions,vector<ResultOfDetection> &resultsOfDetection)
{
    int numberOfPriorBoxLayer=ssdParameter.numberOfPriorBoxLayer;

    // 类型转换
    for(int i = 0; i < numberOfPriorBoxLayer; i++)
    {
        // 分类
        vector<float> classificationOfEachLayer=classifications[i];
        for(int j=0;j<classificationOfEachLayer.size();++j)
        {
            (ssdParameter.classification[i])[j]=classificationOfEachLayer[j]*SSD_QUANT_BASE;
        }

        // 回归
        vector<float> regressionOfEachLayer=regressions[i];
        for(int j=0;j<regressionOfEachLayer.size();++j)
        {
            (ssdParameter.regression[i])[j]=regressionOfEachLayer[j]*SSD_QUANT_BASE;
        }

    }

    int* priorboxOutputData[SSD_MAX_PRIORBOX_LAYER_NUM];
    int* softMaxInputData[SSD_MAX_PRIORBOX_LAYER_NUM];
    int* detectionLocData[SSD_MAX_PRIORBOX_LAYER_NUM];
    int* softMaxOutputData = NULL;
    int* detectionOutTmpBuf = NULL;
    int  softMaxWidth[SSD_MAX_PRIORBOX_LAYER_NUM];
    int size = 0;
    int i = 0;

    /////////////////////////////////// PriorBoxLayer：生成所有priorbox ///////////////////////////////////
    // 分配priorboxOutputData内存空间
    priorboxOutputData[0] = ssdParameter.priorboxOutputData;
    for (i = 1; i < numberOfPriorBoxLayer; i++)
    {
        size=ssdParameter.softMaxInChn[i-1]/ssdParameter.classNum*SSD_COORDI_NUM*2;
        priorboxOutputData[i] = priorboxOutputData[i - 1] + size;
    }
    for (i = 0; i < numberOfPriorBoxLayer; i++)
    {
        PriorBoxLayer(i,priorboxOutputData[i]);
    }

    /////////////////////////////////// SoftmaxLayer：计算所有priorbox的置信度 ///////////////////////////////////
    // 分配softMaxOutputData内存空间
    softMaxOutputData =ssdParameter.softMaxOutputData;
    for(i = 0; i < numberOfPriorBoxLayer; i++)
    {
        softMaxInputData[i] = ssdParameter.classification[i];
        softMaxWidth[i] = ssdParameter.convChannel[i*2+1];
    }
    SoftmaxLayer(softMaxWidth,softMaxInputData, softMaxOutputData);

    /////////////////////////////////// DetectionOutputLayer：对网络输出值解码并经过NMS得到最后的检测结果 ///////////////////////////////////
    // 分配DetectionOut内存空间
    detectionOutTmpBuf = ssdParameter.getResultBuffer;
    for(i = 0; i < numberOfPriorBoxLayer; i++)
    {
        detectionLocData[i] = ssdParameter.regression[i];
    }
    DetectionOutputLayer(detectionLocData, priorboxOutputData, softMaxOutputData,detectionOutTmpBuf);

    // 获取最后的检测结果
    CreateDetectionResults(resultsOfDetection);
}


void DetectorSSD::PriorBoxLayer(int indexOfLayer,int* priorboxOutputData)
{
    // 参数赋值
    int priorBoxWidth=ssdParameter.priorBoxWidth[indexOfLayer];
    int priorBoxHeight=ssdParameter.priorBoxHeight[indexOfLayer];
    int srcImageWidth=ssdParameter.srcImageWidth;
    int srcImageHeight=ssdParameter.srcImageHeight;
    vector<float> priorBoxMinSize=ssdParameter.priorBoxMinSize[indexOfLayer];
    int minSizeNum=ssdParameter.minSizeNum[indexOfLayer];
    vector<float> priorBoxMaxSize=ssdParameter.priorBoxMaxSize[indexOfLayer];
    int maxSizeNum=ssdParameter.maxSizeNum[indexOfLayer];
    int flip=ssdParameter.flip[indexOfLayer];
    int clip=ssdParameter.clip[indexOfLayer];
    int inputAspectRatioNum=ssdParameter.inputAspectRatioNum[indexOfLayer];
    vector<float> priorBoxAspectRatio=ssdParameter.priorBoxAspectRatio[indexOfLayer];
    float priorBoxStepWidth=ssdParameter.priorBoxStepWidth[indexOfLayer];
    float priorBoxStepHeight= ssdParameter.priorBoxStepHeight[indexOfLayer];
    float offset=ssdParameter.offset;
    int *priorBoxVar=ssdParameter.priorBoxVar;

    int aspectRatioNum = 0;
    int index = 0;
    float aspectRatio[SSD_ASPECT_RATIO_NUM] = { 0 };
    int numPrior = 0;
    float centerX = 0;
    float centerY = 0;
    float boxHeight = 0;
    float boxWidth = 0;
    float maxBoxWidth = 0;
    int i = 0;
    int j = 0;
    int n = 0;
    int h = 0;
    int w = 0;

    aspectRatioNum = 0;
    aspectRatio[0] = 1;
    aspectRatioNum++;
    for (i = 0; i < inputAspectRatioNum; i++)
    {
        aspectRatio[aspectRatioNum++] = priorBoxAspectRatio[i];
        if (flip)
        {
            aspectRatio[aspectRatioNum++] = 1.0f / priorBoxAspectRatio[i];
        }
    }
    numPrior = minSizeNum * aspectRatioNum + maxSizeNum;

    index = 0;
    for (h = 0; h < priorBoxHeight; h++)
    {
        for (w = 0; w < priorBoxWidth; w++)
        {
            centerX = (w + offset) * priorBoxStepWidth;
            centerY = (h + offset) * priorBoxStepHeight;
            for (n = 0; n < minSizeNum; n++)
            {
                // 首先产生宽高比为1的priorbox
                boxHeight = priorBoxMinSize[n];
                boxWidth = priorBoxMinSize[n];
                priorboxOutputData[index++] = (int)(centerX - boxWidth * SSD_HALF);
                priorboxOutputData[index++] = (int)(centerY - boxHeight * SSD_HALF);
                priorboxOutputData[index++] = (int)(centerX + boxWidth * SSD_HALF);
                priorboxOutputData[index++] = (int)(centerY + boxHeight * SSD_HALF);

                // 对于max_size,生成宽高比为1的priorbox,宽高为sqrt(min_size * max_size)
                if(maxSizeNum>0)
                {
                    maxBoxWidth = sqrt(priorBoxMinSize[n] * priorBoxMaxSize[n]);
                    boxHeight = maxBoxWidth;
                    boxWidth = maxBoxWidth;
                    priorboxOutputData[index++] = (int)(centerX - boxWidth * SSD_HALF);
                    priorboxOutputData[index++] = (int)(centerY - boxHeight * SSD_HALF);
                    priorboxOutputData[index++] = (int)(centerX + boxWidth * SSD_HALF);
                    priorboxOutputData[index++] = (int)(centerY + boxHeight * SSD_HALF);
                }

                // 剩下的priorbox
                for (i = 1; i < aspectRatioNum; i++)
                {
                    boxWidth = (float)(priorBoxMinSize[n] * sqrt( aspectRatio[i] ));
                    boxHeight = (float)(priorBoxMinSize[n]/sqrt( aspectRatio[i] ));

                    priorboxOutputData[index++] = (int)(centerX - boxWidth * SSD_HALF);
                    priorboxOutputData[index++] = (int)(centerY - boxHeight * SSD_HALF);
                    priorboxOutputData[index++] = (int)(centerX + boxWidth * SSD_HALF);
                    priorboxOutputData[index++] = (int)(centerY + boxHeight * SSD_HALF);
                }
            }
        }
    }

    // 越界处理 [0, srcImageWidth] & [0, srcImageHeight] 
    if (clip)
    {
        for (i = 0; i < (int)(priorBoxWidth * priorBoxHeight * SSD_COORDI_NUM*numPrior / 2); i++)
        {
            priorboxOutputData[2 * i] = SSD_MIN((int)SSD_MAX(priorboxOutputData[2 * i], 0), srcImageWidth);
            priorboxOutputData[2 * i + 1] = SSD_MIN((int)SSD_MAX(priorboxOutputData[2 * i + 1], 0), srcImageHeight);
        }
    }
    // var
    for (h = 0; h < priorBoxHeight; h++)
    {
        for (w = 0; w < priorBoxWidth; w++)
        {
            for (i = 0; i < numPrior; i++)
            {
                for (j = 0; j < SSD_COORDI_NUM; j++)
                {
                    priorboxOutputData[index++] = (int)priorBoxVar[j];
                }
            }
        }
    }
}

void DetectorSSD::SoftmaxLayer(int softMaxWidth[],int* softMaxInputData[], int* softMaxOutputData)
{

    // 参数赋值
    int softMaxInHeight=ssdParameter.softMaxInHeight;
    int *softMaxInChn=ssdParameter.softMaxInChn;
    int concatNum=ssdParameter.concatNum;
    int *convStride=ssdParameter.convStride;

    int* inputData = NULL;
    int* outputTmp = NULL;
    int outerNum = 0;
    int innerNum = 0;
    int inputChannel = 0;
    int i = 0;
    int concatCnt = 0;
    int stride = 0;
    int skip = 0;
    int left = 0;
    outputTmp = softMaxOutputData;
    for (concatCnt = 0; concatCnt < concatNum; concatCnt++)
    {
        inputData = softMaxInputData[concatCnt];
        stride = convStride[concatCnt];
        inputChannel = softMaxInChn[concatCnt];
        outerNum = inputChannel / softMaxInHeight;
        innerNum = softMaxInHeight;
        skip = softMaxWidth[concatCnt] / innerNum;// priorbox number
        left = stride - softMaxWidth[concatCnt];
        for (i = 0; i < outerNum; i++)
        {
            ComputeSoftMax(inputData, (int)innerNum,outputTmp);

            inputData += innerNum;
            outputTmp += innerNum;
        }
    }
}

void DetectorSSD::ComputeSoftMax(int* src, int size, int* dst)
{
    int max = 0;
    int sum = 0;
    int i = 0;
    for (i = 0; i < size; ++i)
    {
        if (max < src[i])
        {
            max = src[i];
        }
    }
    for (i = 0; i < size; ++i)
    {
        dst[i] = (int)(SSD_QUANT_BASE* exp((float)(src[i] - max) / SSD_QUANT_BASE));
        sum += dst[i];
    }
    for (i = 0; i < size; ++i)
    {
        dst[i] = (int)(((float)dst[i] / (float)sum) * SSD_QUANT_BASE);
    }

}

void DetectorSSD::DetectionOutputLayer(int* allLocPreds[], int* allPriorBoxes[],int* confScores, int* assistMemPool)
{

    // 参数赋值
    int concatNum=ssdParameter.concatNum;
    int confThresh=ssdParameter.confThresh;
    int classNum=ssdParameter.classNum;
    int topK=ssdParameter.topK;
    int keepTopK=ssdParameter.keepTopK;
    int NMSThresh=ssdParameter.NMSThresh;
    int *detectInputChn=ssdParameter.detectInputChn;
    int* dstScoreSrc=ssdParameter.dstScore;
    int* dstBboxSrc=ssdParameter.dstRoi;
    int* roiOutCntSrc=ssdParameter.classRoiNum;

    int* locPreds = NULL;
    int* priorBoxes = NULL;
    int* priorVar = NULL;
    int* allDecodeBoxes = NULL;
    int* dstScore = NULL;
    int* dstBbox = NULL;
    int* classRoiNum = NULL;
    int roiOutCnt = 0;
    int* singleProposal = NULL;
    int* afterTopK = NULL;
    QuickSortStack* stack = NULL;
    int priorNum = 0;
    int numPredsPerClass = 0;
    float priorWidth = 0;
    float priorHeight = 0;
    float priorCenterX = 0;
    float priorCenterY = 0;
    float decodeBoxCenterX = 0;
    float decodeBoxCenterY = 0;
    float decodeBoxWidth = 0;
    float decodeBoxHeight = 0;
    int srcIdx = 0;
    int afterFilter = 0;
    int afterTopK2 = 0;
    int keepCnt = 0;
    int i = 0;
    int j = 0;
    int offset = 0;
    priorNum = 0;
    for (i = 0; i < concatNum; i++)
    {
        priorNum += detectInputChn[i] / SSD_COORDI_NUM;
    }

    // 缓存
    allDecodeBoxes = assistMemPool;
    singleProposal = allDecodeBoxes + priorNum * SSD_COORDI_NUM;
    afterTopK = singleProposal + SSD_PROPOSAL_WIDTH * priorNum;
    stack = (QuickSortStack*)(afterTopK + priorNum * SSD_PROPOSAL_WIDTH);
    srcIdx = 0;
    for (i = 0; i < concatNum; i++)
    {
        // 回归预测值
        locPreds = allLocPreds[i];
        numPredsPerClass = detectInputChn[i] / SSD_COORDI_NUM;

        // 获取priorbox
        priorBoxes = allPriorBoxes[i];
        priorVar = priorBoxes + numPredsPerClass*SSD_COORDI_NUM;
        for (j = 0; j < numPredsPerClass; j++)
        {
            priorWidth = (float)(priorBoxes[j*SSD_COORDI_NUM+2] - priorBoxes[j*SSD_COORDI_NUM]);
            priorHeight = (float)(priorBoxes[j*SSD_COORDI_NUM+3] - priorBoxes[j*SSD_COORDI_NUM + 1]);
            priorCenterX = (priorBoxes[j*SSD_COORDI_NUM+2] + priorBoxes[j*SSD_COORDI_NUM])*SSD_HALF;
            priorCenterY = (priorBoxes[j*SSD_COORDI_NUM+3] + priorBoxes[j*SSD_COORDI_NUM+1])*SSD_HALF;

            decodeBoxCenterX = ((float)priorVar[j*SSD_COORDI_NUM]/SSD_QUANT_BASE)*
                ((float)locPreds[j*SSD_COORDI_NUM]/SSD_QUANT_BASE)*priorWidth+priorCenterX;

            decodeBoxCenterY = ((float)priorVar[j*SSD_COORDI_NUM+1]/SSD_QUANT_BASE)*
                ((float)locPreds[j*SSD_COORDI_NUM+1]/SSD_QUANT_BASE)*priorHeight+priorCenterY;

            decodeBoxWidth = exp(((float)priorVar[j*SSD_COORDI_NUM+2]/SSD_QUANT_BASE)*
                ((float)locPreds[j*SSD_COORDI_NUM+2]/SSD_QUANT_BASE))*priorWidth;

            decodeBoxHeight = exp(((float)priorVar[j*SSD_COORDI_NUM+3]/SSD_QUANT_BASE)*
                ((float)locPreds[j*SSD_COORDI_NUM+3]/SSD_QUANT_BASE))*priorHeight;

            allDecodeBoxes[srcIdx++] = (int)(decodeBoxCenterX - decodeBoxWidth * SSD_HALF);
            allDecodeBoxes[srcIdx++] = (int)(decodeBoxCenterY - decodeBoxHeight * SSD_HALF);
            allDecodeBoxes[srcIdx++] = (int)(decodeBoxCenterX + decodeBoxWidth * SSD_HALF);
            allDecodeBoxes[srcIdx++] = (int)(decodeBoxCenterY + decodeBoxHeight * SSD_HALF);
        }
    }

    // 对每一类做NMS
    afterTopK2 = 0;
    for (i = 0; i < classNum; i++)
    {
        if(i==0)
            continue;

        for (j = 0; j < priorNum; j++)
        {
            singleProposal[j * SSD_PROPOSAL_WIDTH] = allDecodeBoxes[j * SSD_COORDI_NUM];
            singleProposal[j * SSD_PROPOSAL_WIDTH + 1] = allDecodeBoxes[j * SSD_COORDI_NUM + 1];
            singleProposal[j * SSD_PROPOSAL_WIDTH + 2] = allDecodeBoxes[j * SSD_COORDI_NUM + 2];
            singleProposal[j * SSD_PROPOSAL_WIDTH + 3] = allDecodeBoxes[j * SSD_COORDI_NUM + 3];
            singleProposal[j * SSD_PROPOSAL_WIDTH + 4] = confScores[j*classNum + i];
            singleProposal[j * SSD_PROPOSAL_WIDTH + 5] = 0;
        }
        QuickSort(singleProposal, 0, priorNum - 1, stack,topK);
        afterFilter = (priorNum < topK) ? priorNum : topK;
        NonMaxSuppression(singleProposal, afterFilter, NMSThresh, afterFilter);
        roiOutCnt = 0;
        dstScore = (int*)dstScoreSrc;
        dstBbox = (int*)dstBboxSrc;
        classRoiNum = (int*)roiOutCntSrc;
        dstScore += (int)afterTopK2;
        dstBbox += (int)(afterTopK2 * SSD_COORDI_NUM);
        for (j = 0; j < topK; j++)
        {
            if (singleProposal[j * SSD_PROPOSAL_WIDTH + 5] == 0 &&
                singleProposal[j * SSD_PROPOSAL_WIDTH + 4] > (int)confThresh)
            {
                dstScore[roiOutCnt] = singleProposal[j * 6 + 4];
                dstBbox[roiOutCnt * SSD_COORDI_NUM] = singleProposal[j * SSD_PROPOSAL_WIDTH];
                dstBbox[roiOutCnt * SSD_COORDI_NUM + 1] = singleProposal[j * SSD_PROPOSAL_WIDTH + 1];
                dstBbox[roiOutCnt * SSD_COORDI_NUM + 2] = singleProposal[j * SSD_PROPOSAL_WIDTH + 2];
                dstBbox[roiOutCnt * SSD_COORDI_NUM + 3] = singleProposal[j * SSD_PROPOSAL_WIDTH + 3];
                roiOutCnt++;
            }
        }
        classRoiNum[i] = (int)roiOutCnt;
        afterTopK2 += roiOutCnt;
    }

    keepCnt = 0;
    offset = 0;
    if (afterTopK2 > keepTopK)
    {
        offset = classRoiNum[0];
        for (i = 1; i < classNum; i++)
        {
            dstScore = (int*)dstScoreSrc;
            dstBbox = (int*)dstBboxSrc;
            classRoiNum = (int*)roiOutCntSrc;
            dstScore += (int)(offset);
            dstBbox += (int)(offset * SSD_COORDI_NUM);
            for (j = 0; j < (int)classRoiNum[i]; j++)
            {
                afterTopK[keepCnt * SSD_PROPOSAL_WIDTH] = dstBbox[j * SSD_COORDI_NUM];
                afterTopK[keepCnt * SSD_PROPOSAL_WIDTH + 1] = dstBbox[j * SSD_COORDI_NUM + 1];
                afterTopK[keepCnt * SSD_PROPOSAL_WIDTH + 2] = dstBbox[j * SSD_COORDI_NUM + 2];
                afterTopK[keepCnt * SSD_PROPOSAL_WIDTH + 3] = dstBbox[j * SSD_COORDI_NUM + 3];
                afterTopK[keepCnt * SSD_PROPOSAL_WIDTH + 4] = dstScore[j];
                afterTopK[keepCnt * SSD_PROPOSAL_WIDTH + 5] = i;
                keepCnt++;
            }
            offset = offset + classRoiNum[i];
        }
        QuickSort(afterTopK, 0, keepCnt - 1, stack,keepCnt);

        offset = 0;
        offset = classRoiNum[0];
        for (i = 1; i < classNum; i++)
        {
            roiOutCnt = 0;
            dstScore = (int*)dstScoreSrc;
            dstBbox = (int*)dstBboxSrc;
            classRoiNum = (int*)roiOutCntSrc;
            dstScore += (int)(offset);
            dstBbox += (int)(offset * SSD_COORDI_NUM);
            for (j = 0; j < keepTopK; j++)
            {
                if (afterTopK[j * SSD_PROPOSAL_WIDTH + 5] == i)
                {
                    dstScore[roiOutCnt] = afterTopK[j * SSD_PROPOSAL_WIDTH + 4];
                    dstBbox[roiOutCnt * SSD_COORDI_NUM] = afterTopK[j * SSD_PROPOSAL_WIDTH];
                    dstBbox[roiOutCnt * SSD_COORDI_NUM + 1] = afterTopK[j * SSD_PROPOSAL_WIDTH + 1];
                    dstBbox[roiOutCnt * SSD_COORDI_NUM + 2] = afterTopK[j * SSD_PROPOSAL_WIDTH + 2];
                    dstBbox[roiOutCnt * SSD_COORDI_NUM + 3] = afterTopK[j * SSD_PROPOSAL_WIDTH + 3];
                    roiOutCnt++;
                }
            }
            classRoiNum[i] = (int)roiOutCnt;
            offset += roiOutCnt;
        }
    }
}

vector<float> DetectorSSD::PermuteLayer(const vector<float> &data,int width,int height,int channels)
{
    vector<float> result(data.size());
    int index=0;
    int channelStep=width*height;
    for(int h=0; h<height;h++)
    {
        for(int w=0;w<width;w++)
        {
            for(int c = 0;c < channels;c++)
            {
                result[index++] = data[c*channelStep + h*width + w];
            }
        }
    }
    return result;

}

void DetectorSSD::QuickSort(int* src,int low, int high, QuickSortStack *stack,int maxNum)
{
    int i = low;
    int j = high;
    int top = 0;
    int keyConfidence = src[SSD_PROPOSAL_WIDTH * low + 4];
    stack[top].min = low;
    stack[top].max = high;

    while(top > -1)
    {
        low = stack[top].min;
        high = stack[top].max;
        i = low;
        j = high;
        top--;

        keyConfidence = src[SSD_PROPOSAL_WIDTH * low + 4];

        while(i < j)
        {
            while((i < j) && (keyConfidence > src[j * SSD_PROPOSAL_WIDTH + 4]))
            {
                j--;
            }
            if(i < j)
            {
                Swap(&src[i*SSD_PROPOSAL_WIDTH], &src[j*SSD_PROPOSAL_WIDTH]);
                i++;
            }

            while((i < j) && (keyConfidence < src[i*SSD_PROPOSAL_WIDTH + 4]))
            {
                i++;
            }
            if(i < j)
            {
                Swap(&src[i*SSD_PROPOSAL_WIDTH], &src[j*SSD_PROPOSAL_WIDTH]);
                j--;
            }
        }

        if(low <= maxNum)
        {
                if(low < i-1)
                {
                    top++;
                    stack[top].min = low;
                    stack[top].max = i-1;
                }

                if(high > i+1)
                {
                    top++;
                    stack[top].min = i+1;
                    stack[top].max = high;
                }
        }
    }
}

void DetectorSSD::NonMaxSuppression( int* proposals, int anchorsNum,int NMSThresh,int maxRoiNum)
{
    int xMin1 = 0;
    int yMin1 = 0;
    int xMax1 = 0;
    int yMax1 = 0;
    int xMin2 = 0;
    int yMin2 = 0;
    int xMax2 = 0;
    int yMax2 = 0;
    int areaTotal = 0;
    int areaInter = 0;
    int i = 0;
    int j = 0;
    int num = 0;
    int NoOverlap  = 1;
    for (i = 0; i < anchorsNum && num < maxRoiNum; i++)
    {
        if( proposals[SSD_PROPOSAL_WIDTH*i+5] == 0 )
        {
            num++;
            xMin1 =  proposals[SSD_PROPOSAL_WIDTH*i];
            yMin1 =  proposals[SSD_PROPOSAL_WIDTH*i+1];
            xMax1 =  proposals[SSD_PROPOSAL_WIDTH*i+2];
            yMax1 =  proposals[SSD_PROPOSAL_WIDTH*i+3];
            for(j= i+1;j< anchorsNum; j++)
            {
                if( proposals[SSD_PROPOSAL_WIDTH*j+5] == 0 )
                {
                    xMin2 = proposals[SSD_PROPOSAL_WIDTH*j];
                    yMin2 = proposals[SSD_PROPOSAL_WIDTH*j+1];
                    xMax2 = proposals[SSD_PROPOSAL_WIDTH*j+2];
                    yMax2 = proposals[SSD_PROPOSAL_WIDTH*j+3];
                    NoOverlap = (xMin2>xMax1)||(xMax2<xMin1)||(yMin2>yMax1)||(yMax2<yMin1);
                    if(NoOverlap)
                    {
                        continue;
                    }
                    ComputeOverlap(xMin1, yMin1, xMax1, yMax1, xMin2, yMin2, xMax2, yMax2, &areaTotal, &areaInter);
                    if(areaInter*SSD_QUANT_BASE > ((int)NMSThresh*areaTotal))
                    {
                        if( proposals[SSD_PROPOSAL_WIDTH*i+4] >= proposals[SSD_PROPOSAL_WIDTH*j+4] )
                        {
                            proposals[SSD_PROPOSAL_WIDTH*j+5] = 1;
                        }
                        else
                        {
                            proposals[SSD_PROPOSAL_WIDTH*i+5] = 1;
                        }
                    }
                }
            }
        }
    }
}

void DetectorSSD::ComputeOverlap(int xMin1, int yMin1, int xMax1, int yMax1, int xMin2,
    int yMin2, int xMax2, int yMax2,  int* areaSum, int* areaInter)
{
    
    int inter = 0;
    int s32Total = 0;
    int xMin = 0;
    int yMin = 0;
    int xMax = 0;
    int yMax = 0;
    int area1 = 0;
    int area2 = 0;
    int interWidth = 0;
    int interHeight = 0;

    xMin = SSD_MAX(xMin1, xMin2);
    yMin = SSD_MAX(yMin1, yMin2);
    xMax = SSD_MIN(xMax1, xMax2);
    yMax = SSD_MIN(yMax1, yMax2);

    interWidth = xMax - xMin + 1;
    interHeight = yMax - yMin + 1;

    interWidth = ( interWidth >= 0 ) ? interWidth : 0;
    interHeight = ( interHeight >= 0 ) ? interHeight : 0;

    inter = interWidth * interHeight;
    area1 = (xMax1 - xMin1 + 1) * (yMax1 - yMin1 + 1);
    area2 = (xMax2 - xMin2 + 1) * (yMax2 - yMin2 + 1);

    s32Total = area1 + area2 - inter;

    *areaSum = s32Total;
    *areaInter = inter;
}

void DetectorSSD::Swap(int* src1, int* src2)
{
    int i = 0;
    int temp = 0;
    for( i = 0; i < SSD_PROPOSAL_WIDTH; i++ )
    {
        temp = src1[i];
        src1[i] = src2[i];
        src2[i] = temp;
    }
}

void DetectorSSD::CreateDetectionResults(std::vector<ResultOfDetection> &resultsOfDetection)
{
    // 参数赋值
    int* score=ssdParameter.dstScore;
    int* roi=ssdParameter.dstRoi;
    int* classRoiNum=ssdParameter.classRoiNum;
    float printResultThresh=((float)ssdParameter.confThresh)/SSD_QUANT_BASE;
    int classNum=ssdParameter.classNum;

    int i = 0, j = 0;
    int roiNumBias = 0;
    int scoreBias = 0;
    int bboxBias = 0;
    float score2 = 0.0f;
    int xMin = 0,yMin= 0,xMax = 0,yMax = 0;

    roiNumBias += classRoiNum[0];
    for (i = 1; i < classNum; i++)
    {
        scoreBias = roiNumBias;
        bboxBias = roiNumBias * SSD_COORDI_NUM;

        if((float)score[scoreBias] / SSD_QUANT_BASE >=
            printResultThresh && classRoiNum[i]!=0)
        {
            //printf("==== The %d th class box info====\n", i);
        }
        for (j = 0; j < (int)classRoiNum[i]; j++)
        {
            score2 = (float)score[scoreBias + j] / SSD_QUANT_BASE;
            if (score2 < printResultThresh)
            {
                break;
            }
            xMin = roi[bboxBias + j*SSD_COORDI_NUM];
            yMin = roi[bboxBias + j*SSD_COORDI_NUM + 1];
            xMax = roi[bboxBias + j*SSD_COORDI_NUM + 2];
            yMax = roi[bboxBias + j*SSD_COORDI_NUM + 3];

            ResultOfDetection result;
            result.boundingBox.x=xMin;
            result.boundingBox.y=yMin;
            result.boundingBox.width=xMax-xMin+1;
            result.boundingBox.height=yMax-yMin+1;
            result.classID=i;
            result.confidence=score2;
            resultsOfDetection.push_back(result);
        }
        roiNumBias += classRoiNum[i];
    }
}

}

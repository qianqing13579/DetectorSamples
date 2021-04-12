#define DLLAPI_EXPORTS
#include "DetectorYOLOV3.h"
#include"CommonUtility.h"
#include"Filesystem.h"
#include"SimpleLog.h"

namespace QQ
{
float DetectorYOLOV3::sigmoid(float x)
{
  return 1. / (1. + exp(-x));
}

float DetectorYOLOV3::overlap(float x1, float w1, float x2, float w2)
{
  float l1 = x1 - w1 / 2;
  float l2 = x2 - w2 / 2;
  float left = l1 > l2 ? l1 : l2;
  float r1 = x1 + w1 / 2;
  float r2 = x2 + w2 / 2;
  float right = r1 < r2 ? r1 : r2;
  return right - left;
}

float DetectorYOLOV3::box_intersection(vector<float> a, vector<float> b)
{
  float w = overlap(a[0], a[2], b[0], b[2]);
  float h = overlap(a[1], a[3], b[1], b[3]);
  if (w < 0 || h < 0) return 0;
  float area = w*h;
  return area;
}

float DetectorYOLOV3::box_union(vector<float> a, vector<float> b)
{
  float i = box_intersection(a, b);
  float u = a[2] * a[3] + b[2] * b[3] - i;
  return u;
}

float DetectorYOLOV3::box_iou(vector<float> a, vector<float> b)
{
  return box_intersection(a, b) / box_union(a, b);
}
void DetectorYOLOV3::ApplyNms(vector< YOLOV3PredictionResult >& boxes, vector<int>& idxes, float threshold) 
{
  map<int, int> idx_map;
  for (int i = 0; i < boxes.size() - 1; ++i) 
  {
    if (idx_map.find(i) != idx_map.end()) 
    {
      continue;
    }
    for (int j = i + 1; j < boxes.size(); ++j) 
    {
      if (idx_map.find(j) != idx_map.end()) 
      {
        continue;
      }
      vector<float> Bbox1, Bbox2;
      Bbox1.push_back(boxes[i].x);
      Bbox1.push_back(boxes[i].y);
      Bbox1.push_back(boxes[i].w);
      Bbox1.push_back(boxes[i].h);

      Bbox2.push_back(boxes[j].x);
      Bbox2.push_back(boxes[j].y);
      Bbox2.push_back(boxes[j].w);
      Bbox2.push_back(boxes[j].h);

      float iou = box_iou(Bbox1, Bbox2);
      if (iou >= threshold) 
      {
        idx_map[j] = 1;
      }
    }
  }
  for (int i = 0; i < boxes.size(); ++i) 
  {
    if (idx_map.find(i) == idx_map.end()) 
    {
      idxes.push_back(i);
    }
  }
}

void DetectorYOLOV3::correct_yolo_boxes(YOLOV3PredictionResult &det, int w, int h, int netw, int neth, int relative)
{
  int new_w=0;
  int new_h=0;
  if (((float)netw/w) < ((float)neth/h)) 
  {
      new_w = netw;
      new_h = (h * netw)/w;
  } 
  else 
  {
      new_h = neth;
      new_w = (w * neth)/h;
  }
  YOLOV3PredictionResult &b = det;
  b.x =  (b.x - (netw - new_w)/2./netw) / ((float)new_w/netw); 
  b.y =  (b.y - (neth - new_h)/2./neth) / ((float)new_h/neth); 
  b.w *= (float)netw/new_w;
  b.h *= (float)neth/new_h;
  if(!relative)
  {
      b.x *= w;
      b.w *= w;
      b.y *= h;
      b.h *= h;
  }

}

void DetectorYOLOV3::get_region_box(vector<float> &b, float* x, vector<float> biases, int n, int index, int i, int j, int lw, int lh, int w, int h, int stride)
{
  b.clear();
  b.push_back((i + (x[index + 0 * stride])) / lw);// 归一化
  b.push_back((j + (x[index + 1 * stride])) / lh);
  b.push_back(exp(x[index + 2 * stride]) * biases[2 * n] / (w));// 归一化
  b.push_back(exp(x[index + 3 * stride]) * biases[2 * n + 1] / (h));
}

bool BoxSortDecendScore(const YOLOV3PredictionResult& box1, const YOLOV3PredictionResult& box2) 
{
  return box1.confidence> box2.confidence;
}


DetectorYOLOV3::DetectorYOLOV3(){}


DetectorYOLOV3::~DetectorYOLOV3()
{
    
}


ErrorCode DetectorYOLOV3::Initialize(InitializationParameterOfDetector initializationParameterOfDetector)
{
    ErrorCode errorCodeOfCommonInitialization=DoCommonInitialization(initializationParameterOfDetector);
    if(errorCodeOfCommonInitialization!=OK)
    {
        LOG_ERROR(logFile,"fail to DoCommonInitialization\n");
        return errorCodeOfCommonInitialization;
    }
    LOG_INFO(logFile,"succeed to DoCommonInitialization\n");

    FileNode rootNode = configurationFile["DetectorYOLOV3"];

    string modelPath=initializationParameter.parentPath + (string)rootNode["ModelPath"];
    net = dnn::readNet(modelPath);// 加载onnx模型
    if (net.empty())
    {
        LOG_ERROR(logFile,"fail to load model: %s\n",GetFileName(modelPath).c_str());
        return FAIL_TO_LOAD_MODEL;
    }
    LOG_INFO(logFile,"succeed to load model: %s\n",GetFileName(modelPath).c_str());
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

    // 获取YOLOV3参数
    GetYOLOV3Parameter();

    return OK;
}


ErrorCode DetectorYOLOV3::Detect(const Mat &srcImage, std::vector<ResultOfDetection> &resultsOfDetection)
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

    // get result
    GetResult(outs,srcImage,resultsOfDetection);

    return OK;
}

void DetectorYOLOV3::GetYOLOV3Parameter()
{
    FileNode rootNode = configurationFile["DetectorYOLOV3"];
  
    yolov3Parameter.numberOfClasses=(int)rootNode["NumberOfClasses"];
    yolov3Parameter.numberOfAnchors=(int)rootNode["numberOfAnchors"];
    yolov3Parameter.confidenceThreshold=(float)rootNode["ConfidenceThreshold"];
    yolov3Parameter.nmsThreshold=(float)rootNode["NMSThreshold"];

    // biases
    int numberOfBiases=0;
    while(true)
    {
        char biasNodeName[256]={0};
        sprintf(biasNodeName,"Biases%d",++numberOfBiases);
        FileNode biasNode = rootNode[biasNodeName];
        if(biasNode.empty())
        {
            break;
        }
        else
        {
          yolov3Parameter.biases.push_back((float)rootNode[biasNodeName]);
        }
        
    }

}

void DetectorYOLOV3::GetResult(const std::vector<Mat> &bottom,const Mat &srcImage,std::vector<ResultOfDetection> &resultsOfDetection)
{
    int lengthOfPredictionResult = 4 + 1 + yolov3Parameter.numberOfClasses ;
    std::vector<YOLOV3PredictionResult> yolov3PredictionResults;

    yolov3PredictionResults.clear();
    float *classScore = new float[yolov3Parameter.numberOfClasses];
    Mat swap;
    for (int t = 0; t < bottom.size(); ++t) // 3个检测层的输出结果
    {
        int height = bottom[t].size[2];
        int width = bottom[t].size[3];
        int stride = width*height;
        int shape[]={bottom[t].size[0],bottom[t].size[1],bottom[t].size[2],bottom[t].size[3]};
        swap=Mat(4,shape,CV_32F);
        float* swapData = (float*)swap.data;
        float* inputData = (float *)bottom[t].data;
        for (int b = 0; b < bottom[t].size[0]; ++b)// bacthsize,这里为1
        {
            for (int s = 0; s < width*height; ++s)
            {
                // 处理特征图上每个位置上的多个anchor
                for (int n = 0; n < yolov3Parameter.numberOfAnchors; ++n)
                {
                    // 在特征图的第s位置处的每个anchor预测数据的起始位置
                    int index = n*lengthOfPredictionResult*stride + s + b*(bottom[t].size[1]*bottom[t].size[2]*bottom[t].size[3]);

                    // 处理每个anchor的(4+1+classNum)个预测值
                    for (int c = 0; c < lengthOfPredictionResult; ++c)
                    {
                        int index2 = c*stride + index;
                        if (c == 2 || c == 3)
                        {
                            // 位置预测值(w,h)直接使用原值
                            swapData[index2] = (inputData[index2 + 0]);
                        }
                        else
                        {
                            if (c > 4)
                            {
                                classScore[c - 5] = sigmoid(inputData[index2 + 0]);
                            }
                            else
                            {
                                // 位置预测值(x,y),以及目标置信度c需要计算sigmoid
                                swapData[index2] = sigmoid(inputData[index2 + 0]);
                            }
                        }
                    }
                    int y2 = s / width;
                    int x2 = s % width;
                    float objectScore=swapData[index + 4 * stride];

                    YOLOV3PredictionResult yolov3PredictionResult;
                    vector<float> predictionResult;
                    for (int c = 0; c < yolov3Parameter.numberOfClasses; ++c)
                    {
                        classScore[c] *= objectScore;
                        if (classScore[c] > yolov3Parameter.confidenceThreshold)
                        {
                            // 对预测值进行解码
                            get_region_box(predictionResult, swapData, yolov3Parameter.biases, t*yolov3Parameter.numberOfAnchors+n, index, x2, y2, width, height, inputWidth, inputHeight, stride);
                            yolov3PredictionResult.x = predictionResult[0];
                            yolov3PredictionResult.y = predictionResult[1];
                            yolov3PredictionResult.w = predictionResult[2];
                            yolov3PredictionResult.h = predictionResult[3];
                            yolov3PredictionResult.classType = c;
                            yolov3PredictionResult.confidence = classScore[c];

                            // 校正(防止发生扭曲变形)
                            correct_yolo_boxes(yolov3PredictionResult,width,height,inputWidth,inputHeight,1);
                            yolov3PredictionResults.push_back(yolov3PredictionResult);
                        }
                    }
                }
            }
        }

    }
    delete[] classScore;

    // NMS
    std::sort(yolov3PredictionResults.begin(), yolov3PredictionResults.end(), BoxSortDecendScore);
    vector<int> idxes;
    int numberOfNMSResult = 0;
    if(yolov3PredictionResults.size() > 0)
    {
        ApplyNms(yolov3PredictionResults, idxes, yolov3Parameter.nmsThreshold);
        numberOfNMSResult = idxes.size();
    }

    // 生成最后的检测结果
    for (int i = 0; i < numberOfNMSResult; ++i)
    {
        if(yolov3PredictionResults[idxes[i]].confidence>yolov3Parameter.confidenceThreshold)
        {
            float x1 = (yolov3PredictionResults[idxes[i]].x - yolov3PredictionResults[idxes[i]].w / 2.);
            float x2 = (yolov3PredictionResults[idxes[i]].x + yolov3PredictionResults[idxes[i]].w / 2.);
            float y1 = (yolov3PredictionResults[idxes[i]].y - yolov3PredictionResults[idxes[i]].h / 2.);
            float y2 = (yolov3PredictionResults[idxes[i]].y + yolov3PredictionResults[idxes[i]].h / 2.);
            int xmin=x1 * srcImage.cols;
            int ymin=y1 * srcImage.rows;
            int xmax=x2 * srcImage.cols;
            int ymax=y2 * srcImage.rows;
            ResultOfDetection result;
            Rect detectedRect(xmin,ymin,(xmax-xmin+1),(ymax-ymin+1));
            detectedRect&=Rect(0,0,srcImage.cols,srcImage.rows);
            result.boundingBox=detectedRect;
            result.confidence=yolov3PredictionResults[idxes[i]].confidence;//confidence
            result.classID=yolov3PredictionResults[idxes[i]].classType + 1; //label
            resultsOfDetection.push_back(result);
      }
    }
    std::sort(resultsOfDetection.begin(), resultsOfDetection.end(),CompareConfidence);
}

}

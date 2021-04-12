//////////////////////////////////////////////////////////////////////////
// 通用工具
//
// Please contact me if you find any bugs, or have any suggestions.
// Contact:
//		Email:654393155@qq.com
//		Blog:http://blog.csdn.net/qianqing13579
//////////////////////////////////////////////////////////////////////////

#ifndef __COMMON_UTILITY_H__
#define __COMMON_UTILITY_H__

#include<mutex>
#include<string>
#include<vector>
#include"CommonDefinition.h"

using namespace std;

namespace QQ
{

DLL_EXPORTS string GetCurrentTimeString();
DLL_EXPORTS std::vector<string> SplitString(string str,std::string separator);

// 排序规则: 按照置信度或者按照面积排序
DLL_EXPORTS bool CompareConfidence(const ResultOfDetection &L,const ResultOfDetection &R);
DLL_EXPORTS bool CompareConfidence2(const ResultOfPrediction &L, const ResultOfPrediction &R);
DLL_EXPORTS bool CompareArea(const ResultOfDetection &L,const ResultOfDetection &R);

DLL_EXPORTS void NMS(std::vector<ResultOfDetection> &detections, float IOUThreshold);

}

#endif //

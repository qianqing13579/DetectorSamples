#define DLLAPI_EXPORTS
#include "CommonUtility.h"
#include<assert.h>
#include<ctype.h>
#include<time.h>
#include<stdlib.h>
#include<algorithm>
#include<sstream>
#include<vector>

#ifdef _WIN32
#include <io.h>
#include <direct.h>
#include<Windows.h>
#else
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>
#endif

#include "SimpleLog.h"

namespace QQ
{


_Time GetCurrentTime3()
{
    _Time currentTime;

#if (defined WIN32 || defined _WIN32)
	SYSTEMTIME systemTime;
	GetLocalTime(&systemTime);

	char temp[8] = { 0 };
	sprintf(temp, "%04d", systemTime.wYear);
	currentTime.year=string(temp);
	sprintf(temp, "%02d", systemTime.wMonth);
	currentTime.month=string(temp);
	sprintf(temp, "%02d", systemTime.wDay);
	currentTime.day=string(temp);
	sprintf(temp, "%02d", systemTime.wHour);
	currentTime.hour=string(temp);
	sprintf(temp, "%02d", systemTime.wMinute);
	currentTime.minute=string(temp);
	sprintf(temp, "%02d", systemTime.wSecond);
	currentTime.second=string(temp);
	sprintf(temp, "%03d", systemTime.wMilliseconds);
	currentTime.millisecond=string(temp);
	sprintf(temp, "%d", systemTime.wDayOfWeek);
	currentTime.weekDay=string(temp);
#else
	struct timeval    tv;
	struct tm         *p;
	gettimeofday(&tv, NULL);
	p = localtime(&tv.tv_sec);

	char temp[8]={0};
    sprintf(temp,"%04d",1900+p->tm_year);
    currentTime.year=string(temp);
	sprintf(temp,"%02d",1+p->tm_mon);
	currentTime.month=string(temp);
	sprintf(temp,"%02d",p->tm_mday);
	currentTime.day=string(temp);
	sprintf(temp,"%02d",p->tm_hour);
	currentTime.hour=string(temp);
	sprintf(temp,"%02d",p->tm_min);
	currentTime.minute=string(temp);
	sprintf(temp,"%02d",p->tm_sec);
	currentTime.second=string(temp);
	sprintf(temp,"%03d",tv.tv_usec/1000);
	currentTime.millisecond = string(temp);
    sprintf(temp, "%03d", tv.tv_usec % 1000);
	currentTime.microsecond = string(temp);
    sprintf(temp, "%d", p->tm_wday);
    currentTime.weekDay = string(temp);
#endif
    return currentTime;
}

string GetCurrentTimeString()
{

    char timeString[256]={0};
    _Time currentTime=GetCurrentTime3();
    sprintf(timeString,"%s%s%s%s%s%s%s",currentTime.year.c_str(),currentTime.month.c_str(),
            currentTime.day.c_str(),currentTime.hour.c_str(),
            currentTime.minute.c_str(),currentTime.second.c_str(),currentTime.millisecond.c_str());

    return timeString;

}

vector<string> SplitString(string str, std::string separator)
{
    std::string::size_type pos;
    std::vector<std::string> result;
    str+=separator;//扩展字符串以方便操作
    int size=str.size();

    for(int i=0; i<size; i++)
    {
        pos=str.find(separator,i);
        if(pos<size)
        {
            std::string s=str.substr(i,pos-i);
            result.push_back(s);
            i=pos+separator.size()-1;
        }
    }
    return result;
}


bool CompareConfidence(const ResultOfDetection &L,const ResultOfDetection &R)
{
    return L.confidence > R.confidence;
}

bool CompareConfidence2(const ResultOfPrediction &L, const ResultOfPrediction &R)
{
    return L.confidence > R.confidence;
}

bool CompareArea(const ResultOfDetection &L,const ResultOfDetection &R)
{
    return L.boundingBox.area() > R.boundingBox.area();
}

void NMS(vector<ResultOfDetection> &detections, float IOUThreshold)
{
    // sort
    std::sort(detections.begin(), detections.end(), CompareConfidence);

    for (int i = 0; i<detections.size(); ++i)
    {
        if (detections[i].exist)
        {
            for (int j = i + 1; j<detections.size(); ++j)
            {
                if (detections[j].exist)
                {
                    // compute IOU
                    float intersectionArea = (detections[i].boundingBox & detections[j].boundingBox).area();
                    float intersectionRate = intersectionArea / (detections[i].boundingBox.area() + detections[j].boundingBox.area() - intersectionArea);

                    if (intersectionRate>IOUThreshold)
                    {
                        detections[j].exist = false;
                    }
                }
            }
        }
    }

}

}

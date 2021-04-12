#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include"Sample.h"

void MIGraphXSamplesUsage(char* programName)
{
    printf("Usage : %s <index> \n", programName);
    printf("index:\n");
    printf("\t 0) SSD sample.\n");
    printf("\t 1) YOLOV3 sample.\n");
    printf("\t 2) YOLOV5 sample.\n");
}

int main(int argc, char *argv[])
{
    if (argc < 2 || argc > 2)
    {
        MIGraphXSamplesUsage(argv[0]);
        return -1;
    }
    if (!strncmp(argv[1], "-h", 2))
    {
        MIGraphXSamplesUsage(argv[0]);
        return 0;
    }
    switch (*argv[1])
    {
        case '0':
            {
                Sample_DetectorSSD();
            }
            break;
        case '1':
            {
                Sample_DetectorYOLOV3();
            }
            break;
        case '2':
            {
                Sample_DetectorYOLOV5();
            }
            break;
        default :
            {
                MIGraphXSamplesUsage(argv[0]);
            }
            break;
    }
    return 0;
}


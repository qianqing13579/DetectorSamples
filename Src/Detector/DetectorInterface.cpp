#define DLLAPI_EXPORTS
#include"DetectorInterface.h"
#include"CommonUtility.h"
#include"Filesystem.h"
#include"SimpleLog.h"

namespace QQ
{

DetectorInterface::DetectorInterface():logFile(NULL)
{
}

DetectorInterface::~DetectorInterface()
{
    configurationFile.release();
}

ErrorCode DetectorInterface::DoCommonInitialization(InitializationParameterOfDetector initializationParameterOfDetector)
{
    initializationParameter=initializationParameterOfDetector;

    // log
    logFile=LogManager::GetInstance()->GetLogFile(initializationParameter.logName);

    // parentPath and configFilePath
    std::string &parentPath = initializationParameter.parentPath;
    std::string configFilePath=initializationParameter.configFilePath;
    if (!parentPath.empty())
    {
        if(!IsPathSeparator(parentPath[parentPath.size() - 1]))
        {
           parentPath+=PATH_SEPARATOR;
        }
    }

    // load configuration file
    if(!Exists(configFilePath))
    {
        LOG_ERROR(logFile, "no configuration file!\n");
        return CONFIG_FILE_NOT_EXIST;
    }
    if(!configurationFile.open(configFilePath, FileStorage::READ))
    {
       LOG_ERROR(logFile, "fail to open configuration file\n");
       return FAIL_TO_OPEN_CONFIG_FILE;
    }
    LOG_INFO(logFile, "succeed to open configuration file\n");

    return OK;


}

ErrorCode DetectorInterface::SetMiniSize(const int miniSize)
{

    (void)miniSize;// suppress unused variable warning

    return OK;

}

}



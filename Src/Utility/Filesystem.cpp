#define DLLAPI_EXPORTS
#include "Filesystem.h"
#include <algorithm>
#include <sys/stat.h>
#include <sys/types.h>
#include <fstream>
#ifdef _WIN32
#include <io.h>
#include <direct.h>
#include <Windows.h>
#else
#include <unistd.h>
#include <dirent.h>
#endif
#include "CommonUtility.h"
#include"opencv2/opencv.hpp"
#include"SimpleLog.h"

using namespace cv;
 
// 路径分隔符(Linux:‘/’,Windows:’\\’)
#ifdef _WIN32
#define  PATH_SEPARATOR '\\'
#else
#define  PATH_SEPARATOR '/'
#endif

namespace QQ
{

#if defined _WIN32 || defined WINCE
    const char dir_separators[] = "/\\";

	struct dirent
	{
		const char* d_name;
	};

	struct DIR
	{
#ifdef WINRT
		WIN32_FIND_DATAW data;
#else
        WIN32_FIND_DATAA data;
#endif
		HANDLE handle;
		dirent ent;
#ifdef WINRT
		DIR() { }
		~DIR()
		{
			if (ent.d_name)
				delete[] ent.d_name;
		}
#endif
	};

	DIR* opendir(const char* path)
	{
		DIR* dir = new DIR;
		dir->ent.d_name = 0;
#ifdef WINRT
		string full_path = string(path) + "\\*";
		wchar_t wfull_path[MAX_PATH];
		size_t copied = mbstowcs(wfull_path, full_path.c_str(), MAX_PATH);
		CV_Assert((copied != MAX_PATH) && (copied != (size_t)-1));
		dir->handle = ::FindFirstFileExW(wfull_path, FindExInfoStandard,
			&dir->data, FindExSearchNameMatch, NULL, 0);
#else
		dir->handle = ::FindFirstFileExA((string(path) + "\\*").c_str(),
			FindExInfoStandard, &dir->data, FindExSearchNameMatch, NULL, 0);
#endif
		if (dir->handle == INVALID_HANDLE_VALUE)
		{
			/*closedir will do all cleanup*/
			delete dir;
			return 0;
		}
		return dir;
	}

	dirent* readdir(DIR* dir)
	{
#ifdef WINRT
		if (dir->ent.d_name != 0)
		{
			if (::FindNextFileW(dir->handle, &dir->data) != TRUE)
				return 0;
		}
		size_t asize = wcstombs(NULL, dir->data.cFileName, 0);
		CV_Assert((asize != 0) && (asize != (size_t)-1));
		char* aname = new char[asize + 1];
		aname[asize] = 0;
		wcstombs(aname, dir->data.cFileName, asize);
		dir->ent.d_name = aname;
#else
		if (dir->ent.d_name != 0)
		{
			if (::FindNextFileA(dir->handle, &dir->data) != TRUE)
				return 0;
		}
		dir->ent.d_name = dir->data.cFileName;
#endif
		return &dir->ent;
	}

	void closedir(DIR* dir)
	{
		::FindClose(dir->handle);
		delete dir;
	}
#else
# include <dirent.h>
# include <sys/stat.h>
	const char dir_separators[] = "/";
#endif

	static bool isDir(const string &path, DIR* dir)
	{
#if defined _WIN32 || defined WINCE
		DWORD attributes;
		BOOL status = TRUE;
		if (dir)
			attributes = dir->data.dwFileAttributes;
		else
		{
			WIN32_FILE_ATTRIBUTE_DATA all_attrs;
#ifdef WINRT
			wchar_t wpath[MAX_PATH];
			size_t copied = mbstowcs(wpath, path.c_str(), MAX_PATH);
			CV_Assert((copied != MAX_PATH) && (copied != (size_t)-1));
			status = ::GetFileAttributesExW(wpath, GetFileExInfoStandard, &all_attrs);
#else
			status = ::GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &all_attrs);
#endif
			attributes = all_attrs.dwFileAttributes;
		}

		return status && ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
#else
		(void)dir;
		struct stat stat_buf;
		if (0 != stat(path.c_str(), &stat_buf))
			return false;
		int is_dir = S_ISDIR(stat_buf.st_mode);
		return is_dir != 0;
#endif
	}

	bool IsDirectory(const string &path)
	{
		return isDir(path, NULL);
	}

	bool Exists(const string& path)
	{

#if defined _WIN32 || defined WINCE
			BOOL status = TRUE;
		{
			WIN32_FILE_ATTRIBUTE_DATA all_attrs;
#ifdef WINRT
			wchar_t wpath[MAX_PATH];
			size_t copied = mbstowcs(wpath, path.c_str(), MAX_PATH);
			CV_Assert((copied != MAX_PATH) && (copied != (size_t)-1));
			status = ::GetFileAttributesExW(wpath, GetFileExInfoStandard, &all_attrs);
#else
			status = ::GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &all_attrs);
#endif
		}

		return !!status;
#else
		struct stat stat_buf;
		return (0 == stat(path.c_str(), &stat_buf));
#endif
	}

    bool IsPathSeparator(char c)
    {
        return c == '/' || c == '\\';
	}

	string JoinPath(const string& base, const string& path)
	{
		if (base.empty())
			return path;
		if (path.empty())
			return base;

		bool baseSep = IsPathSeparator(base[base.size() - 1]);
		bool pathSep = IsPathSeparator(path[0]);
		string result;
		if (baseSep && pathSep)
		{
			result = base + path.substr(1);
		}
		else if (!baseSep && !pathSep)
		{
            result = base + PATH_SEPARATOR + path;
		}
		else
		{
			result = base + path;
		}
		return result;
	}

	static bool wildcmp(const char *string, const char *wild)
	{
		const char *cp = 0, *mp = 0;

		while ((*string) && (*wild != '*'))
		{
			if ((*wild != *string) && (*wild != '?'))
			{
				return false;
			}

			wild++;
			string++;
		}

		while (*string)
		{
			if (*wild == '*')
			{
				if (!*++wild)
				{
					return true;
				}

				mp = wild;
				cp = string + 1;
			}
			else if ((*wild == *string) || (*wild == '?'))
			{
				wild++;
				string++;
			}
			else
			{
				wild = mp;
				string = cp++;
			}
		}

		while (*wild == '*')
		{
			wild++;
		}

		return *wild == 0;
	}

	static void glob_rec(const string &directory, const string& wildchart, std::vector<string>& result,
		bool recursive, bool includeDirectories, const string& pathPrefix)
	{
		DIR *dir;

		if ((dir = opendir(directory.c_str())) != 0)
		{
			/* find all the files and directories within directory */
			try
			{
				struct dirent *ent;
				while ((ent = readdir(dir)) != 0)
				{
					const char* name = ent->d_name;
					if ((name[0] == 0) || (name[0] == '.' && name[1] == 0) || (name[0] == '.' && name[1] == '.' && name[2] == 0))
						continue;

					string path = JoinPath(directory, name);
					string entry = JoinPath(pathPrefix, name);

					if (isDir(path, dir))
					{
						if (recursive)
							glob_rec(path, wildchart, result, recursive, includeDirectories, entry);
						if (!includeDirectories)
							continue;
					}

					if (wildchart.empty() || wildcmp(name, wildchart.c_str()))
						result.push_back(entry);
				}
			}
			catch (...)
			{
				closedir(dir);
				throw;
			}
			closedir(dir);
		}
		else
		{
			LOG_INFO(stdout, "could not open directory: %s", directory.c_str());
		}
	}

	void GetFileNameList(const string &directory, const string &pattern, std::vector<string>& result, bool recursive, bool addPath)
	{
        // split pattern
        vector<string> patterns=SplitString(pattern,",");

        result.clear();

        for(int i=0;i<patterns.size();++i)
        {
            string eachPattern=patterns[i];
            std::vector<string> eachResult;
            glob_rec(directory, eachPattern, eachResult, recursive, true, directory);
            for(int j=0;j<eachResult.size();++j)
            {
				if (IsDirectory(eachResult[j]))
					continue;
                if(addPath)
                {
                    result.push_back(eachResult[j]);
                }
                else
                {
                    result.push_back(GetFileName(eachResult[j]));
                }
            }
        }
		std::sort(result.begin(), result.end());
	}

	void GetFileNameList2(const string &directory, const string &pattern, std::vector<string>& result, bool recursive, bool addPath)
	{
		// split pattern
		vector<string> patterns = SplitString(pattern, ",");

        result.clear();

		for (int i = 0; i<patterns.size(); ++i)
		{
			string eachPattern = patterns[i];
			std::vector<string> eachResult;
			glob_rec(directory, eachPattern, eachResult, recursive, true, directory);
			for (int j = 0; j<eachResult.size(); ++j)
			{
				string filePath = eachResult[j];
				if (IsDirectory(filePath))
				{
					filePath = filePath + "/";
					for (int k = 0; k < filePath.size(); ++k)
					{
						if (IsPathSeparator(filePath[k]))
						{
							filePath[k] = '/';
						}
					}
				}
				if (addPath)
				{
					result.push_back(filePath);
				}
				else
				{
					if (!IsDirectory(filePath))
					{
						result.push_back(GetFileName(filePath));
					}
				}
			}
		}
		std::sort(result.begin(), result.end());
	}

	void WriteFileNameListToFile(const string &directory, const string &pattern, const string &pathOfFileNameList, bool recursive, bool addPath)
	{
		std::ofstream fileNameListFile(pathOfFileNameList, ios::out);
		if (!fileNameListFile.is_open())
		{
			LOG_ERROR(stdout, "can not open file:%s\n", pathOfFileNameList.c_str());

			return;
		}

		// read fileNames
		std::vector<string> fileNameList;
        QQ::GetFileNameList(directory, pattern, fileNameList, recursive, addPath);
		LOG_INFO(stdout, "the number of files:%d\n", fileNameList.size());

		for (int i = 0; i < fileNameList.size(); ++i)
		{
			fileNameListFile << fileNameList[i] << endl;
		}
		fileNameListFile.close();

		LOG_INFO(stdout, "succeed to write:%s\n", GetFileName(pathOfFileNameList).c_str());


	}
	
	void RemoveAll(const string& path)
	{

		if (!Exists(path))
			return;

		if (IsDirectory(path))
		{
			std::vector<string> entries;
			GetFileNameList2(path, string(), entries, false, true);
			for (size_t i = 0; i < entries.size(); i++)
			{
				const string& e = entries[i];
				RemoveAll(e);
			}
#ifdef _MSC_VER
			bool result = _rmdir(path.c_str()) == 0;
#else
			bool result = rmdir(path.c_str()) == 0;
#endif
			if (!result)
			{
				LOG_INFO(stdout, "can't remove directory: %s\n", path.c_str());
			}
		}
		else
		{
#ifdef _MSC_VER
			bool result = _unlink(path.c_str()) == 0;
#else
			bool result = unlink(path.c_str()) == 0;
#endif
			if (!result)
			{
				LOG_INFO(stdout, "can't remove file: %s\n", path.c_str());
			}
		}
	}


        void RemoveAllSdk(const string& path)
        {
#ifdef MINIVISIONSDK
            if (!Exists(path))
                return;

            if (IsDirectory(path))
            {
                std::vector<string> entries;
                GetFileNameList2(path, string(), entries, false, true);
                for (size_t i = 0; i < entries.size(); i++)
                {
                    const string& e = entries[i];
                    RemoveAll(e);
                }
#ifdef _MSC_VER
                bool result = _rmdir(path.c_str()) == 0;
#else
                bool result = rmdir(path.c_str()) == 0;
#endif
                if (!result)
                {
                    LOG_INFO(stdout, "can't remove directory: %s\n", path.c_str());
                }
            }
            else
            {
#ifdef _MSC_VER
                bool result = _unlink(path.c_str()) == 0;
#else
                bool result = unlink(path.c_str()) == 0;
#endif
                if (!result)
                {
                    LOG_INFO(stdout, "can't remove file: %s\n", path.c_str());
                }
            }
#endif
        }

	void Remove(const string &directory, const string &extension)
	{

		DIR *dir;

		static int numberOfFiles = 0;

		if ((dir = opendir(directory.c_str())) != 0)
		{
			/* find all the files and directories within directory */
			try
			{
				struct dirent *ent;
				while ((ent = readdir(dir)) != 0)
				{
					const char* name = ent->d_name;
					if ((name[0] == 0) || (name[0] == '.' && name[1] == 0) || (name[0] == '.' && name[1] == '.' && name[2] == 0))
						continue;

					string path = JoinPath(directory, name);

					if (isDir(path, dir))
					{
						Remove(path, extension);
					}

					// �ж���չ��
					if (extension.empty() || wildcmp(name, extension.c_str()))
					{
						RemoveAll(path);
						++numberOfFiles;
						LOG_INFO(stdout, "%s deleted! number of deleted files:%d\n", path.c_str(), numberOfFiles);
					}

				}
			}
			catch (...)
			{
				closedir(dir);
				throw;
			}
			closedir(dir);
		}
		else
		{
			LOG_INFO(stdout, "could not open directory: %s", directory.c_str());
		}

		// ����RemoveAllɾ��Ŀ¼
		RemoveAll(directory);
	}
	string GetFileName(const string &path)
	{
        string fileName;
        int indexOfPathSeparator = -1;
        for (int i = path.size() - 1; i >= 0; --i)
        {
            if (IsPathSeparator(path[i]))
            {
                fileName = path.substr(i + 1, path.size() - i - 1);
                indexOfPathSeparator = i;
                break;
            }
        }
        if (indexOfPathSeparator == -1)
        {
            fileName = path;
        }

        return fileName;
	}
    string GetFileName_NoExtension(const string &path)
    {
        string fileName=GetFileName(path);
        string fileName_NoExtension;
        for(int i=fileName.size()-1;i>0;--i)
        {
            if(fileName[i]=='.')
            {
                fileName_NoExtension=fileName.substr(0,i);
                break;
            }
        }

        return fileName_NoExtension;
    }

	string GetExtension(const string &path)
	{
		string fileName;
		for (int i = path.size() - 1; i >= 0; --i)
		{
			if (path[i]=='.')
			{
				fileName = path.substr(i, path.size() - i);
				break;
			}
		}

		return fileName;

	}

	string GetParentPath(const string &path)
	{
		string fileName;
		for (int i = path.size() - 1; i >= 0; --i)
		{
			if (IsPathSeparator(path[i]))
			{
				fileName = path.substr(0, i+1);
				break;
			}
		}

		return fileName;
	}

    static bool CreateDirectory(const string &path)
    {
    #if defined WIN32 || defined _WIN32 || defined WINCE
    #ifdef WINRT
                wchar_t wpath[MAX_PATH];
            size_t copied = mbstowcs(wpath, path.c_str(), MAX_PATH);
            CV_Assert((copied != MAX_PATH) && (copied != (size_t)-1));
            int result = CreateDirectoryA(wpath, NULL) ? 0 : -1;
    #else
                int result = _mkdir(path.c_str());
    #endif
    #elif defined __linux__ || defined __APPLE__
                int result = mkdir(path.c_str(), 0777);
    #else
                int result = -1;
    #endif

            if (result == -1)
            {
                return IsDirectory(path);
            }
            return true;
    }

	bool CreateDirectories(const string &directoryPath)
	{
		string path = directoryPath;

		for (;;)
		{
			char last_char = path.empty() ? 0 : path[path.length() - 1];
			if (IsPathSeparator(last_char))
			{
				path = path.substr(0, path.length() - 1);
				continue;
			}
			break;
		}

		if (path.empty() || path == "./" || path == ".\\" || path == ".")
			return true;
		if (IsDirectory(path))
			return true;

		size_t pos = path.rfind('/');
		if (pos == string::npos)
			pos = path.rfind('\\');
		if (pos != string::npos)
		{
			string parent_directory = path.substr(0, pos);
			if (!parent_directory.empty())
			{
				if (!CreateDirectories(parent_directory))
					return false;
			}
		}

		return CreateDirectory(path);
	}

    bool CopyFile(const string srcPath, const string dstPath)
    {
        std::ifstream srcFile(srcPath,ios::binary);
        std::ofstream dstFile(dstPath,ios::binary);

        if(!srcFile.is_open())
        {
            LOG_INFO(stdout,"can not open %s\n",srcPath.c_str());
            return false;
        }
        if(!dstFile.is_open())
        {
			LOG_INFO(stdout, "can not open %s\n", dstPath.c_str());
            return false;
        }
        if(srcPath==dstPath)
        {
			LOG_INFO(stdout, "src can not be same with dst\n");
            return false;
        }
        char buffer[2048];
        unsigned int numberOfBytes=0;
        while(srcFile)
        {
            srcFile.read(buffer,2048);
            dstFile.write(buffer,srcFile.gcount());
            numberOfBytes+=srcFile.gcount();
        }
        srcFile.close();
        dstFile.close();
        return true;
    }

    bool CopyDirectories(string srcPath, const string dstPath)
    {
        if(srcPath==dstPath)
        {
			LOG_INFO(stdout, "src can not be same with dst\n");
            return false;
        }

		// ȥ������·���ָ���
		srcPath = srcPath.substr(0, srcPath.size() - 1);

        vector<string> fileNameList;
        GetFileNameList2(srcPath, "", fileNameList, true, true);

        string parentPathOfSrc=GetParentPath(srcPath);
        int length=parentPathOfSrc.size();

        // create all directories
        for(int i=0;i<fileNameList.size();++i)
        {
            // create directory
            string srcFilePath=fileNameList[i];
            string subStr=srcFilePath.substr(length,srcFilePath.size()-length);
            string dstFilePath=dstPath+subStr;
            string parentPathOfDst=GetParentPath(dstFilePath);
            CreateDirectories(parentPathOfDst);
        }

        // copy file
        for(int i=0;i<fileNameList.size();++i)
        {
            string srcFilePath=fileNameList[i];
			if (IsDirectory(srcFilePath))
			{
				continue;
			}
            string subStr=srcFilePath.substr(length,srcFilePath.size()-length);
            string dstFilePath=dstPath+subStr;

            // copy file
            CopyFile(srcFilePath,dstFilePath);

			// process
			double process = (1.0*(i + 1) / fileNameList.size()) * 100;
			LOG_INFO(stdout, "%s done! %f% \n", GetFileName(fileNameList[i]).c_str(), process);
        }
		LOG_INFO(stdout, "all done!(the number of files:%d)\n", fileNameList.size());

        return true;


    }

	class CopyDirectories_LoopBodyObject :public ParallelLoopBody
	{
	public:
		CopyDirectories_LoopBodyObject(const vector<string> _fileNameList, const string _dstPath, const int _length):
			fileNameList(_fileNameList),
			dstPath(_dstPath),
			length(_length){}

		void operator()(const Range  &r) const
		{

			for (int i = r.start; i<r.end; ++i)
			{
				string srcFilePath = fileNameList[i];
				if (IsDirectory(srcFilePath))
				{
					continue;
				}
				string subStr = srcFilePath.substr(length, srcFilePath.size() - length);
				string dstFilePath = dstPath + subStr;

				// copy file
				CopyFile(srcFilePath, dstFilePath);

				// process
				double process = (1.0*(i + 1) / fileNameList.size()) * 100;
				LOG_INFO(stdout, "%s done! %f% \n", GetFileName(fileNameList[i]).c_str(), process);
				
			}
		}

	private:
		vector<string> fileNameList;
		string dstPath;
		int length;

	};

	bool CopyDirectories_Parallel(string srcPath, const string dstPath)
	{
		if (srcPath == dstPath)
		{
			LOG_ERROR(stdout,"src can not be same with dst\n");
			return false;
		}

		// ȥ������·���ָ���
		srcPath = srcPath.substr(0, srcPath.size() - 1);

		vector<string> fileNameList;
		GetFileNameList2(srcPath, "", fileNameList, true, true);

		string parentPathOfSrc = GetParentPath(srcPath);
		int length = parentPathOfSrc.size();

		// create all directories
		for (int i = 0; i<fileNameList.size(); ++i)
		{
			// create directory
			string srcFilePath = fileNameList[i];
			string subStr = srcFilePath.substr(length, srcFilePath.size() - length);
			string dstFilePath = dstPath + subStr;
			string parentPathOfDst = GetParentPath(dstFilePath);
			CreateDirectories(parentPathOfDst);
		}

		// copy file
		CopyDirectories_LoopBodyObject block(fileNameList,dstPath,length);
		parallel_for_(Range(0, fileNameList.size()), block);
		LOG_INFO(stdout, "all done!(the number of files:%d)\n", fileNameList.size());

		return true;

	}

	void CopyFileInDirectory(const string &srcPath, const string &extension, int number, const string &dstPath, bool useOriginalName, int startIndex)
	{
		if (!Exists(srcPath))
		{
			LOG_ERROR(stdout, "could not find directory:%s\n", srcPath.c_str());
			return;
		}
		if (!Exists(dstPath))
		{
			CreateDirectories(dstPath);
		}

		// read all file names
		std::vector<string> imageNames;
        QQ::GetFileNameList(srcPath, extension, imageNames, true, true);
		std::vector<string> imageNames_NoPath;
		for (int i = 0; i < imageNames.size(); ++i)
		{
			imageNames_NoPath.push_back(GetFileName(imageNames[i]));
		}


		// get part of it
		std::vector<string> imageNames2;
		std::vector<string> imageNames_NoPath2;

		number = MIN(number, imageNames.size());
		if (number == 0)
			return;
		imageNames2.reserve(number);
		imageNames_NoPath2.reserve(number);
		for (int i = 0; i<number; ++i)
		{
			imageNames2.push_back(imageNames[i]);
			imageNames_NoPath2.push_back(imageNames_NoPath[i]);
		}

		// copy image
		for (int i = 0; i<imageNames2.size(); ++i)
		{
			string srcPath1(imageNames2[i]);
			string dstPath1;

			if (useOriginalName)
			{
				dstPath1 = string(dstPath + imageNames_NoPath2[i]);
			}
			else
			{
				char fileName[256] = { 0 };
				string currentTimeString = GetCurrentTimeString();
				sprintf(fileName, "%s%s_%08d%s", dstPath.c_str(), currentTimeString.c_str(), startIndex++, GetExtension(srcPath1).c_str());
				dstPath1 = string(fileName);
			}

			// move
			if (!Exists(dstPath1))
			{
                QQ::CopyFile(srcPath1, dstPath1);
			}

			// log
			double process = (1.0*(i + 1) / imageNames2.size()) * 100;
			LOG_INFO(stdout, "%s  done! %f \n", imageNames2[i].c_str(), process);

		}

	}

	class CopyFileInDirectory_LoopBodyObject :public ParallelLoopBody
	{
	public:
		CopyFileInDirectory_LoopBodyObject(std::vector<string> _imageNames,
			std::vector<string> _imageNames_NoPath,
			string _dstPath,
			bool _useOriginalName,
			int *_startIndex,
			Mutex *_mutex) :
			imageNames(_imageNames),
			imageNames_NoPath(_imageNames_NoPath),
			dstPath(_dstPath),
			useOriginalName(_useOriginalName),
			startIndex(_startIndex),
			mutex(_mutex){}

		void operator()(const Range  &r) const
		{

			for (int i = r.start; i<r.end; ++i)
			{
				string srcPath1(imageNames[i]);
				string dstPath1;

				if (useOriginalName)
					dstPath1 = string(dstPath + imageNames_NoPath[i]);
				else
				{
					mutex->lock();
					string index = to_string((*startIndex)++);
					mutex->unlock();

					// �ڲ���ѭ�����У�ʹ��sprintf()ʵ�������ķ�ʽ�ᱨ��
					string currentTimeString = GetCurrentTimeString();
					dstPath1 = string(dstPath + currentTimeString + index + GetExtension(srcPath1));

				}

				// copy
				if (!Exists(dstPath1))
				{
                    QQ::CopyFile(srcPath1, dstPath1);
				}

				// log
				LOG_INFO(stdout, "%s done!\n", srcPath1.c_str());
			}
		}

	private:
		std::vector<string> imageNames;
		std::vector<string> imageNames_NoPath;
		string dstPath;
		bool useOriginalName;
		int *startIndex;
		Mutex *mutex;

	};

	void CopyFileInDirectory_Parallel(const string &srcPath, const string &extension, int number, const string &dstPath, bool useOriginalName, int startIndex)
	{
		if (!Exists(srcPath))
		{
			LOG_ERROR(stdout, "could not find directory:%s\n", srcPath.c_str());
			return;
		}
		if (!Exists(dstPath))
		{
			CreateDirectories(dstPath);
		}

		// read all file names
		std::vector<string> imageNames;
        QQ::GetFileNameList(srcPath, extension, imageNames, true, true);
		std::vector<string> imageNames_NoPath;
		for (int i = 0; i < imageNames.size(); ++i)
		{
			imageNames_NoPath.push_back(GetFileName(imageNames[i]));
		}

		// get part of it
		std::vector<string> imageNames2;
		std::vector<string> imageNames_NoPath2;

		number = MIN(number, imageNames.size());
		if (number == 0)
			return;
		imageNames2.reserve(number);
		imageNames_NoPath2.reserve(number);
		for (int i = 0; i<number; ++i)
		{
			imageNames2.push_back(imageNames[i]);
			imageNames_NoPath2.push_back(imageNames_NoPath[i]);
		}

		// move image
		Mutex mutex;
		CopyFileInDirectory_LoopBodyObject block(imageNames2, imageNames_NoPath2, dstPath, useOriginalName, &startIndex, &mutex);
		parallel_for_(Range(0, imageNames2.size()), block);

		LOG_INFO(stdout, "all done!\n");

	}

	void MoveFileInDirectory(const string &srcPath, const string &extension, int number, const string &dstPath, bool useOriginalName, int startIndex)
	{

		if (!Exists(srcPath))
		{
			LOG_ERROR(stdout, "could not find directory:%s\n", srcPath.c_str());
			return;
		}
		if (!Exists(dstPath))
		{
			CreateDirectories(dstPath);
		}

		// read all file names
		std::vector<string> imageNames;
        QQ::GetFileNameList(srcPath, extension, imageNames, true, true);
		std::vector<string> imageNames_NoPath;
		for (int i = 0; i < imageNames.size(); ++i)
		{
			imageNames_NoPath.push_back(GetFileName(imageNames[i]));
		}

		// get part of it
		std::vector<string> imageNames2;
		std::vector<string> imageNames_NoPath2;

		number = MIN(number, imageNames.size());
		if (number == 0)
			return;
		imageNames2.reserve(number);
		imageNames_NoPath2.reserve(number);
		for (int i = 0; i<number; ++i)
		{
			imageNames2.push_back(imageNames[i]);
			imageNames_NoPath2.push_back(imageNames_NoPath[i]);
		}

		// move image
		for (int i = 0; i<imageNames2.size(); ++i)
		{
			string srcPath1(imageNames2[i]);
			string dstPath1;

			if (useOriginalName)
			{
				dstPath1 = string(dstPath + imageNames_NoPath2[i]);
			}
			else
			{
				char fileName[256] = { 0 };
				string currentTimeString = GetCurrentTimeString();
				sprintf(fileName, "%s%s_%08d%s", dstPath.c_str(), currentTimeString.c_str(), startIndex++, GetExtension(srcPath1).c_str());
				dstPath1 = string(fileName);
			}

			// move
			if (!Exists(dstPath1))
			{
                QQ::CopyFile(srcPath1, dstPath1);
				RemoveAll(srcPath1);
			}

			// log
			double process = (1.0*(i + 1) / imageNames2.size()) * 100;
			LOG_INFO(stdout, "%s  done! %f \n", imageNames2[i].c_str(), process);

		}

	}

	class MoveFileInDirectory_LoopBodyObject :public ParallelLoopBody
	{
	public:
		MoveFileInDirectory_LoopBodyObject(std::vector<string> _imageNames,
			std::vector<string> _imageNames_NoPath,
			string _dstPath,
			bool _useOriginalName,
			int *_startIndex,
			Mutex *_mutex) :
			imageNames(_imageNames),
			imageNames_NoPath(_imageNames_NoPath),
			dstPath(_dstPath),
			useOriginalName(_useOriginalName),
			startIndex(_startIndex),
			mutex(_mutex){}

		void operator()(const Range  &r) const
		{

			for (int i = r.start; i<r.end; ++i)
			{
				string srcPath1(imageNames[i]);
				string dstPath1;

				if (useOriginalName)
					dstPath1 = string(dstPath + imageNames_NoPath[i]);
				else
				{
					mutex->lock();
					string index = to_string((*startIndex)++);
					mutex->unlock();

					// �ڲ���ѭ�����У�ʹ��sprintf()ʵ�������ķ�ʽ�ᱨ��
					string currentTimeString = GetCurrentTimeString();
					dstPath1 = string(dstPath + currentTimeString + index + GetExtension(srcPath1));

				}

				// move
				if (!Exists(dstPath1))
				{
                    QQ::CopyFile(srcPath1, dstPath1);
                    QQ::RemoveAll(srcPath1);
				}

				// log
				LOG_INFO(stdout, "%s done!\n", srcPath1.c_str());
			}

		}

	private:
		std::vector<string> imageNames;
		std::vector<string> imageNames_NoPath;
		string dstPath;
		bool useOriginalName;
		int *startIndex;
		Mutex *mutex;

	};

	void MoveFileInDirectory_Parallel(const string &srcPath, const string &extension, int number, const string &dstPath, bool useOriginalName, int startIndex)
	{

		if (!Exists(srcPath))
		{
			LOG_ERROR(stdout, "could not find directory:%s\n", srcPath.c_str());
			return;
		}
		if (!Exists(dstPath))
		{
			CreateDirectories(dstPath);
		}

		// read all file names
		std::vector<string> imageNames;
        QQ::GetFileNameList(srcPath, extension, imageNames, true, true);
		std::vector<string> imageNames_NoPath;
		for (int i = 0; i < imageNames.size(); ++i)
		{
			imageNames_NoPath.push_back(GetFileName(imageNames[i]));
		}

		// get part of it
		std::vector<string> imageNames2;
		std::vector<string> imageNames_NoPath2;

		// compute mini
		number = MIN(number, imageNames.size());
		if (number == 0)
			return;
		imageNames2.reserve(number);
		imageNames_NoPath2.reserve(number);
		for (int i = 0; i<number; ++i)
		{
			imageNames2.push_back(imageNames[i]);
			imageNames_NoPath2.push_back(imageNames_NoPath[i]);
		}

		// move image
		Mutex mutex;
		MoveFileInDirectory_LoopBodyObject block(imageNames2, imageNames_NoPath2, dstPath, useOriginalName, &startIndex, &mutex);
		parallel_for_(Range(0, imageNames2.size()), block);

		LOG_INFO(stdout, "all done!\n");

	}

	void RenameFilesInDirectory(const string &srcPath, const string &dstPath)
	{
		if (!Exists(srcPath))
		{
			LOG_ERROR(stdout, "could not find directory:%s\n",srcPath.c_str());
			return;
		}
		if (!Exists(dstPath))
		{
			CreateDirectories(dstPath);
		}

		vector<string> fileNames;
        QQ::GetFileNameList(srcPath, "", fileNames, true, true);

		int numberOfAvailable = 0;
		char newFileName[256] = { 0 };
		int startIndex = 0;
		for (size_t i = 0; i<fileNames.size(); ++i)
		{
			string srcFileName(fileNames[i]);

			// create new file name
			string timeString = GetCurrentTimeString();
			sprintf(newFileName, "%s%s_%04d.png", dstPath.c_str(), timeString.c_str(), startIndex++);

			if (srcPath == dstPath)
			{
				//
                QQ::CopyFile(srcFileName, newFileName);
				RemoveAll(srcFileName);
			}
			else
			{
                QQ::CopyFile(srcFileName, newFileName);
			}

			++numberOfAvailable;

			// log
			double process = (1.0*(i + 1) / fileNames.size()) * 100;
			LOG_INFO(stdout, "%s  done! %f%\n", srcFileName.c_str(), process);
		}
		LOG_INFO(stdout, "the number of files : %d\n", numberOfAvailable);
	}

bool CreateSingleDirectory(const string &path)
{
#if defined WIN32 || defined _WIN32 || defined WINCE
#ifdef WINRT
    wchar_t wpath[MAX_PATH];
    size_t copied = mbstowcs(wpath, path.c_str(), MAX_PATH);
    CV_Assert((copied != MAX_PATH) && (copied != (size_t)-1));
    int result = CreateDirectoryA(wpath, NULL) ? 0 : -1;
#else
    int result = _mkdir(path.c_str());
#endif
#elif defined __linux__ || defined __APPLE__
    int result = mkdir(path.c_str(), 0777);
#else
    int result = -1;
#endif

    if (result == -1)
    {
        return IsDirectory(path);
    }
    return true;
}

}



//////////////////////////////////////////////////////////////////////////
// 文件以及目录处理
//
// Please contact me if you find any bugs, or have any suggestions.
// Contact:
//		Email:654393155@qq.com
//		Blog:http://blog.csdn.net/qianqing13579
//////////////////////////////////////////////////////////////////////////

#ifndef __FILE_SYSTEM_H__
#define __FILE_SYSTEM_H__

#include<vector>
#include<string>
#include"CommonDefinition.h"

using namespace std;
 
namespace QQ
{

    // 路径是否存在
	bool Exists(const string &path);

    // 路径是否为目录
	bool IsDirectory(const string &path);

    // 是否是路径分隔符(Linux:‘/’,Windows:’\\’)
    bool IsPathSeparator(char c);

	string JoinPath(const string &base, const string &path);

    // 创建多级目录,注意：创建多级目录的时候，目标目录是不能有文件存在的
	bool CreateDirectories(const string &directoryPath);

    bool CreateSingleDirectory(const string &path);

	/**
    * 生成符合指定模式的文件名列表(支持递归遍历)
    * pattern:模式,比如"*.jpg","*.png","*.jpg,*.png"
    * addPath：是否包含父路径
    * 注意：
        1. 多个模式使用","分割,比如"*.jpg,*.png"
        2. 支持通配符'*','?' ,比如第一个字符是7的所有文件名:"7*.*", 以512结尾的所有jpg文件名："*512.jpg"
        3. 使用"*.jpg"，而不是".jpg"
        4. 空string表示返回所有结果
		5. 不能返回子目录名
    *
	*/
	void GetFileNameList(const string &directory, const string &pattern, std::vector<string> &result, bool recursive, bool addPath);
	
	// 与GetFileNameList的区别在于如果有子目录，在addPath为true的时候会返回子目录路径(目录名最后有"/")
	void GetFileNameList2(const string &directory, const string &pattern, std::vector<string> &result, bool recursive, bool addPath);

	// 文件列表写入txt文件中,参数含义参考 GetFileNameList
	void WriteFileNameListToFile(const string &directory, const string &pattern, const string &pathOfFileNameList, bool recursive, bool addPath);

	/* 已弃用，使用 Remove
	删除文件或目录
	*/
	void RemoveAll(const string& path);

	/* 删除文件或者目录,支持递归删除
	支持删除大量文件(>1000w)，RemoveAll不能支持大量文件的删除
	只支持单个扩展名(使用空字符串表示所有类型的文件)
	Remove中调用RemoveAll删除文件或者目录
	*/
	void Remove(const string &directory, const string &extension="");

	// 获取路径的文件名和扩展名
	// D:/1/1.txt,文件名为1.txt,扩展名为.txt,父路径为D:/1/
    string GetFileName(const string &path); // 1.txt
    string GetFileName_NoExtension(const string &path); // 1
    string GetExtension(const string &path);// .txt
    string GetParentPath(const string &path);// D:/1/

    // 拷贝文件:CopyFile("D:/1.txt","D:/2.txt");将1.txt拷贝为2.txt
    bool CopyFile(const string srcPath,const string dstPath);

    /*拷贝目录
    示例：CopyDirectories(“D:/0/1/2/”,”E:/3/”);实现把D:/0/1/2/目录拷贝到E:/3/目录中(即拷贝完成后的目录结构为E:/3/2/)
    注意：
        1.第一个参数的最后不能加”/”
        2.不能拷贝隐藏文件
    */
    bool CopyDirectories(string srcPath,const string dstPath);
	bool CopyDirectories_Parallel(string srcPath, const string dstPath);// 并行版本(多线程)

	// 拷贝目录中的文件
	// 将srcPath中后缀为extension的number个文件拷贝到dstPath,useOriginalName表示是否使用原文件名，如果不使用则以时间戳命名，并加入startIndex后缀，支持递归遍历
	void CopyFileInDirectory(const string &srcPath, const string &extension, int number, const string &dstPath, bool useOriginalName = true, int startIndex = 0);
	void CopyFileInDirectory_Parallel(const string &srcPath, const string &extension, int number, const string &dstPath, bool useOriginalName = true, int startIndex = 0);

	// 移动目录中的文件
	// 将srcPath中后缀为extension的number个文件移动到dstPath,useOriginalName表示是否使用原文件名，如果不使用则以时间戳命名，并加入startIndex后缀，支持递归遍历
	void MoveFileInDirectory(const string &srcPath, const string &extension, int number, const string &dstPath, bool useOriginalName = true, int startIndex = 0);
	void MoveFileInDirectory_Parallel(const string &srcPath, const string &extension, int number, const string &dstPath, bool useOriginalName = true, int startIndex = 0);

	// 重命名目录中的文件
	// 将srcPath中的文件重名到dstPath,重名采用时间戳的方式,如果 srcPath==dstPath,则直接替换原文件
	void RenameFilesInDirectory(const string &srcPath, const string &dstPath);
}

#endif //

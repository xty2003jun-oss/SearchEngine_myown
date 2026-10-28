#include "DirectoryScanner.h"

#include <dirent.h>   // opendir、readdir、closedir
#include <iostream>

using std::cerr;
using std::endl;

vector<string> DirectoryScanner::scan(const string & dir)
{
    vector<string> result;              // 收集文件名
    DIR * pdir = ::opendir(dir.c_str()); // 打开目录
    if (pdir == nullptr)                 // 目录不存在或没有权限
    {
        cerr << "opendir failed: " << dir << endl;
        return result;                   // 返回空数组
    }

    dirent * ent = nullptr;
    while ((ent = ::readdir(pdir)) != nullptr)  // 每次读一个目录项，读完返回空
    {
        string name = ent->d_name;       // 这一项的名字
        if (name == "." || name == "..") // 当前目录和上级目录，不是语料文件
        {
            continue;
        }
        result.push_back(name);          // 只保存名字，路径由调用者拼接
    }
    ::closedir(pdir);                    // 关掉目录流
    return result;
}
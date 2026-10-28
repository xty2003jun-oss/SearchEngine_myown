#pragma once
#include <string>
#include <vector>
using std::string;
using std::vector;
class DirectoryScanner
{
public:
    // 返回 dir 里面每个文件的名字，不含路径。"." 和 ".." 不要放进去
    static vector<string> scan(const string & dir);
private:
    DirectoryScanner() = delete;  // 只有静态函数，禁止创建对象
};
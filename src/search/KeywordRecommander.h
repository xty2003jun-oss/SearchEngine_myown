#pragma once

#include <string>
#include <unordered_map>   // 行号查词、单字查行号
#include <unordered_set>
#include <utility>         // pair
#include <vector>

using std::pair;
using std::string;
using std::unordered_map;
using std::unordered_set;
using std::vector;

class KeywordRecommander
{
public:
    void init();                              // 读四份文件。必须在线程池启动前调用
    string query(const string & keyword);     // 返回 JSON 数组，例如 ["一下","一个"]

private:
    void loadDict(const string & path, unordered_map<int, pair<string, int>> & dict);
    void loadIndex(const string & path, unordered_map<string, unordered_set<int>> & index);
    bool useChinese(const string & keyword);  // 含有高位字节就走中文库
    vector<string> splitUtf8(const string & text);  // 按字切开，编辑距离不能按字节算
    int editDistance(const vector<string> & a, const vector<string> & b);

    unordered_map<int, pair<string, int>> _dictCn;   // 行号 -> (词, 词频)
    unordered_map<int, pair<string, int>> _dictEn;
    unordered_map<string, unordered_set<int>> _indexCn;  // 字 -> 词典行号
    unordered_map<string, unordered_set<int>> _indexEn;
};
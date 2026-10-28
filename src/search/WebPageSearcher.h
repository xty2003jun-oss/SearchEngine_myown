#pragma once

#include <cppjieba/Jieba.hpp>   // 把查询句子切开

#include <fstream>               // 按偏移读取网页库
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <utility>

using std::map;
using std::string;
using std::unordered_map;
using std::unordered_set;
using std::vector;
using std::pair;

class WebPageSearcher
{
public:
    WebPageSearcher();
    void init();                               // 读倒排索引、偏移和停用词
    string query(const string & sentence);     // 返回 JSON 数组

private:
    vector<string> cutQuery(const string & sentence);          // 分词并去掉停用词
    string readDoc(int id);                                    // 按偏移读出一整篇 <doc>
    string between(const string & xml, const string & beginTag, const string & endTag);  // 取出标签中间的文字
    string abstractOf(const string & content);                 // 摘要只取正文开头一段

    cppjieba::Jieba _jieba;                                    // 查询分词用
    unordered_map<string, map<int, double>> _index;            // 词 -> (文档id -> 权重)
    unordered_map<int, pair<long, long>> _offset;              // 文档id -> (起点, 字节数)
    unordered_set<string> _stop;                               // 停用词
    std::ifstream _pages;                                      // 一直开着 ripepage.dat，按起点跳着读
};
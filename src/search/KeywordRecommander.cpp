#include "KeywordRecommander.h"

#include <nlohmann/json.hpp>  // 把推荐词打成 JSON 数组
#include <algorithm>          // sort
#include <fstream>
#include <sstream>

using json = nlohmann::json;
using std::ifstream;
using std::istringstream;

void KeywordRecommander::init()
{
    loadDict("dicts/dict_cn.dat", _dictCn);       // 一行：词 词频，行号从 1 开始
    loadDict("dicts/dict_en.dat", _dictEn);
    loadIndex("indexes/index_cn.dat", _indexCn);  // 一行：字 行号 行号 ...
    loadIndex("indexes/index_en.dat", _indexEn);
}

void KeywordRecommander::loadDict(const string & path, unordered_map<int, pair<string, int>> & dict)
{
    ifstream in(path);          // 打开词典
    string line;
    int lineNo = 1;             // 和建索引时的行号对齐，第一行是 1
    while (getline(in, line))
    {
        istringstream iss(line);
        string word;
        int freq = 0;
        iss >> word >> freq;                 // 空格前是词，后面是次数
        dict[lineNo] = pair<string, int>(word, freq);
        ++lineNo;
    }
}

void KeywordRecommander::loadIndex(const string & path, unordered_map<string, unordered_set<int>> & index)
{
    ifstream in(path);
    string line;
    while (getline(in, line))
    {
        istringstream iss(line);
        string key;                         // 中文是一个字，英文是一个字母
        iss >> key;
        int lineNo = 0;
        while (iss >> lineNo)               // 后面每个整数都是词典行号
        {
            index[key].insert(lineNo);
        }
    }
}

bool KeywordRecommander::useChinese(const string & keyword)
{
    for (unsigned char ch : keyword)        // 高位为 1 的字节属于多字节的 UTF-8 字
    {
        if (ch >= 0x80)
        {
            return true;                    // 有汉字就查中文词典
        }
    }
    return false;                           // 纯 ASCII 查英文词典
}

vector<string> KeywordRecommander::splitUtf8(const string & text)
{
    vector<string> chars;                   // 每个元素是一个字，不是一个字节
    for (size_t i = 0; i < text.size(); )
    {
        unsigned char c = text[i];
        size_t n = 1;                       // ASCII 默认 1 字节
        if ((c & 0xF0) == 0xE0)             // 汉字通常是 3 字节
        {
            n = 3;
        }
        else if ((c & 0xE0) == 0xC0)
        {
            n = 2;
        }
        else if ((c & 0xF8) == 0xF0)
        {
            n = 4;
        }
        if (i + n > text.size())            // 末尾残缺时只取剩下的
        {
            n = text.size() - i;
        }
        chars.push_back(text.substr(i, n));
        i += n;
    }
    return chars;
}

int KeywordRecommander::editDistance(const vector<string> & a, const vector<string> & b)
{
    int n = a.size();                       // 查询被切成多少个字
    int m = b.size();                       // 候选词被切成多少个字
    vector<vector<int>> dp(n + 1, vector<int>(m + 1));  // dp[i][j] 是 a 前 i 个字变成 b 前 j 个字的最少步数
    for (int i = 0; i <= n; ++i)
    {
        dp[i][0] = i;                       // 删掉 i 个字
    }
    for (int j = 0; j <= m; ++j)
    {
        dp[0][j] = j;                       // 插入 j 个字
    }
    for (int i = 1; i <= n; ++i)
    {
        for (int j = 1; j <= m; ++j)
        {
            int cost = a[i - 1] == b[j - 1] ? 0 : 1;  // 同一个字不用替换
            int del = dp[i - 1][j] + 1;               // 删除 a 的当前字
            int ins = dp[i][j - 1] + 1;               // 插入 b 的当前字
            int rep = dp[i - 1][j - 1] + cost;        // 替换
            dp[i][j] = std::min(del, std::min(ins, rep));
        }
    }
    return dp[n][m];
}

string KeywordRecommander::query(const string & keyword)
{
    bool cn = useChinese(keyword);          // 决定查哪一套词典和索引
    auto & dict = cn ? _dictCn : _dictEn;
    auto & index = cn ? _indexCn : _indexEn;

    unordered_set<int> lines;               // 候选词的词典行号，用集合去重
    for (const string & ch : splitUtf8(keyword))
    {
        auto it = index.find(ch);          // 这个字在索引里吗
        if (it == index.end())
        {
            continue;                       // 索引没有这个字，就没有对应候选
        }
        lines.insert(it->second.begin(), it->second.end());  // 合并这个字出现过的所有行
    }

    struct Candidate
    {
        string word;
        int freq;
        int dist;
    };
    vector<Candidate> all;                  // 不要用 priority_queue 再 memset，容器不能整块清零
    vector<string> queryChars = splitUtf8(keyword);
    for (int lineNo : lines)
    {
        auto it = dict.find(lineNo);
        if (it == dict.end())               // 索引行号在词典里找不到就跳过
        {
            continue;
        }
        const string & word = it->second.first;
        int freq = it->second.second;
        int dist = editDistance(queryChars, splitUtf8(word));
        all.push_back(Candidate{word, freq, dist});
    }

    std::sort(all.begin(), all.end(), [](const Candidate & a, const Candidate & b)
    {
        if (a.dist != b.dist)
        {
            return a.dist < b.dist;         // 编辑距离小的在前
        }
        if (a.freq != b.freq)
        {
            return a.freq > b.freq;         // 距离相同，词频高的在前
        }
        return a.word < b.word;             // 再按字典序
    });

    json arr = json::array();               // JSON 数组，不是一个对象
    int limit = 5;                          // 只返回前 5 个
    for (int i = 0; i < static_cast<int>(all.size()) && i < limit; ++i)
    {
        arr.push_back(all[i].word);         // 只要词，不把距离和词频发给客户端
    }
    return arr.dump();                      // 变成 "[\"一下\",\"一个\"]" 这种字符串
}
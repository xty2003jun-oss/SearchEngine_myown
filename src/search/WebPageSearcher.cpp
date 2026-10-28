#include "WebPageSearcher.h"

#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <sstream>

using json = nlohmann::json;
using std::ifstream;
using std::istringstream;

static const char * DICT = "/home/xty/SearchEngine/third_party/cppjieba/dict/jieba.dict.utf8";
static const char * HMM  = "/home/xty/SearchEngine/third_party/cppjieba/dict/hmm_model.utf8";
static const char * USER = "/home/xty/SearchEngine/third_party/cppjieba/dict/user.dict.utf8";
static const char * IDF  = "/home/xty/SearchEngine/third_party/cppjieba/dict/idf.utf8";
static const char * STOP = "/home/xty/SearchEngine/third_party/cppjieba/dict/stop_words.utf8";

WebPageSearcher::WebPageSearcher()
: _jieba(DICT, HMM, USER, IDF, STOP)  // 和离线分词用同一套词典，切出来的词才能对上倒排索引
{}

void WebPageSearcher::init()
{
    ifstream indexIn("webpage/inverted_index.dat");  // 一行：词 id 权重 id 权重
    string line;
    while (getline(indexIn, line))
    {
        if (line.empty())                    // 文件开头可能有空行
        {
            continue;
        }
        istringstream iss(line);
        string word;
        iss >> word;                         // 第一个字段是词
        int id = 0;
        double weight = 0;
        while (iss >> id >> weight)          // 后面两个一组：文档 id 和这篇里的权重
        {
            _index[word][id] = weight;
        }
    }

    ifstream offsetIn("webpage/offsetpage.dat");  // 一行：id 起点 字节数
    int id = 0;
    long start = 0;
    long size = 0;
    while (offsetIn >> id >> start >> size)
    {
        _offset[id] = std::make_pair(start, size);
    }

    ifstream cn("corpus/stopwords/cn_stopwords.txt");  // 查询里的停用词不参与检索
    ifstream en("corpus/stopwords/en_stopwords.txt");
    string word;
    while (cn >> word)
    {
        _stop.insert(word);
    }
    while (en >> word)
    {
        _stop.insert(word);
    }

    _pages.open("webpage/ripepage.dat");     // 打开后留着，每次按起点跳过去读
}

vector<string> WebPageSearcher::cutQuery(const string & sentence)
{
    vector<string> raw;
    _jieba.Cut(sentence, raw);               // 把一句话切成词
    vector<string> words;
    for (const string & word : raw)
    {
        if (word.empty() || _stop.count(word) != 0)  // 空词和停用词不查
        {
            continue;
        }
        words.push_back(word);
    }
    return words;
}

string WebPageSearcher::readDoc(int id)
{
    auto it = _offset.find(id);              // 没有偏移就读不了这篇
    if (it == _offset.end() || !_pages)
    {
        return "";
    }
    long start = it->second.first;           // 这篇在 ripepage.dat 里的起点
    long size = it->second.second;           // 这篇占多少字节
    _pages.clear();                          // 上次读到文件尾时，要先清掉错误状态
    _pages.seekg(start);                     // 跳到这篇的开头
    string xml(size, '\0');                  // 按字节数准备缓冲区
    _pages.read(&xml[0], size);              // 读出 <doc>...</doc>
    return xml;
}

string WebPageSearcher::between(const string & xml, const string & beginTag, const string & endTag)
{
    size_t begin = xml.find(beginTag);       // 找开始标签
    if (begin == string::npos)
    {
        return "";
    }
    begin += beginTag.size();                // 挪到标签后面的正文
    size_t end = xml.find(endTag, begin);    // 从这里找结束标签
    if (end == string::npos)
    {
        return "";
    }
    return xml.substr(begin, end - begin);   // 两个标签中间的文字
}

string WebPageSearcher::abstractOf(const string & content)
{
    string text;
    size_t i = 0;
    int count = 0;
    while (i < content.size() && count < 40) // 摘要最多 40 个字，按 UTF-8 切，不要切在汉字中间
    {
        unsigned char c = content[i];
        size_t n = 1;
        if ((c & 0xF0) == 0xE0)
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
        if (i + n > content.size())
        {
            break;
        }
        text.append(content, i, n);          // 追加这一个字
        i += n;
        ++count;
    }
    return text;
}

string WebPageSearcher::query(const string & sentence)
{
    vector<string> words = cutQuery(sentence);
    json arr = json::array();                // 没有可查的词时返回空数组
    if (words.empty())
    {
        return arr.dump();
    }

    // 只保留同时包含全部查询词的文档。从第一个词的文档集合开始，再和后面的求交集
    map<int, double> scores;                 // 文档 id -> 和查询的点积
    bool first = true;
    for (const string & word : words)
    {
        auto it = _index.find(word);
        if (it == _index.end())              // 有一个词根本不在索引里，交集一定是空的
        {
            return arr.dump();
        }
        if (first)
        {
            for (const auto & one : it->second)
            {
                scores[one.first] = one.second;  // 第一词：文档的初始分数就是这个词的权重
            }
            first = false;
            continue;
        }
        map<int, double> next;               // 和当前词求交集后的结果
        for (const auto & one : it->second)
        {
            auto old = scores.find(one.first);
            if (old != scores.end())         // 这篇文档前面的每个查询词都有
            {
                next[one.first] = old->second + one.second;  // 把这个词的权重加进分数
            }
        }
        scores.swap(next);                   // 交集缩小一圈
        if (scores.empty())
        {
            return arr.dump();
        }
    }

    vector<pair<double, int>> ranked;        // 分数在前，方便从大到小排
    for (const auto & one : scores)
    {
        ranked.push_back(std::make_pair(one.second, one.first));
    }
    std::sort(ranked.begin(), ranked.end(), [](const pair<double, int> & a, const pair<double, int> & b)
    {
        return a.first > b.first;            // 分数高的在前
    });

    int limit = 5;                           // 最多返回 5 篇
    for (int i = 0; i < static_cast<int>(ranked.size()) && i < limit; ++i)
    {
        int id = ranked[i].second;
        string xml = readDoc(id);           // 按偏移读出这篇网页
        json item;
        item["id"] = std::to_string(id);
        item["title"] = between(xml, "<title>", "</title>");
        item["link"] = between(xml, "<link>", "</link>");
        item["abstract"] = abstractOf(between(xml, "<content>", "</content>"));
        arr.push_back(item);
    }
    return arr.dump();
}
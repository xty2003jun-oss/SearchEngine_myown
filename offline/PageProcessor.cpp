#include "PageProcessor.h"

#include <tinyxml2.h>     // 读 RSS 的 XML
#include <cmath>          // log2、sqrt
#include <fstream>
#include <iostream>

using std::cerr;
using std::cout;
using std::endl;
using std::ifstream;
using std::ofstream;

static const char * DICT = "/home/xty/SearchEngine/third_party/cppjieba/dict/jieba.dict.utf8";
static const char * HMM  = "/home/xty/SearchEngine/third_party/cppjieba/dict/hmm_model.utf8";
static const char * USER = "/home/xty/SearchEngine/third_party/cppjieba/dict/user.dict.utf8";
static const char * IDF  = "/home/xty/SearchEngine/third_party/cppjieba/dict/idf.utf8";
static const char * STOP = "/home/xty/SearchEngine/third_party/cppjieba/dict/stop_words.utf8";

PageProcessor::PageProcessor()
: _jieba(DICT, HMM, USER, IDF, STOP)     // 分词器，路径必须写明
, _hasher(DICT, HMM, IDF, STOP)          // Simhasher 内部也有一个分词器，同样要这四份词典
{
    ifstream cn("../corpus/stopwords/cn_stopwords.txt");  // 相对 offline 目录
    ifstream en("../corpus/stopwords/en_stopwords.txt");
    string word;
    while (cn >> word)                   // 中文停用词
    {
        _stop.insert(word);
    }
    while (en >> word)                   // 英文停用词
    {
        _stop.insert(word);
    }
}

void PageProcessor::process(const string & dir)
{
    extract(dir);                                            // 先全部读进来
    dedup();                                                 // 再丢掉太像的
    writePages("../webpage/ripepage.dat", "../webpage/offsetpage.dat");
    writeIndex("../webpage/inverted_index.dat");
}

string PageProcessor::stripTags(const string & html)
{
    string text;                             // 去掉标签后的正文
    bool inTag = false;                      // 当前是不是正在一个 <...> 里面
    for (char ch : html)
    {
        if (ch == '<')                       // 标签开始，后面的字符先不收
        {
            inTag = true;
        }
        else if (ch == '>')                  // 标签结束
        {
            inTag = false;
        }
        else if (!inTag)                     // 标签外面的才是正文
        {
            text.push_back(ch);
        }
    }
    return text;
}

void PageProcessor::extract(const string & dir)
{
    vector<string> names = DirectoryScanner::scan(dir);  // 每个 xml 文件名
    int id = 1;                                          // 去重前的临时编号
    for (const string & name : names)
    {
        string path = dir + "/" + name;                  // 拼出文件路径
        tinyxml2::XMLDocument xml;                       // 一份 XML 文档
        if (xml.LoadFile(path.c_str()) != tinyxml2::XML_SUCCESS)  // 不是合法 XML 就跳过
        {
            continue;
        }
        tinyxml2::XMLElement * rss = xml.FirstChildElement("rss");          // 根下面的 rss
        if (rss == nullptr)
        {
            continue;
        }
        tinyxml2::XMLElement * channel = rss->FirstChildElement("channel"); // channel 里有很多 item
        if (channel == nullptr)
        {
            continue;
        }
        for (tinyxml2::XMLElement * item = channel->FirstChildElement("item");
             item != nullptr;
             item = item->NextSiblingElement("item"))    // 下一个兄弟还是 item 才继续
        {
            Document doc;
            doc.id = id++;                               // 先给一个临时 id，去重后会重编
            tinyxml2::XMLElement * link = item->FirstChildElement("link");
            tinyxml2::XMLElement * title = item->FirstChildElement("title");
            tinyxml2::XMLElement * content = item->FirstChildElement("content");
            if (content == nullptr)                      // 有的 RSS 把正文放在 description
            {
                content = item->FirstChildElement("description");
            }
            if (link && link->GetText())                 // GetText 为空说明这个标签没有文字
            {
                doc.link = link->GetText();
            }
            if (title && title->GetText())
            {
                doc.title = title->GetText();
            }
            if (content && content->GetText())
            {
                doc.content = stripTags(content->GetText());  // 去掉 HTML 再保存
            }
            if (!doc.content.empty())                    // 没有正文的条目不做索引
            {
                _docs.push_back(doc);
            }
        }
    }
}

void PageProcessor::dedup()
{
    vector<Document> kept;          // 留下来的网页
    vector<uint64_t> fingers;       // 和 kept 一一对应的指纹
    int id = 1;                     // 去重后的编号从 1 重新数，偏移文件用这个 id
    for (Document & doc : _docs)
    {
        uint64_t code = 0;                          // 这一篇的 64 位指纹
        _hasher.make(doc.content, 5, code);         // 用权重最高的 5 个词生成指纹
        bool same = false;
        for (uint64_t old : fingers)
        {
            if (simhash::Simhasher::isEqual(old, code))  // 距离足够近就视为重复
            {
                same = true;
                break;
            }
        }
        if (!same)
        {
            doc.id = id++;              // 只给留下的文档编号
            fingers.push_back(code);
            kept.push_back(doc);
        }
    }
    _docs.swap(kept);                   // 用去重后的数组替换原来的
}

void PageProcessor::writePages(const string & pageFile, const string & offsetFile)
{
    ofstream pages(pageFile);          // 网页正文库
    ofstream offsets(offsetFile);      // 每篇的起点和长度
    for (const Document & doc : _docs)
    {
        std::streampos start = pages.tellp();   // 写这篇之前，文件已经有多长
        pages << "<doc>\n";
        pages << "<id>" << doc.id << "</id>\n";
        pages << "<link>" << doc.link << "</link>\n";
        pages << "<title>" << doc.title << "</title>\n";
        pages << "<content>" << doc.content << "</content>\n";
        pages << "</doc>\n";
        std::streampos end = pages.tellp();     // 写完这篇之后的位置
        offsets << doc.id << " " << start << " " << (end - start) << "\n";  // id 起点 字节数
    }
}


bool PageProcessor::keepWord(const string & word)
{
    if (word.empty())                  // 空词没有意义
    {
        return false;
    }
    if (_stop.count(word) != 0)        // 停用词丢掉
    {
        return false;
    }
    return true;
}

void PageProcessor::writeIndex(const string & indexFile)
{
    // wordDocs[词] = 出现过这个词的文档 id
    map<string, set<int>> wordDocs;
    // docTf[文档id][词] = 该词在这篇里的次数 / 这篇的总词数
    map<int, map<string, double>> docTf;
    for (const Document & doc : _docs)
    {
        vector<string> words;
        _jieba.Cut(doc.content, words);         // 正文分词
        map<string, int> count;                 // 这一篇里每个词出现几次
        int total = 0;                          // 这一篇留下的词一共有多少个
        for (const string & word : words)
        {
            if (!keepWord(word))
            {
                continue;
            }
            count[word]++;                      // 次数加 1
            ++total;
        }
        if (total == 0)                         // 整篇都是停用词，不建索引
        {
            continue;
        }
        for (const auto & item : count)
        {
            docTf[doc.id][item.first] = static_cast<double>(item.second) / total;  // TF
            wordDocs[item.first].insert(doc.id);  // 这个词出现在哪几篇
        }
    }

    double docCount = _docs.size();             // 文档总数，算 IDF 用
    // weight[词][文档id] = 归一化前的 TF-IDF
    map<string, map<int, double>> weight;
    for (const auto & item : wordDocs)
    {
        const string & word = item.first;
        double df = item.second.size();         // 有多少篇包含这个词
        double idf = std::log2(docCount / df);  // 越少见的词，IDF 越大
        for (int id : item.second)
        {
            weight[word][id] = docTf[id][word] * idf;
        }
    }

    // 每篇文档各自做归一化，避免长网页只因为词多而权重特别大
    map<int, double> norm;
    for (const auto & item : weight)
    {
        for (const auto & one : item.second)
        {
            norm[one.first] += one.second * one.second;  // 先累加平方
        }
    }
    for (auto & item : norm)
    {
        item.second = std::sqrt(item.second);            // 再开方，得到这篇的向量长度
    }
    for (auto & item : weight)
    {
        for (auto & one : item.second)
        {
            if (norm[one.first] > 0)
            {
                one.second /= norm[one.first];           // 权重除以这篇的向量长度
            }
        }
    }

    ofstream out(indexFile);
    for (const auto & item : weight)
    {
        out << item.first;                               // 先写词
        for (const auto & one : item.second)
        {
            out << " " << one.first << " " << one.second;  // 再写 文档id 权重
        }
        out << "\n";
    }
}
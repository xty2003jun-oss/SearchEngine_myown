#pragma once

#include "DirectoryScanner.h"          // 列出网页目录里的文件名
#include <cppjieba/Jieba.hpp>          // 给正文分词，用来算倒排索引
#include <simhash/Simhasher.hpp>       // 给正文做指纹，用来去重
#include <map>
#include <set>
#include <string>
#include <vector>


using std::map;
using std::set;
using std::string;
using std::vector;
class PageProcessor
{
public:
    
    PageProcessor();                               // 准备分词器、指纹器和停用词
    void process(const string & dir);              // dir 是 RSS 所在目录
private:
    
    struct Document                                // 从一条 RSS item 抽出的一篇网页
    {
        int id;                                    // 去重之后重新从 1 编号
        string link;                               // 原文链接
        string title;                              // 标题
        string content;                            // 去掉 HTML 后的正文
    };
    
    
    void extract(const string & dir);              // 把每个 item 放进 _docs
    void dedup();                                  // 指纹太近的丢掉
    void writePages(const string & pageFile, const string & offsetFile);  // 网页库和偏移
    void writeIndex(const string & indexFile);     // 倒排索引
    string stripTags(const string & html);         // 删掉 <...> 标签
    bool keepWord(const string & word);            // 停用词和空词不要
    
    
    cppjieba::Jieba _jieba;                        // 倒排索引用的分词器
    simhash::Simhasher _hasher;                    // 去重用的指纹
    set<string> _stop;                             // 中英文停用词合在一个集合里
    vector<Document> _docs;                        // 当前还保留的网页
};
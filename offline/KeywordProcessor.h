#pragma once

#include <cppjieba/Jieba.hpp>  // 中文分词

#include <map>
#include <set>
#include <string>

using std::map;
using std::set;
using std::string;

class KeyWordProcessor
{
public:
    KeyWordProcessor();  // 加载中英文停用词，并准备分词器

    // chDir 是中文语料目录，enDir 是英文语料目录
    void process(const string & chDir, const string & enDir);

private:
    void createCnDict(const string & dir, const string & outfile);   // 中文词典：词 词频
    void buildCnIndex(const string & dictFile, const string & indexFile);  // 中文索引：字 行号...
    void createEnDict(const string & dir, const string & outfile);   // 英文词典
    void buildEnIndex(const string & dictFile, const string & indexFile);  // 英文字母索引

    bool isPureChinese(const string & word);  // 每个字都是 UTF-8 汉字才留下
    void loadWords(const string & path, set<string> & out);  // 把停用词文件读进 set

    cppjieba::Jieba _jieba;          // 中文分词器。必须在初始化列表里构造
    set<string> _cnStop;             // 中文停用词
    set<string> _enStop;             // 英文停用词
};
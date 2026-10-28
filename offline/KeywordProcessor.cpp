#include "KeywordProcessor.h"
#include "DirectoryScanner.h"
#include <cctype>     // isalpha、tolower
#include <fstream>    // ifstream、ofstream
#include <iostream>
#include <vector>
#include <string>

using std::cerr;
using std::cout;
using std::endl;
using std::ifstream;
using std::ofstream;
using std::vector;

static const char * DICT = "/home/xty/SearchEngine/third_party/cppjieba/dict/jieba.dict.utf8";
static const char * HMM  = "/home/xty/SearchEngine/third_party/cppjieba/dict/hmm_model.utf8";
static const char * USER = "/home/xty/SearchEngine/third_party/cppjieba/dict/user.dict.utf8";
static const char * IDF  = "/home/xty/SearchEngine/third_party/cppjieba/dict/idf.utf8";
static const char * STOP = "/home/xty/SearchEngine/third_party/cppjieba/dict/stop_words.utf8";

KeyWordProcessor::KeyWordProcessor()
: _jieba(DICT, HMM, USER, IDF, STOP)  // 五个词典路径都显式给出
{
    loadWords("../corpus/stopwords/cn_stopwords.txt", _cnStop);  // 相对 offline 目录
    loadWords("../corpus/stopwords/en_stopwords.txt", _enStop);
}

void KeyWordProcessor::loadWords(const string & path, set<string> & out)
{
    ifstream in(path);          // 打开停用词文件
    if (!in)                    // 打不开就没法过滤
    {
        cerr << "open failed: " << path << endl;
        return;
    }
    string word;
    while (in >> word)          // 按空白读出一个词
    {
        out.insert(word);       // set 用来判断某个词是不是停用词
    }
}

void KeyWordProcessor::process(const string & chDir, const string & enDir)
{
    createCnDict(chDir, "../dicts/dict_cn.dat");                 // 先有词典
    buildCnIndex("../dicts/dict_cn.dat", "../indexes/index_cn.dat");  // 行号必须和刚写出的词典一致
    createEnDict(enDir, "../dicts/dict_en.dat");
    buildEnIndex("../dicts/dict_en.dat", "../indexes/index_en.dat");
}

bool KeyWordProcessor::isPureChinese(const string & word)
{
    if (word.empty())                 // 空串不是中文词
    {
        return false;
    }
    if (word.size() % 3 != 0)         // 这里只接受 3 字节一个字的常用汉字
    {
        return false;
    }
    for (size_t i = 0; i < word.size(); i += 3)  // 每次看一个汉字
    {
        unsigned char c1 = word[i];
        unsigned char c2 = word[i + 1];
        unsigned char c3 = word[i + 2];
        // 常用汉字 UTF-8 首字节在 0xE4 到 0xE9，后两字节都在 0x80 到 0xBF
        if (!(c1 >= 0xE4 && c1 <= 0xE9) ||
            !(c2 >= 0x80 && c2 <= 0xBF) ||
            !(c3 >= 0x80 && c3 <= 0xBF))
        {
            return false;
        }
    }
    return true;
}

void KeyWordProcessor::createCnDict(const string & dir, const string & outfile)
{
    map<string, int> dict;                         // 词 -> 出现次数
    vector<string> names = DirectoryScanner::scan(dir);  // 这个目录下所有文件名
    for (const string & name : names)
    {
        ifstream in(dir + "/" + name);             // 拼出完整路径
        if (!in)
        {
            continue;                              // 打不开就处理下一个文件
        }
        string content;                            // 整篇拼在一起再分词
        string line;
        while (getline(in, line))                  // 一行一行读
        {
            content += line;                       // 去掉换行，避免把一句话切成两截
        }
        vector<string> words;
        _jieba.Cut(content, words);                // cppjieba 切出词
        for (const string & word : words)
        {
            if (!isPureChinese(word))              // 标点、英文、数字丢掉
            {
                continue;
            }
            if (_cnStop.count(word) != 0)          // 停用词丢掉
            {
                continue;
            }
            dict[word]++;                          // 这个词的次数加 1
        }
    }
    ofstream out(outfile);                         // 覆盖写出词典
    if (!out)
    {
        cerr << "open failed: " << outfile << endl;
        return;
    }
    for (const auto & item : dict)                 // map 按词的字典序输出
    {
        out << item.first << " " << item.second << endl;  // 一行：词 词频
    }
}

static string nextUtf8(const string & word, size_t & i)
{
    unsigned char c = word[i];     // 看首字节决定这个字有多长
    size_t n = 1;
    if ((c & 0xF0) == 0xE0)        // 1110xxxx，三个字节
    {
        n = 3;
    }
    else if ((c & 0xE0) == 0xC0)   // 110xxxxx，两个字节
    {
        n = 2;
    }
    else if ((c & 0xF8) == 0xF0)   // 11110xxx，四个字节
    {
        n = 4;
    }
    if (i + n > word.size())       // 残缺的字节序列，只取剩下的
    {
        n = word.size() - i;
    }
    string one = word.substr(i, n);  // 这一个字
    i += n;                          // 调用者下一次从下一个字开始
    return one;
}

void KeyWordProcessor::buildCnIndex(const string & dictFile, const string & indexFile)
{
    ifstream in(dictFile);                    // 刚生成的词典
    map<string, set<int>> index;               // 字 -> 出现过这个字的行号
    string line;
    int lineNo = 1;                            // 第一行的行号是 1，后面查询也从 1 数
    while (getline(in, line))
    {
        string word = line.substr(0, line.find(' '));  // 空格前面是词，后面是词频
        for (size_t i = 0; i < word.size(); )          // i 由 nextUtf8 往前推
        {
            string one = nextUtf8(word, i);            // 取出一个字
            index[one].insert(lineNo);                 // 这个字在第 lineNo 行出现
        }
        ++lineNo;                                      // 下一行
    }
    ofstream out(indexFile);
    for (const auto & item : index)
    {
        out << item.first;                             // 先写字
        for (int no : item.second)                     // set 里的行号已经从小到大
        {
            out << " " << no;
        }
        out << endl;                                   // 一行：字 行号 行号 ...
    }
}

void KeyWordProcessor::createEnDict(const string & dir, const string & outfile)
{
    map<string, int> dict;
    vector<string> names = DirectoryScanner::scan(dir);
    for (const string & name : names)
    {
        ifstream in(dir + "/" + name);
        if (!in)
        {
            continue;
        }
        string raw;
        while (in >> raw)                         // 按空白取出一段
        {
            string word;
            for (unsigned char ch : raw)           // 用 unsigned char，避免负数字节
            {
                if (isalpha(ch))                   // 是字母
                {
                    word.push_back(tolower(ch));   // 统一成小写
                }
                else if (!word.empty())            // 遇到标点，先结算前面的单词
                {
                    if (_enStop.count(word) == 0)
                    {
                        dict[word]++;
                    }
                    word.clear();                  // 准备接收标点后面的下一个词
                }
            }
            if (!word.empty() && _enStop.count(word) == 0)  // 这一段结尾也是单词
            {
                dict[word]++;
            }
        }
    }
    ofstream out(outfile);
    for (const auto & item : dict)
    {
        out << item.first << " " << item.second << endl;
    }
}

void KeyWordProcessor::buildEnIndex(const string & dictFile, const string & indexFile)
{
    ifstream in(dictFile);
    map<char, set<int>> index;          // 字母 -> 行号
    string line;
    int lineNo = 1;
    while (getline(in, line))
    {
        string word = line.substr(0, line.find(' '));
        for (char ch : word)            // 英文词典里已经都是小写字母
        {
            index[ch].insert(lineNo);   // 这个字母出现在第 lineNo 行
        }
        ++lineNo;
    }
    ofstream out(indexFile);
    for (const auto & item : index)
    {
        out << item.first;
        for (int no : item.second)
        {
            out << " " << no;
        }
        out << endl;
    }
}
#include "log.h"

#include <log4cpp/OstreamAppender.hh>       // 输出到 cout
#include <log4cpp/FileAppender.hh>          // 输出到一个固定文件
#include <log4cpp/RollingFileAppender.hh>   // 文件太大就换一个新文件
#include <log4cpp/PatternLayout.hh>         // 每行日志的格式
#include <log4cpp/Priority.hh>              // INFO、ERROR 这些级别

#include <iostream>  // cout
#include <string>    // to_string

using std::string;
using std::to_string;

string my_Logger::format(const char * file, int line, const char * func, const char * msg)
{
    string text(file);            // 源文件名
    text += ":";                  // 分隔符
    text += to_string(line);      // 行号
    text += " ";
    text += func;                 // 函数名
    text += " ";
    text += msg;                  // 你传入的那句话
    return text;
}

void my_Logger::info(const char * file, int line, const char * func, const char * msg)
{
    _root->info(format(file, line, func, msg));  // 级别是 INFO，低于这个级别的日志不会出现
}

void my_Logger::error(const char * file, int line, const char * func, const char * msg)
{
    _root->error(format(file, line, func, msg));  // 级别是 ERROR
}

my_Logger::my_Logger()
{
    // 三个输出目标。名字是 log4cpp 内部用来区分它们的，不要重复
    log4cpp::Appender * toConsole = new log4cpp::OstreamAppender("console", &std::cout);
    log4cpp::Appender * toFile = new log4cpp::FileAppender("file", "file_log.txt");
    // 每个备份文件最多 5KB，最多保留 9 个旧文件
    log4cpp::Appender * toRolling = new log4cpp::RollingFileAppender("rolling", "backup_log.txt", 5 * 1024, 9);

    // 一个 Layout 只能绑给一个 Appender，所以要三个
    log4cpp::PatternLayout * layout1 = new log4cpp::PatternLayout();
    log4cpp::PatternLayout * layout2 = new log4cpp::PatternLayout();
    log4cpp::PatternLayout * layout3 = new log4cpp::PatternLayout();
    // %d 时间，%p 级别，%m 正文，%n 换行
    layout1->setConversionPattern("%d %p %m%n");
    layout2->setConversionPattern("%d %p %m%n");
    layout3->setConversionPattern("%d %p %m%n");

    toConsole->setLayout(layout1);   // 终端使用第一种格式
    toFile->setLayout(layout2);      // 普通文件使用第二种
    toRolling->setLayout(layout3);   // 滚动文件使用第三种

    _root = &log4cpp::Category::getRoot();     // 取根记录器。整个进程共用这一个
    _root->setPriority(log4cpp::Priority::INFO);  // 只输出 INFO 和比它更严重的 ERROR
    _root->addAppender(toConsole);             // 同时写到三个地方
    _root->addAppender(toFile);
    _root->addAppender(toRolling);
}

my_Logger::~my_Logger()
{
    log4cpp::Category::shutdown();  // 关掉所有 Appender，把缓冲区刷到文件
}
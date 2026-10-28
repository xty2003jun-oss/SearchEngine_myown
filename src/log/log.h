#pragma once
#include <log4cpp/Category.hh>
#include <string>  // 拼日志正文

// 包一层 log4cpp。业务代码只调用 info / error，不直接碰 Category
class my_Logger
{
public:
    my_Logger();   // 配置输出到终端和文件
    ~my_Logger();  // 关闭 log4cpp

    // file、line、func 用来标出这条日志是哪一行打的
    void info(const char * file, int line, const char * func, const char * msg);   // 正常事件：连上、收到、关闭
    void error(const char * file, int line, const char * func, const char * msg);  // 出错：没读全、长度非法

private:
    // 把文件名、行号、函数名、正文拼成一条字符串
    std::string format(const char * file, int line, const char * func, const char * msg);

    log4cpp::Category * _root;  // 根记录器。真正往终端和文件写的是它
};
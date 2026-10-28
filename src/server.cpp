#include "TcpServer.h"  // 启动监听和 epoll
#include "Message.h" 
#include "ThreadPool.h"
#include "log.h"
#include "KeywordRecommander.h"
#include "WebPageSearcher.h"
#include "LruKCache.h"
#include <iostream>


using std::cout;
using std::endl;

ThreadPool pool(4, 10);
my_Logger logger;
KeywordRecommander recommander;
WebPageSearcher searcher; 
LruKCache<string, string> cache(100, 150, 2);  // 主缓存 100 条，历史 150 条，看到 2 次才进主缓存

void onNew(const TcpConnectionPtr & con)
{
    string text = con->toString() + " connected";           // 拼出“谁连上了”
    logger.info(__FILE__, __LINE__, __func__, text.c_str()); // __FILE__ 是当前文件名，__LINE__ 是这一行，__func__ 是函数名
}


void onMessage(const TcpConnectionPtr & con)
{
    Message msg;
    if (!con->receive(msg))
    {
        logger.error(__FILE__, __LINE__, __func__, "receive failed");
        return;
    }
    string key = std::to_string(msg.tag) + ":" + msg.value;  // 1:苹果 和 2:苹果 是两个键
    pool.addTask([con, msg, key]()
    {
        string reply;
        if (cache.get(key, reply))               // 历史或主缓存里已有结果
        {
            logger.info(__FILE__, __LINE__, __func__, "cache hit");
        }
        else
        {
            if (msg.tag == 1)
            {
                reply = recommander.query(msg.value);
            }
            else if (msg.tag == 2)
            {
                reply = searcher.query(msg.value);
            }
            else
            {
                reply = "[]";
            }
            cache.put(key, reply);               // 第一次只进入历史队列
            logger.info(__FILE__, __LINE__, __func__, "cache miss");
        }
        con->sendInLoop(reply);
    });
}


void onClose(const TcpConnectionPtr & con)
{
    string text = con->toString() + " closed";
    logger.info(__FILE__, __LINE__, __func__, text.c_str());
}
int main()
{
    TcpServer server("127.0.0.1", 8888);               // 创建套接字和 epoll，还没 listen
    server.setAllCallback(onNew, onMessage, onClose);  // 登记三个函数
    recommander.init();
    searcher.init();
    pool.start();
    server.start();                                    // bind、listen，然后一直等待
}
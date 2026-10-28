#pragma once

#include"Acceptor.h"
#include"TcpConnection.h"

#include<sys/epoll.h>
#include<unistd.h>
#include<vector>
#include<map>
#include<sys/eventfd.h>
#include<mutex>


using std::map;
using std::vector;
using std::mutex;
using std::lock_guard;
using Functor = function<void()>;

class EventLoop
{
public:
    EventLoop(Acceptor & acceptor);  // 引用已经创建好的 Acceptor，不复制它
    ~EventLoop();
    void loop();    // 一直等事件
    void unloop();  // 让 loop 的循环结束
    void setNewConnectionCallback(TcpConnectionCallback cb);  // 保存三个回调
    void setMessageCallback(TcpConnectionCallback cb);
    
    
    void setCloseCallback(TcpConnectionCallback cb);

    void runInLoop(Functor cb);
private:
    int createEpollFd();         // 创建 epoll
    void addEpollFd(int fd);     // 把 fd 放进 epoll
    void delEpollFd(int fd);     // 把 fd 从 epoll 去掉
    void waitEpollFd();          // 等一次，并处理这次所有就绪 fd
    void handleNewConnection();  // 监听套接字就绪
    void handleMessage(int fd);  // 已连接套接字就绪
    int _epfd;                          // epoll 自己的编号
    vector<epoll_event> _evtList;       // 就绪事件数组
    bool _isLooping;                    // 是否继续 loop
    Acceptor & _acceptor;               // 监听器
    map<int, TcpConnectionPtr> _conns;  // 键是连接 fd，值是那条连接
    TcpConnectionCallback _onNewConnection;
    TcpConnectionCallback _onMessage;
    TcpConnectionCallback _onClose;
    int createEventFd();         // 创建一个 eventfd
    void handleRead();           // 把 eventfd 里的通知读掉，否则会一直可读
    void wakeup();               // 往 eventfd 写 8 字节，叫醒 epoll_wait
    void doPendingFunctors();    // 执行积累下来的发送函数
    int _evtfd;                  // 线程之间通信用的 fd
    vector<Functor> _pendings;   // 等待 IO 线程执行的发送函数
    mutex _mutex;

};
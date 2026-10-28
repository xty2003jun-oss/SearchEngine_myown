#include "TcpServer.h"


TcpServer::TcpServer(const string & ip, unsigned short port)
: _acceptor(ip, port)  // 先创建监听套接字和地址
, _eloop(_acceptor)    // 再把 Acceptor 交给 EventLoop，监听 fd 加入 epoll
{}

TcpServer::~TcpServer() {}  // 先析构 EventLoop，再析构 Acceptor


void TcpServer::setAllCallback(TcpConnectionCallback onNew,
                               TcpConnectionCallback onMessage,
                               TcpConnectionCallback onClose)
{
    _eloop.setNewConnectionCallback(onNew);      // 三个回调都交给事件循环
    _eloop.setMessageCallback(onMessage);
    _eloop.setCloseCallback(onClose);
}


void TcpServer::start()
{
    _acceptor.ready();  // 这时才 bind 和 listen
    _eloop.loop();      // 停在 epoll_wait
}


void TcpServer::stop()
{
    _eloop.unloop();    // 把循环标志设为 false
}
#pragma once

#include"Acceptor.h"
#include"EventLoop.h"

class TcpServer
{
public:

    TcpServer(const string& ip, unsigned short port);
    ~TcpServer();

    void start();
    void stop();

    void setAllCallback(TcpConnectionCallback onNew,      // 新连接
                        TcpConnectionCallback onMessage,  // 收到一条消息
                        TcpConnectionCallback onClose);
private:
    Acceptor _acceptor;
    EventLoop _eloop;


};
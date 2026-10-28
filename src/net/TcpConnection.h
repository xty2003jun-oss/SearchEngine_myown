#pragma once

#include"Socket.h"
#include"InetAddress.h"
#include"Message.h"
class EventLoop;

#include<functional>
#include<memory>
#include<string>

using std::function;
using std::shared_ptr;
using std::string;

class TcpConnection;
using TcpConnectionPtr = shared_ptr<TcpConnection>;
using TcpConnectionCallback = function<void(const TcpConnectionPtr&)>;

class TcpConnection
:public std::enable_shared_from_this<TcpConnection>
{
public:
    explicit TcpConnection(int fd,EventLoop * loop);
    ~TcpConnection();

    bool receive(Message & msg);
    void send(const string&msg);

    string toString();
    bool isClosed();

    void setNewConnectionCallback(const TcpConnectionCallback& cb);
    void setMessageCallback(const TcpConnectionCallback& cb);
    void setCloseCallback(const TcpConnectionCallback& cb); 

    void handleNewConnectionCallback();
    void handleMessageCallback();
    void handleCloseCallback();

    void sendInLoop(const string & msg);

private:
    InetAddress getLocalAddr();
    InetAddress getPeerAddr();

    Socket _sock;
    InetAddress _localAddr;
    InetAddress _peerAddr;

    TcpConnectionCallback _onNewConnection;
    TcpConnectionCallback _onMessage;
    TcpConnectionCallback _onClose;

    EventLoop * _loop;

    static const int MAX_VALUE = 1*1024*1024;
};
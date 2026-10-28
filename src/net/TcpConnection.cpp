#include"TcpConnection.h"
#include "EventLoop.h"
#include<sys/socket.h>
#include<sstream>


using std::ostringstream;

TcpConnection::TcpConnection(int fd, EventLoop * loop)
:_sock(fd)
,_localAddr(getLocalAddr())
,_peerAddr(getPeerAddr())
,_loop(loop)
{}

TcpConnection::~TcpConnection()
{}

bool TcpConnection::receive(Message & msg){
    int fd = _sock.getFd();
    if(readn(fd,&msg.tag,sizeof(msg.tag)) != sizeof(msg.tag))
    {
        return false;
    }
    if(readn(fd,&msg.length,sizeof(msg.length)) != sizeof(msg.length))
    {
        return false;
    }

    if(msg.length<0 ||msg.length > MAX_VALUE){
        return false;
    }
    msg.value.resize(msg.length);
    if(msg.length > 0 && readn(fd,msg.value.data(),msg.length) != msg.length)
    {
        return false;
    }
    return true;
}

void TcpConnection::send(const string& msg){
    int fd = _sock.getFd();
    int len = msg.size();
    writen(fd,&len,sizeof(len));
    writen(fd,msg.data(),len);
}

bool TcpConnection::isClosed(){
    char buf[1];
    int ret = ::recv(_sock.getFd(),buf,sizeof(buf),MSG_PEEK);
    return ret == 0;
}

InetAddress TcpConnection::getLocalAddr()
{
    sockaddr_in addr;              // 内核把本端地址填到这里
    socklen_t len = sizeof(addr);  // 传入缓冲区大小
    getsockname(_sock.getFd(), reinterpret_cast<sockaddr *>(&addr), &len);  // 查询本端
    return InetAddress(addr);      // 用已填好的 sockaddr_in 构造
}
InetAddress TcpConnection::getPeerAddr()
{
    sockaddr_in addr;              // 内核把对方地址填到这里
    socklen_t len = sizeof(addr);
    getpeername(_sock.getFd(), reinterpret_cast<sockaddr *>(&addr), &len);  // 查询对端
    return InetAddress(addr);
}

void TcpConnection::sendInLoop(const string & msg)
{
    auto self = shared_from_this();  // 抓住自己。任务还没执行时，map 里即使删了连接，对象也不会先被销毁
    _loop->runInLoop([self, msg]()   // 这个函数将在 IO 线程执行
    {
        self->send(msg);             // send 里的 writen 只在 epoll 线程发生
    });
}

string TcpConnection::toString()
{
    ostringstream oss;  // 在字符串流里拼接
    oss << _localAddr.getIp() << ":" << _localAddr.getPort()
        << " ---- "
        << _peerAddr.getIp() << ":" << _peerAddr.getPort();
    return oss.str();  // 取出拼好的字符串
}
void TcpConnection::setNewConnectionCallback(const TcpConnectionCallback & cb)
{
    _onNewConnection = cb;  // 存下新连接回调
}
void TcpConnection::setMessageCallback(const TcpConnectionCallback & cb)
{
    _onMessage = cb;  // 存下收到消息的回调
}
void TcpConnection::setCloseCallback(const TcpConnectionCallback & cb)
{
    _onClose = cb;  // 存下关闭回调
}
void TcpConnection::handleNewConnectionCallback()
{
    if (_onNewConnection)                      // 没设置回调时不能调用
    {
        _onNewConnection(shared_from_this());  // 把这条连接交给外面的 onNew
    }
}
void TcpConnection::handleMessageCallback()
{
    if (_onMessage)
    {
        _onMessage(shared_from_this());  // 交给外面的 onMessage
    }
}
void TcpConnection::handleCloseCallback()
{
    if (_onClose)
    {
        _onClose(shared_from_this());  // 交给外面的 onClose
    }
}
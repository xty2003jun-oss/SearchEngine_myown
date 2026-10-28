#include "EventLoop.h"  // 类声明
#include <errno.h>      // errno


EventLoop::EventLoop(Acceptor & acceptor)
: _epfd(createEpollFd())  // 先得到 epoll 的 fd
, _evtList(1024)          // 一次最多记录 1024 个就绪事件
, _isLooping(false)       // 还没开始循环
, _acceptor(acceptor)     // 绑定外面的 Acceptor
, _conns()
, _evtfd(createEventFd()) // 创建 eventfd
, _pendings()
, _mutex()
{
    addEpollFd(_acceptor.fd());
    addEpollFd(_evtfd);     // 把 eventfd 也放进 epoll
}

EventLoop::~EventLoop()
{
    ::close(_evtfd);
    ::close(_epfd);  // 关闭 epoll。各连接由 map 里的对象析构时关闭
}

void EventLoop::loop()
{
    _isLooping = true;   // 允许循环
    while (_isLooping)   // unloop 之后，要等当前这次 wait 返回才会停
    {
        waitEpollFd();   // 等事件并处理
    }
}

void EventLoop::unloop()
{
    _isLooping = false;  // 请求停止
}

void EventLoop::setNewConnectionCallback(TcpConnectionCallback cb)
{
    _onNewConnection = cb;  // 存新连接回调
}

void EventLoop::setMessageCallback(TcpConnectionCallback cb)
{
    _onMessage = cb;  // 存消息回调
}
void EventLoop::setCloseCallback(TcpConnectionCallback cb)
{
    _onClose = cb;  // 存关闭回调
}
int EventLoop::createEpollFd()
{
    int epfd = ::epoll_create(1);  // 参数大于 0 即可，不表示只能监听 1 个 fd
    if (epfd < 0)
    {
        perror("epoll_create");    // 创建失败
    }
    return epfd;
}
void EventLoop::addEpollFd(int fd)
{
    epoll_event event;       // 描述要监听的事件
    event.data.fd = fd;      // 就绪后靠这个字段认出 fd
    event.events = EPOLLIN;  // 只关心可读
    ::epoll_ctl(_epfd, EPOLL_CTL_ADD, fd, &event);  // 加入 epoll
}
void EventLoop::delEpollFd(int fd)
{
    epoll_event event;       // 删除时也要带一个 event
    event.data.fd = fd;      // 指定哪个 fd
    event.events = EPOLLIN;  // 和加入时相同
    // fd 还开着才能删。先 close 再删会失败
    ::epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, &event);
}
void EventLoop::waitEpollFd()
{
    // -1 表示没有事件就一直等，不空转
    int ready = ::epoll_wait(_epfd, _evtList.data(), _evtList.size(), -1);
    if (ready < 0)          // 出错，或被信号打断
    {
        perror("epoll_wait");
        return;             // 这次不处理连接
    }
    for (int i = 0; i < ready; ++i)    // ready 是这次就绪的个数
    {
        int fd = _evtList[i].data.fd;   // 这次就绪的 fd
        if (fd == _acceptor.fd())       // 监听套接字
        {
            handleNewConnection();
        }
        else if (fd == _evtfd)          // 工作线程发来的“请发送”通知
        {
            handleRead();               // 读掉通知
            doPendingFunctors();        // 在这个 IO 线程里真正 write
        }
        else                            // 普通连接
        {
            handleMessage(fd);
        }
    }
}
void EventLoop::handleNewConnection()
{
    int connfd = _acceptor.accept();  // 从队列取出一条连接
    if (connfd < 0)                   // 取出失败
    {
        return;
    }
    addEpollFd(connfd);               // 这条连接以后有数据时会被 epoll 报告
    // 必须用 shared_ptr。栈对象不能调用 shared_from_this
    TcpConnectionPtr con(new TcpConnection(connfd,this));
    con->setNewConnectionCallback(_onNewConnection);  // 装上三个回调
    con->setMessageCallback(_onMessage);
    con->setCloseCallback(_onClose);
    _conns[connfd] = con;             // map 保存这条连接，键是 fd
    con->handleNewConnectionCallback();  // 通知 main 里的 onNew
}
void EventLoop::handleMessage(int fd)
{
    auto it = _conns.find(fd);     // 用 fd 找到连接对象
    if (it == _conns.end())        // 表里没有这条连接
    {
        return;
    }
    if (it->second->isClosed())    // 对端关闭。epoll 仍会说这个 fd 可读
    {
        it->second->handleCloseCallback();  // 先通知 onClose
        delEpollFd(fd);                     // 再从 epoll 删除，此时 fd 还没关
        _conns.erase(it);                   // 销毁对象，析构里 close(fd)
    }
    else
    {
        it->second->handleMessageCallback();  // 有数据，交给 onMessage
    }
}

int EventLoop::createEventFd()
{
    int evtfd = ::eventfd(0, 0);  // 初始计数是 0。有人 write 之后它才可读
    if (evtfd < 0)
    {
        perror("eventfd");        // 创建失败
    }
    return evtfd;
}
void EventLoop::handleRead()
{
    uint64_t one = 0;                                      // eventfd 每次读写都是 8 字节
    ::read(_evtfd, &one, sizeof(one));                     // 读掉它，避免 epoll 下次还认为它可读
}
void EventLoop::wakeup()
{
    uint64_t one = 1;                                      // 写入 1，表示有一件事要做
    ::write(_evtfd, &one, sizeof(one));                    // 这一写会让 epoll_wait 返回
}
void EventLoop::runInLoop(Functor cb)
{
    {
        lock_guard<mutex> lg(_mutex);  // 工作线程和 IO 线程不能同时改这个数组
        _pendings.push_back(cb);       // 先把发送函数存起来
    }
    wakeup();                          // 再通知 IO 线程来取
}
void EventLoop::doPendingFunctors()
{
    vector<Functor> tmp;               // 拿到锁之后尽快把任务换出来，不要拿着锁执行发送
    {
        lock_guard<mutex> lg(_mutex);
        tmp.swap(_pendings);           // tmp 得到全部待发送函数，_pendings 变空
    }
    for (auto & cb : tmp)              // 现在是 IO 线程，可以安全 write
    {
        cb();
    }
}
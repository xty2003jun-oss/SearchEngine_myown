#include "InetAddress.h"
#include "Socket.h"
#include "Message.h"

#include <iostream>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::getline;
using std::string;

static const unsigned short PORT = 8888;
static const int MAX_VALUE = 1 * 1024 * 1024;

// 把 len 字节全部写出去。返回实际写了多少。
int writeAll(int fd, const void * buf, int len)
{
    return writen(fd, buf, len);
}

// 读一条响应：4 字节长度 + 正文。失败返回空串，并把 ok 设为 false。
string readReply(int fd, bool & ok)
{
    ok = false;
    int replyLen = 0;                                              // 响应没有 tag，先读长度
    if (readn(fd, &replyLen, sizeof(replyLen)) != sizeof(replyLen))
    {
        return "";
    }
    if (replyLen < 0 || replyLen > MAX_VALUE)                      // 非法长度不能 resize
    {
        cerr << "invalid reply length" << endl;
        return "";
    }
    string reply(replyLen, '\0');                                  // 按长度准备缓冲区
    if (replyLen > 0 && readn(fd, reply.data(), replyLen) != replyLen)
    {
        return "";
    }
    ok = true;
    return reply;
}

int main()
{
    Socket sock;                                                   // 客户端套接字
    int fd = sock.getFd();
    if (fd < 0)
    {
        return 1;
    }

    InetAddress server("127.0.0.1", PORT);                         // 服务端地址
    if (::connect(fd,
                  reinterpret_cast<const sockaddr *>(server.getInetAddressPtr()),
                  sizeof(sockaddr_in)) < 0)
    {
        perror("connect");
        return 1;
    }
    cout << "connected. input 1 or 2, then the query. input 0 to quit." << endl;

    int epfd = ::epoll_create(1);                                  // 同时等待键盘和套接字
    epoll_event ev;
    ev.events = EPOLLIN;                                           // 只关心可读
    ev.data.fd = STDIN_FILENO;                                     // 0，标准输入
    ::epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev);
    ev.data.fd = fd;                                               // 连上的套接字
    ::epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);

    bool waitQuery = false;                                        // false：下一行是 1 或 2；true：下一行是查询内容
    int tag = 1;                                                   // 最近一次选择的类型
    epoll_event ready[2];                                          // 一次最多两个 fd 同时就绪

    while (true)
    {
        if (!waitQuery)
        {
            cout << "1 keyword, 2 page, 0 quit: " << endl;         // 提示选类型
        }
        else
        {
            cout << "query: " << endl;                             // 提示输入查询
        }

        int n = ::epoll_wait(epfd, ready, 2, -1);                  // 一直等到键盘或套接字有数据
        if (n < 0)
        {
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < n; ++i)
        {
            if (ready[i].data.fd == STDIN_FILENO)                  // 键盘有一行
            {
                string line;
                if (!getline(cin, line))                           // 读到文件结束就退出
                {
                    ::close(epfd);
                    return 0;
                }
                if (!waitQuery)                                    // 这一行是菜单
                {
                    if (line == "0")                               // 0 退出
                    {
                        ::close(epfd);
                        return 0;
                    }
                    if (line != "1" && line != "2")                // 只接受 1 和 2
                    {
                        cout << "input 1 or 2" << endl;
                        continue;
                    }
                    tag = line[0] - '0';                           // 字符 '1' 变成整数 1
                    waitQuery = true;                              // 下一行再读查询
                }
                else                                               // 这一行是查询内容
                {
                    Message msg;
                    msg.tag = tag;                                 // 1 关键词，2 网页
                    msg.value = line;
                    msg.length = msg.value.size();                 // 字节数
                    if (writen(fd, &msg.tag, sizeof(msg.tag)) != sizeof(msg.tag) ||
                        writen(fd, &msg.length, sizeof(msg.length)) != sizeof(msg.length) ||
                        writen(fd, msg.value.data(), msg.length) != msg.length)
                    {
                        perror("writen");
                        ::close(epfd);
                        return 1;
                    }
                    waitQuery = false;                             // 发完回到菜单
                }
            }
            else if (ready[i].data.fd == fd)                       // 套接字上有服务端的响应
            {
                bool ok = false;
                string reply = readReply(fd, ok);
                if (!ok)
                {
                    cerr << "server closed" << endl;
                    ::close(epfd);
                    return 1;
                }
                if (reply.empty() || reply == "[]")                // 空数组不要再拆字段
                {
                    cout << "no result" << endl;
                }
                else
                {
                    cout << reply << endl;                         // 关键词是词的数组，网页是对象数组
                }
            }
        }
    }

    ::close(epfd);                                                 // 关掉 epoll。套接字由 sock 析构关闭
    return 0;
}
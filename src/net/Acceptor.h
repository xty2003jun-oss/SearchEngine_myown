#pragma once

#include"Socket.h"
#include"InetAddress.h"

#include<iostream>

using std::cout;
using std::endl;

class Acceptor
{
public:
    Acceptor(const string& ip, unsigned short port);
    ~Acceptor();

    void ready();
    int accept();
    int fd();

private:
    void setReuseAddr();
    void bind();
    void listen();

    Socket _sock;
    InetAddress _addr;
};

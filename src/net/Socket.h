#pragma once

#include<iostream>
#include"NoCopy.h"

using std::cerr;
using std::endl;

class Socket
:public NoCopy
{
public:
    Socket();
    explicit Socket(int fd);
    ~Socket();

    int getFd();

private:
    int _fd;
};
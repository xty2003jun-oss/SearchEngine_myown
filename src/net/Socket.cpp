#include"Socket.h"

#include<sys/socket.h>
#include<unistd.h>

Socket::Socket(){
    _fd =  ::socket(AF_INET,SOCK_STREAM,0);
    if(_fd < 0){
        cerr << "socket error" << endl;
    }
}

Socket::Socket(int fd)
:_fd(fd)
{}

Socket::~Socket(){
    if(_fd >= 0)
    {
        ::close(_fd);
    }
}

int Socket::getFd(){
    return _fd;
}

#include"Acceptor.h"
#include<sys/socket.h>

Acceptor::Acceptor(const string& ip, unsigned short port)
: _sock()
, _addr(ip,port)
{}

Acceptor::~Acceptor()
{}

void Acceptor::ready(){
    setReuseAddr();
    bind();
    listen();
    cout<<"listen..."<<endl;
}

int Acceptor::fd(){
    return _sock.getFd();
}

void Acceptor::setReuseAddr(){
    int opt = 1;
    setsockopt(_sock.getFd(), SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
}

void Acceptor::bind(){
    ::bind(_sock.getFd(),
           reinterpret_cast<const struct sockaddr*> (_addr.getInetAddressPtr()),    
           sizeof(sockaddr_in));
}

void Acceptor::listen(){
    ::listen(_sock.getFd(),128);
}

int Acceptor::accept(){
    int connfd = ::accept(_sock.getFd(),nullptr,nullptr);
    if(connfd < 0){
        perror("accept");
    }
    return connfd;
}



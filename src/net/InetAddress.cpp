#include"InetAddress.h"
#include<string.h>

InetAddress::InetAddress(const string& ip, unsigned short port){
    memset(&_addr,0,sizeof(_addr));
    _addr.sin_family = AF_INET;
    _addr.sin_addr.s_addr = inet_addr(ip.c_str());
    _addr.sin_port = htons(port);
}

InetAddress::InetAddress(const struct sockaddr_in& addr)
:_addr(addr)
{}

InetAddress::~InetAddress(){}

string InetAddress::getIp(){
    char ipv4_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET,(void*)&_addr.sin_addr,ipv4_str,INET_ADDRSTRLEN);
    return string(ipv4_str);
}

unsigned short InetAddress::getPort(){
    return ntohs(_addr.sin_port);
}

const struct sockaddr_in* InetAddress::getInetAddressPtr(){
    return &_addr;
}


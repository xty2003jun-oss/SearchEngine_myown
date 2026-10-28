#pragma once

#include<arpa/inet.h>
#include<string>

using namespace std;

class InetAddress
{
public:
    InetAddress(const string& ip, unsigned short port);
    InetAddress(const struct sockaddr_in& addr);

    ~InetAddress();

    string getIp();
    unsigned short getPort();

    const struct sockaddr_in* getInetAddressPtr();

private:
    struct sockaddr_in _addr;
};
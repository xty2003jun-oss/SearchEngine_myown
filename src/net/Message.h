#pragma once

#include<string>

using std::string;

struct Message
{
    int tag;
    int length;
    string value;
};

int readn(int fd,void *buf,int len);

int writen(int fd,const void* buf, int len);
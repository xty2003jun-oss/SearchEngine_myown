#include "Message.h"
#include<errno.h>
#include<iostream>
#include<unistd.h>

using namespace std;

int readn(int fd,void *buf, int len){
    int left = len;
    char* p = static_cast<char*>(buf);
    while(left > 0){
        int ret = ::read(fd,p,left);
        if(ret == -1 && errno == EINTR){
            continue;
        }
        else if(ret == -1){
            cerr << "readn failed "<<endl;
            return -1;
        }
        else if(ret == 0){
            break;
        }
        else{
            p +=ret;
            left -=ret;
        }
    }
    return len - left;
}

int writen(int fd, const void* buf, int len){
    int left = len;
    const char* p = static_cast<const char*>(buf);
    while(left > 0){
        int ret = ::write(fd,p,left);
        if(ret == -1 && errno == EINTR){
            continue;
        }
        else if(ret == -1){
            cerr << "writen failed "<<endl;
            return -1;
        }
        else if(ret == 0){
            break;
        }
        else{
            p +=ret;
            left -=ret;
        }
    }
    return len - left;
}
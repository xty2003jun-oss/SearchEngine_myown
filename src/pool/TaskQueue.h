#pragma once

#include<queue>
#include<mutex>
#include<condition_variable>
#include<functional>

using std::queue;
using std::mutex;
using std::condition_variable;
using std::function;
using std::unique_lock;

using ElemType = function<void()>;

class TaskQueue
{
public:
    TaskQueue(size_t capa);
    ~TaskQueue();

    void push(ElemType val);
    ElemType pop();
    bool full();
    bool empty();
    void wakeup();
private:
    size_t _capacity;
    queue<ElemType> _que;
    mutex _mtx;
    condition_variable _notEmpty;
    condition_variable _notFull;
    bool _flag;
};


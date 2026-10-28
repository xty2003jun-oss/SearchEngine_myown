#pragma once
#include "TaskQueue.h"  // 任务队列
#include <thread>       // std::thread
#include <vector>       // 保存工作线程
#include <iostream>
using std::thread;
using std::vector;
using std::cout;
using std::endl;
class ThreadPool
{
public:
    ThreadPool(size_t threadNum, size_t queSize);  // 线程个数和队列容量
    ~ThreadPool();
    void start();                 // 创建工作线程
    void stop();                  // 等任务做完，再回收线程
    void addTask(ElemType pt);    // IO 线程把任务放进来
private:
    ElemType getTask();  // 工作线程从队列取任务
    void doTask();       // 每个工作线程的入口
    size_t _threadNum;       // 工作线程数量
    vector<thread> _threads; // 这些线程对象
    size_t _queSize;         // 队列容量
    TaskQueue _taskQueue;    // 任务队列
    bool _isExit;            // true 表示线程池正在退出
};
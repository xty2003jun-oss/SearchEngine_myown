#include "ThreadPool.h"
#include <unistd.h>  // stop 里短暂 sleep
ThreadPool::ThreadPool(size_t threadNum, size_t queSize)
: _threadNum(threadNum)   // 记下要开几个线程
, _threads()              // 线程容器先空着，start 时才创建
, _queSize(queSize)       // 队列容量
, _taskQueue(_queSize)    // 用这个容量构造队列
, _isExit(false)          // 还没退出
{}
ThreadPool::~ThreadPool()
{}
void ThreadPool::start()
{
    for (size_t i = 0; i < _threadNum; ++i)                 // 创建 _threadNum 个线程
    {
        _threads.push_back(thread(&ThreadPool::doTask, this));  // 每个线程都跑 doTask，this 是线程池自己
    }
}
void ThreadPool::stop()
{
    while (!_taskQueue.empty())  // 队列里还有任务时先别关
    {
        sleep(1);                // 让工作线程把剩下的任务做完
    }
    _isExit = true;              // 之后 doTask 的循环可以结束
    _taskQueue.wakeup();         // 叫醒正堵在空队列上的线程
    for (auto & th : _threads)   // 等每一个工作线程返回
    {
        th.join();
    }
}
void ThreadPool::addTask(ElemType pt)
{
    if (pt)                      // 空函数不放进队列
    {
        _taskQueue.push(pt);     // 放进队列。队列满时这里会等待
    }
}
ElemType ThreadPool::getTask()
{
    return _taskQueue.pop();     // 没任务时会在 pop 里等待
}
void ThreadPool::doTask()
{
    while (!_isExit)             // 线程池没退出就一直取任务
    {
        ElemType task = getTask();  // 阻塞，直到有任务或被 wakeup
        if (task)                // 取到的是真正的任务
        {
            task();              // 在这个工作线程里执行
        }
    }
}
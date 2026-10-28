#include"TaskQueue.h"

TaskQueue::TaskQueue(size_t capa)
:_capacity(capa)
,_flag(true)
{}

TaskQueue::~TaskQueue()
{}

void TaskQueue::push(ElemType val)
{
    unique_lock<mutex> ul(_mtx);  // 加锁。wait 时会暂时放开，醒来再拿回
    while (full())                 // 用 while，防止被虚假唤醒后队列其实还是满的
    {
        _notFull.wait(ul);         // 满了就睡，直到有人取走任务并 notify
    }
    _que.push(val);                // 队列有空位，放入任务
    _notEmpty.notify_one();        // 叫醒一个正在等任务的工作线程
}
ElemType TaskQueue::pop()
{
    unique_lock<mutex> ul(_mtx);       // 取任务也要拿同一把锁
    while (empty() && _flag)           // 空着且线程池还没退出，才继续等
    {
        _notEmpty.wait(ul);            // 没任务就睡
    }
    if (_flag)                         // 醒来后线程池仍在工作，说明队列里有任务
    {
        ElemType temp = _que.front();  // 取出队头
        _que.pop();                    // 从队列删掉
        _notFull.notify_one();         // 叫醒一个可能在等空位的生产者
        return temp;                   // 把任务交给工作线程
    }
    return nullptr;                    // 线程池要退出，返回空函数
}
bool TaskQueue::full()
{
    return _que.size() == _capacity;  // 个数等于容量就是满
}
bool TaskQueue::empty()
{
    return _que.size() == 0;          // 个数为 0 就是空
}
void TaskQueue::wakeup()
{
    {
        unique_lock<mutex> ul(_mtx);  // 改标志也要在锁里，避免和 pop 同时读
        _flag = false;                 // 告诉 pop：不要再为了空队列睡觉
    }
    _notEmpty.notify_all();            // 叫醒所有卡在 pop 里的工作线程
}
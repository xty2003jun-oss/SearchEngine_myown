#pragma once

#include "LruCache.h"

#include <mutex>

template <typename Key, typename Value>
class LruKCache
{
public:
    LruKCache(int capacity, int historyCapacity, size_t k)
    : _k(k)                              // 历史队列里看到几次才进主缓存
    , _history(historyCapacity)          // 第一次出现放这里
    , _main(capacity)                    // 确认会再用的放这里
    {}

    bool get(const Key & key, Value & value)
    {
        std::lock_guard<std::mutex> lock(_mutex);  // 多个工作线程不能同时改链表

        if (_main.get(key, value))       // 主缓存里有，直接用，不再计算
        {
            return true;
        }
        if (_history.get(key, value))    // 历史队列里有，说明这是第二次及以后
        {
            _history.incrementAccessCount(key);
            if (_history.getAccessCount(key) >= _k)  // 次数够了，升级到主缓存
            {
                _history.remove(key);
                _main.put(key, value);
            }
            return true;                 // 历史里已经有结果，这次也不必重算
        }
        return false;                    // 两级都没有
    }

    void put(const Key & key, const Value & value)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        Value old;
        if (_main.get(key, old))         // 已经在主缓存，只更新
        {
            _main.put(key, value);
            return;
        }
        if (_history.get(key, old))      // 已经在历史队列，再记一次
        {
            _history.incrementAccessCount(key);
            if (_history.getAccessCount(key) >= _k)
            {
                _history.remove(key);
                _main.put(key, value);
            }
            return;
        }
        _history.put(key, value);        // 第一次见到，只放历史队列
        if (_history.getAccessCount(key) >= _k)  // k 为 1 时第一次就会升级；k 为 2 时这里不会进
        {
            _history.remove(key);
            _main.put(key, value);
        }
    }

private:
    size_t _k;
    LruCache<Key, Value> _history;
    LruCache<Key, Value> _main;
    std::mutex _mutex;
};
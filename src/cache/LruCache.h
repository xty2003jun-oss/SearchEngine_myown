#pragma once

#include <list>             // 按最近使用顺序排，表头最新，表尾最旧
#include <unordered_map>    // 用键直接找到链表节点
#include <utility>

using std::list;
using std::unordered_map;

template <typename Key, typename Value>
class LruCache
{
    struct Node
    {
        Key key;
        Value value;
        size_t count;   // 在历史队列里被看到的次数
        Node(const Key & k, const Value & v)
        : key(k)
        , value(v)
        , count(1)      // 放进来时算访问过 1 次
        {}
    };

    using List = list<Node>;
    using Iter = typename List::iterator;

public:
    explicit LruCache(int capacity)
    : _capacity(capacity)   // 最多留多少条
    {}

    bool get(const Key & key, Value & value)
    {
        auto it = _map.find(key);     // 先查有没有这个键
        if (it == _map.end())
        {
            return false;             // 没有
        }
        _list.splice(_list.begin(), _list, it->second);  // 把这个节点挪到表头，表示刚用过
        value = it->second->value;    // 把存着的结果交出去
        return true;
    }

    void put(const Key & key, const Value & value)
    {
        auto it = _map.find(key);
        if (it != _map.end())                     // 已经在里面，只更新值并挪到表头
        {
            it->second->value = value;
            _list.splice(_list.begin(), _list, it->second);
            return;
        }
        if (_capacity > 0 && static_cast<int>(_list.size()) >= _capacity)  // 满了就丢掉最久没用的表尾
        {
            _map.erase(_list.back().key);
            _list.pop_back();
        }
        _list.emplace_front(key, value);          // 新数据放表头
        _map[key] = _list.begin();                // 记下它在链表里的位置
    }

    void incrementAccessCount(const Key & key)
    {
        auto it = _map.find(key);
        if (it != _map.end())
        {
            it->second->count++;                  // 历史队列里再被看到一次
        }
    }

    size_t getAccessCount(const Key & key)
    {
        auto it = _map.find(key);
        if (it == _map.end())
        {
            return 0;
        }
        return it->second->count;
    }

    void remove(const Key & key)
    {
        auto it = _map.find(key);
        if (it == _map.end())
        {
            return;
        }
        _list.erase(it->second);                  // 从链表删掉
        _map.erase(it);                           // 从映射删掉
    }

private:
    int _capacity;
    List _list;                                   // 使用顺序
    unordered_map<Key, Iter> _map;                // 键到链表节点
};
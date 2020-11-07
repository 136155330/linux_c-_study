#ifndef __BLOCKING_QUEUE__
#define __BLOCKING_QUEUE__
#include <iostream>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <assert.h>

template <typename T>
class blocking_queue{
public:
    typedef std::lock_guard<std::mutex> MutexLockGuard;
    blocking_queue():_mutex(), _notEmpty(), _queue(){}
    blocking_queue(const blocking_queue &) = delete;
    blocking_queue & operator = (const blocking_queue &) = delete;
    void put(const T& x){
        {
            MutexLockGuard lock(_mutex);
            _queue.push_back(x);
        }
        _notEmpty.notify_one();
    };
    void put(T && x){
        {
            MutexLockGuard lock(_mutex);
            _queue.push_back(std::move(x));
        }
        _notEmpty.notify_one();
    };
    T take(){
        std::unique_lock<std::mutex> lock(_mutex);
        _notEmpty.wait(lock, [this]{  return !this->_queue.empty(); });  //第二个参数为false的情况下 才进行阻塞
        assert(!_queue.empty());
        T front(std::move(_queue.front()));
        _queue.pop_front();
        return  front;
    };
    size_t size() const{
        MutexLockGuard lock(_mutex);
        return _queue.size();
    };
private:
    mutable std::mutex _mutex;
    std::condition_variable _notEmpty;
    std::deque<T> _queue;
};



#endif
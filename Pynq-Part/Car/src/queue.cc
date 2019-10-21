/*
 * @Author: Sauron Wu
 * @GitHub: wutianze
 * @Email: 1369130123qq@gmail.com
 * @Date: 2019-10-21 16:30:06
 * @LastEditors: Sauron Wu
 * @LastEditTime: 2019-10-21 16:30:06
 * @Description: 
 */
#include <iostream>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <memory>
#include <algorithm>
#include <vector>
#include <string>
#include <queue>
using namespace std;

template<typename T>
class safe_queue
{
public:
    mutex m_mut;
    condition_variable m_cond;
    queue<T> m_queue;  
public:
    safe_queue(){}
    safe_queue(const safe_queue& rhs)
    {
        lock_guard<mutex> lk(rhs.m_mut);
        m_queue = rhs;
    }
    void push(T data)
    {
        lock_guard<mutex> lk(m_mut);  
        m_queue.push(move(data));  
        m_cond.notify_one(); 
    }
    void wait_and_pop(T &res)  
    {
        unique_lock<mutex> lk(m_mut);
        m_cond.wait(lk,[this]{return !m_queue.empty();});
        res = move(m_queue.front());
        m_queue.pop();
    }

    bool try_pop(T &res)  //return immediately
    {
        lock_guard<mutex> lk(m_mut);
        if(m_queue.empty())
            return false;
        res = move(m_queue.front());
        return true;
    }

    shared_ptr<T> wait_and_pop()
    {
        unique_lock<mutex> lk(m_mut);
        m_cond.wait(lk,[this]{return !m_queue.empty();});
        shared_ptr<T> res(make_shared<T>(move(m_queue.front())));
        m_queue.pop();
        return res;
    }

    shared_ptr<T> try_pop()
    {
        lock_guard<mutex> lk(m_mut);
        if(m_queue.empty())
            return NULL;
        shared_ptr<T> res(make_shared<T>(move(m_queue.front())));
        m_queue.pop();
        return res;
    }
 };
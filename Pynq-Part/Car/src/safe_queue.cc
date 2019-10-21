/*
 * @Author: Sauron Wu
 * @GitHub: wutianze
 * @Email: 1369130123qq@gmail.com
 * @Date: 2019-10-21 16:30:06
 * @LastEditors: Sauron Wu
 * @LastEditTime: 2019-10-21 17:34:42
 * @Description: 
 */
#include "safe_queue.h"

template<typename T>
safe_queue::safe_queue(){};
safe_queue::safe_queue(const safe_queue& rhs)
{
    lock_guard<mutex> lk(rhs.m_mut);
    m_queue = rhs;
};
int safe_queue::size(){
    lock_guard<mutex> lk(m_mut);
    return m_queue.size();
}
void safe_queue::push(T data)
{
    lock_guard<mutex> lk(m_mut);  
    m_queue.push(move(data));  
    m_cond.notify_one(); 
};
void safe_queue::wait_and_pop(T &res)  
{
    unique_lock<mutex> lk(m_mut);
    m_cond.wait(lk,[this]{return !m_queue.empty();});
    res = move(m_queue.front());
    m_queue.pop();
};

bool safe_queue::try_pop(T &res)  //return immediately
{
    lock_guard<mutex> lk(m_mut);
    if(m_queue.empty())
        return false;
    res = move(m_queue.front());
    return true;
};

shared_ptr<T> safe_queue::wait_and_pop()
{
    unique_lock<mutex> lk(m_mut);
    m_cond.wait(lk,[this]{return !m_queue.empty();});
    shared_ptr<T> res(make_shared<T>(move(m_queue.front())));
    m_queue.pop();
    return res;
};

shared_ptr<T> safe_queue::try_pop()
{
    lock_guard<mutex> lk(m_mut);
    if(m_queue.empty())
        return NULL;
    shared_ptr<T> res(make_shared<T>(move(m_queue.front())));
    m_queue.pop();
    return res;
};
    

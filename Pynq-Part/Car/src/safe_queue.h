/*
 * @Author: Sauron Wu
 * @GitHub: wutianze
 * @Email: 1369130123qq@gmail.com
 * @Date: 2019-10-21 16:51:39
 * @LastEditors: Sauron Wu
 * @LastEditTime: 2019-10-21 16:51:39
 * @Description: 
 */
#include <thread>
#include <condition_variable>
#include <mutex>
#include <memory>
#include <queue>
using namespace std;
template<typename T>
class safe_queue
{
public:
    mutex m_mut;
    condition_variable m_cond;
    queue<T> m_queue;  
    safe_queue();
    safe_queue(const safe_queue& rhs);

    int size();
    void push(T data);
    void wait_and_pop(T &res);
    bool try_pop(T &res);

    shared_ptr<T> wait_and_pop();

    shared_ptr<T> try_pop();
 };
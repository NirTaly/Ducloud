/*******************************************************************************
 * File name:   serializer.h
 * Developer:   HRD07	
 * Version:		0 
 * Date:        2019-04-08 13:09:47
 * Description: 
 ******************************************************************************/

#ifndef __WPQ_H__
#define __WPQ_H__

#include <queue>                /* priority_queue                            */
#include <vector>               /* vector                                    */
#include <functional>           /* less                                      */
#include <mutex>                /* mutex                                     */
#include <condition_variable>   /* condition_variable                        */
#include <chrono>               /* chrono                                    */

namespace ilrd
{

template<
    class T,
    class Container = std::vector<T>,
    class Compare = std::less<typename Container::value_type>
>class WPQ final
{
public:
    WPQ() = default;

    /* WARNING: destructing the WPQ when some thread is blocked in Pop() yields
    * undefined behavior
    */
    ~WPQ() = default;

    /* Thread-safe functions:
    * Pop(), Push(), IsEmpty(), Size()
    * Each of these calls tries to establish an exclusive lock on the WPQ
    */

    /* O(log(N)) */
    /* If empty, blocks until an element is inserted (on concurrent runs, only
     * one waiting thread returns)
     * Returns a copy of the value
     */
    void Pop(T& out_val);

    template< class Rep, class Period >
    bool Pop(T& out_val, std::chrono::duration<Rep, Period> const& timeout);

    /* O(log(N)) */
    /* Copies the value */
    void Push(T const& val);

    bool IsEmpty() const;
    size_t Size() const;

    WPQ(WPQ<T, Container, Compare> const& other) = delete;
    WPQ<T, Container, Compare>&
        operator=(WPQ<T, Container, Compare> const& other) = delete;
private:
    std::priority_queue<T, Container, Compare> m_pq;
    std::condition_variable m_cond_var;
    mutable std::mutex m_mutex;
};

/******************************************************************************/
/******************************************************************************/

template <typename T, typename Container, typename Compare>
void WPQ<T, Container, Compare>::Push(const T& value)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    m_pq.push(value);

    m_cond_var.notify_one();
}
/******************************************************************************/

template <typename T, typename Container, typename Compare>
void WPQ<T, Container, Compare>::Pop(T& out_val)
{
    std::unique_lock<std::mutex> lk(m_mutex);
    m_cond_var.wait(lk, [this](){ return (!m_pq.empty()); });
    
    out_val = m_pq.top();
    m_pq.pop();
}
/******************************************************************************/

template <typename T, typename Container, typename Compare>
template< class Rep, class Period >
bool WPQ<T, Container, Compare>::Pop(T& out_val, 
                            std::chrono::duration<Rep, Period> const& timeout)
{
    std::unique_lock<std::mutex> lk(m_mutex);
    bool time_up = m_cond_var.wait_for(lk, timeout, [this](){ return (!m_pq.empty()); });
    if (time_up == false)
    {
        return false;
    }

    out_val = m_pq.top();
    m_pq.pop();

    return true;
}
/******************************************************************************/

template <typename T, typename Container, typename Compare>
bool WPQ<T, Container, Compare>::IsEmpty() const
{
    std::unique_lock<std::mutex> lock(m_mutex);

    return m_pq.empty();
}
/******************************************************************************/

template <typename T, typename Container, typename Compare>
size_t WPQ<T, Container, Compare>::Size() const
{
    std::unique_lock<std::mutex> lock(m_mutex);

    return m_pq.size();;
}

}

#endif     /* __WPQ_H__ */
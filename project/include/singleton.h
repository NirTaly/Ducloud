/*****************************************************************************
 * File name:   driver_proxy.h
 * Developer:   
 * Version: 	4
 * Date:        2019-03-27 10:26:28
 * Description: 
 *****************************************************************************/

#ifndef __SINGLETON_H__
#define __SINGLETON_H__

#include <atomic>

#include "../../cpp/lock.h"

namespace ilrd
{

template <typename T>
class Singleton
{
public:
    static T* GetInstance();
    
    Singleton(const Singleton&) = delete;
    Singleton operator=(const Singleton&) = delete;
private:
    Singleton();
    ~Singleton();

    // allow singleton to be deleted at end
    class MemGuard 
    {
    public: 
        ~MemGuard() 
        {
          delete m_instance;
          m_instance = nullptr;
        }
    };

    static std::atomic<T*> m_instance;
    static std::mutex m_mutex;
};

template <typename T>
T* Singleton<T>::GetInstance()
{
    static MemGuard g;
    
    T* tmp = m_instance.load(std::memory_order_acquire);
    
    if (!tmp)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        tmp = m_instance.load(std::memory_order_relaxed);
        if (!tmp)
        {
            tmp = new T;

            m_instance.store(tmp, std::memory_order_release);
        }
    }

    return tmp;
}

template <typename T> std::atomic<T*> Singleton<T>::m_instance(nullptr);
template <typename T> std::mutex Singleton<T>::m_mutex;

}
#endif     /* __SINGLETON_H__ */

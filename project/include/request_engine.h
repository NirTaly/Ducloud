/*****************************************************************************
 * File name:   request_engine.h
 * Developer:   HRD7
 * Version:     1
 * Date:        2019-04-28 15:31:24
 * Description: Request Engine Framework header
 *****************************************************************************/

#ifndef __REQUEST_ENGINE_H__
#define __REQUEST_ENGINE_H__

#include <sys/epoll.h>
#include <vector>           // std::vector

#include "utils.h"
#include "thread_pool.h"
#include "factory.h"
#include "plug_and_play.h"
#include "handleton.h"
#include "logger.h"


namespace ilrd
{
class PlugNPlay;
const size_t storage_size(200*MB);

template<typename Key, typename Params>
class RequestEngine
{
public:
    class Gateway;
    class Task;
    using CreatorFunc = typename Factory<RequestEngine::Task, Key, Params>::FactoryFunc;
    
    // path_name to DirMonitor to allow Plug&Play
    RequestEngine(const std::string& path_name, size_t num_of_threads = 4);
    ~RequestEngine() = default;

    // Add Task to handle.
    // If is already in, it is override it
    bool ConfigTask(const Key& key, CreatorFunc creator);

    void AddGateway(Gateway& gateway);
    void Run();
    void Stop();

    // Un-copyable
    RequestEngine(const RequestEngine&) = delete;
    RequestEngine& operator=(const RequestEngine&) = delete;

private:
    static std::shared_ptr<PlugNPlay> m_pnp;
    std::vector<Gateway*> m_gateways; // Storing FDs
    ThreadPool m_thread_pool;
    Handleton<Logger> m_logger;
    Handleton< Factory<RequestEngine::Task,Key,Params>> m_factory;
    FD m_epoll_fd;
    std::vector<struct epoll_event> m_events;

    std::atomic_bool m_to_disconnect;
};

template<typename Key, typename Params>
std::shared_ptr<PlugNPlay> RequestEngine<Key,Params>::m_pnp(nullptr);
/*****************************************************************************/

template<typename Key, typename Params>
class RequestEngine<Key, Params>::Task : public ilrd::Task
{
public:
    using TaskPriority = Task::Priority;  
        
    // Possible values TaskPriority: VERY_HIGH, HIGH, MEDIUM, LOW, VERY_LOW
    explicit Task(TaskPriority priority = TaskPriority::VERY_LOW);
    
private:
    virtual void Execute() = 0;
};
/*****************************************************************************/

// Class Responsibility to close fd
template<typename Key, typename Params>
class RequestEngine<Key, Params>::Gateway
{
public:
    explicit Gateway(int fd);
    virtual ~Gateway() = default;
    
    int GetFD() /* const */;  
    struct epoll_event* GetEvent() { return &m_epoll_event; }  

    virtual std::pair<Key, Params> Read() = 0;

private:
    FD m_fd;
    struct epoll_event m_epoll_event;
};
/*****************************************************************************/

// needs to initialize first in program, and be static
class PlugNPlay
{
public:
    PlugNPlay(const std::string& path_name);
    ~PlugNPlay() = default;

    // Un-copyable
    PlugNPlay(const PlugNPlay&) = delete;
    PlugNPlay& operator=(const PlugNPlay&) = delete;

private:
    DirMonitor m_monitor;
    DllLoader m_loader;
};


/*===========================================================================*/
/*===========================================================================*/

template<typename Key, typename Params>
RequestEngine<Key, Params>::RequestEngine(const std::string& path_name, 
        size_t num_of_threads): m_thread_pool(num_of_threads) ,m_events(2)
{ 
    try
    {
        m_pnp = std::make_shared<PlugNPlay>(path_name);
    }
    catch(const std::bad_alloc& e)
    {
        m_logger->Write(e.what());
        throw;
    }

    m_epoll_fd = std::move( FD(epoll_create1(0)));
    if(m_epoll_fd == -1)
    {
        m_logger->Write("SetEpoll(): Failed to create epoll file descriptor");        
        throw std::runtime_error("SetEpoll(): Failed to create epoll file descriptor");
    }
}

/*****************************************************************************/

template<typename Key, typename Params>
bool RequestEngine<Key, Params>::ConfigTask(const Key& key, CreatorFunc creator)
{
    return m_factory->Add(key, creator);
}

/*****************************************************************************/
template<typename Key, typename Params>
void RequestEngine<Key, Params>::AddGateway(Gateway& gateway)
{
    m_gateways.push_back(&gateway);

    epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, gateway.GetFD() , gateway.GetEvent());
}

/*****************************************************************************/
template<typename Key, typename Params>
void RequestEngine<Key, Params>::Run()
{
    while (m_to_disconnect == false)
    {            
        int event_count = epoll_wait(m_epoll_fd, m_events.data(), m_events.size(), TIMEOUT);
        if(m_to_disconnect == true)
        {
            break;
        }

        for (int j = 0; j < event_count; j++)
        {
            for (size_t i = 0; i < m_gateways.size(); i++)
            {
                if (m_gateways[i]->GetFD() == m_events[j].data.fd)
                {
                    std::pair<Key,Params> retval = m_gateways[i]->Read();
                    m_thread_pool.AddTask(m_factory->CreateShared(retval.first, retval.second));
                }
            }
        }
    }
}

/*****************************************************************************/
template<typename Key, typename Params>
inline void RequestEngine<Key, Params>::Stop()
{
    m_to_disconnect = true;
}

/*****************************************************************************/
/*****************************************************************************/

template<typename Key, typename Params>
RequestEngine<Key, Params>::Gateway::Gateway(int fd): m_fd(fd) 
{ 
    bzero(static_cast<void*>(&m_epoll_event), sizeof(struct epoll_event));   
    
    m_epoll_event.events = EPOLLIN;
    m_epoll_event.data.fd = fd; 
}

/*****************************************************************************/
template<typename Key, typename Params>
int RequestEngine<Key, Params>::Gateway::GetFD() /* const */
{
    return m_fd;
}

/*****************************************************************************/

/*****************************************************************************/
/*****************************************************************************/
template<typename Key, typename Params>
RequestEngine<Key, Params>::Task::Task(TaskPriority priority)
    : ilrd::Task(priority) { }
/*****************************************************************************/
/*****************************************************************************/
PlugNPlay::PlugNPlay(const std::string& path_name)
    : m_monitor(path_name), m_loader(m_monitor.GetPublisher()) 
{
    
}
/*****************************************************************************/
/*****************************************************************************/


} //ilrd
 
#endif     /* __REQUEST_ENGINE_H__ */


/*******************************************************************************
******************************** END OF FILE ***********************************
*******************************************************************************/


/*****************************************************************************
 * File name:   plug_and_play.cpp
 * Developer:   HRD07
 * Version: 	0
 * Date:        2019-04-22 09:54:54
 * Description: 
 *****************************************************************************/

#include <sys/inotify.h>
#include <sys/types.h>
#include <dlfcn.h>

#include "plug_and_play.h"
#include "logger.h"
/*****************************************************************************/

namespace ilrd
{
#define MAX_EVENTS 1024                                         /*Max. number of events to process at one go*/
#define LEN_NAME 80                                             /*Assuming that the length of the filename won't exceed 16 bytes*/
#define EVENT_SIZE  ( sizeof (struct inotify_event) )           /*size of one event*/
#define BUF_LEN     ( MAX_EVENTS * ( EVENT_SIZE + LEN_NAME ))   /*buffer to store the data of events*/

// Handleton<Logger> log;
Logger* log = Singleton<Logger>::GetInstance();
/*****************************************************************************/
DirMonitor::DirMonitor(const std::string& path_name)
    : m_path_name(path_name), m_is_monitoring(true)
{
    /* Initialize Inotify*/
    m_fd_inotify = std::move(FD(inotify_init()));
    if ( m_fd_inotify < 0 ) 
    {
        log->Write("Couldn't initialize inotify");
        throw(std::runtime_error("DirMonitor(): inotify_init failed"));
    }
    
    /* add watch to starting directory */
    m_wd = inotify_add_watch(m_fd_inotify, m_path_name.c_str(),
                                             IN_CREATE | IN_MODIFY | IN_DELETE); 
    if (m_wd == -1)
    {
        log->Write("DirMonitor(): Couldn't add watch\n");
        throw(std::runtime_error("DirMonitor(): Couldn't add watch"));
    }
    
    m_thread = std::thread(&DirMonitor::INotifyFunc, this);
}
/*****************************************************************************/

void DirMonitor::INotifyFunc()
{
    char buffer[BUF_LEN];
    int i = 0, length;

    while(m_is_monitoring)
    {
        i = 0;
        length = read( m_fd_inotify, buffer, BUF_LEN );  
        if (!m_is_monitoring)
        {
            std::cout << "Exit Thread" << std::endl;
            break;
        }

        if ( length < 0 ) 
        {
            log->Write("INotifyFunc(): read");
            throw(std::runtime_error("INotifyFunc(): read"));
        }  
    
        while ( i < length ) 
        {
            struct inotify_event *event = (struct inotify_event*) &buffer[i];

            if ( event->len ) 
            {
                if ((event->mask & IN_CREATE && !(event->mask & IN_ISDIR)) /* ||
                    event->mask & IN_MODIFY */)
                {
                    printf("New event: %s\n", event->name);
                    fflush(stdin);
                    m_pub.Publish(m_path_name + std::string((char*)&event->name));
                } 

                i += EVENT_SIZE + event->len;
            }
        }
    } 
}
/*****************************************************************************/

DirMonitor::~DirMonitor()
{
    m_is_monitoring = false;
    /* Clean up wd*/
    inotify_rm_watch( m_fd_inotify, m_wd );
    
    std::cout << "Before Join" << std::endl;
    m_thread.join();
    std::cout << "After Join" << std::endl;
}
/*****************************************************************************/

Publisher<std::string> *DirMonitor::GetPublisher() { return &m_pub; }
/*****************************************************************************/
/*****************************************************************************/

DllLoader::DllLoader(Publisher<std::string>* publisher)
    : m_sub(publisher, &DllLoader::Load, &DllLoader::Stop, *this) { }

/*****************************************************************************/

DllLoader::~DllLoader()
{ 
    Stop();
}
/*****************************************************************************/

void DllLoader::Load(const std::string& file_name)
{
    std::cout << file_name.c_str() << std::endl;

    void* handle = dlopen(file_name.c_str(), RTLD_LAZY);
    if (!handle)
    {
        throw(std::runtime_error(dlerror()));        
        log->Write("Load(): dlopen - %s");
        return;
    }

    std::cout << "handle open: " << handle << std::endl;
    m_handle_vector.push_back(handle);
}

void DllLoader::Stop()
{
    while (!m_handle_vector.empty())
    {
        std::cout << "handle close: " << m_handle_vector.back() << std::endl;
        dlclose(m_handle_vector.back());
        m_handle_vector.pop_back();
    }
}

} // namespace ilrd

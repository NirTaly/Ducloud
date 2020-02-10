/*****************************************************************************
 * File name:   plug_and_play.h
 * Developer:   HRD07
 * Version: 	0
 * Date:        2019-04-22 09:54:54
 * Description: 
 *****************************************************************************/

#ifndef __PLUG_AND_PLAY_H__
#define __PLUG_AND_PLAY_H__

#include <cstddef>      /* size_t */
#include <thread>
#include <unistd.h>

#include "utils.h"
#include "publisher_subscriber.h"

namespace ilrd
{

    class DirMonitor
    {
    public:
        // DirMonitor recives the monitored file pathname. 
        // exception is thrown if ctor failed(run_time error)
        // path_name : "/path/to/monitored_dir/"
        explicit DirMonitor(const std::string& path_name);
        ~DirMonitor();
        
        Publisher<std::string> *GetPublisher();
        
        //UnCopyable
        DirMonitor(const DirMonitor& other)        = delete;
        DirMonitor& operator=(const DirMonitor&)   = delete;
        
        //UnMoveable
        DirMonitor(DirMonitor&& other)             = delete;
        DirMonitor& operator=(DirMonitor&&)        = delete;

    private:    
        // class FD
        // {
        // public:
        //     explicit FD(int fd = -1) : m_fd(fd) { }
        //     ~FD() { if (m_fd != -1) close(m_fd); }

        //     int GetFD() const { return m_fd; }

        //     FD& operator=(FD&& other) noexcept
        //     {
        //         if (m_fd != -1)
        //         {
        //             close(m_fd);
        //         }


        //         m_fd = other.m_fd;
                
        //         other.m_fd = -1;
                
        //         return *this;
        //     }
        //     operator int() { return m_fd; }

        //     FD(const FD&) = delete;
        //     FD& operator=(const FD&) = delete;
        // private:
        //     int m_fd;
        // };

        Publisher<std::string> m_pub;
        std::string            m_path_name;
        FD                     m_fd_inotify;
        int                    m_wd;
        std::thread            m_thread;
        bool                   m_is_monitoring;

        void INotifyFunc();
    };

    /**
     *  Allow ONLY adding new files - NOT modify/deleting
     */
    class DllLoader
    {
    public:    
        explicit DllLoader(Publisher<std::string>* publisher);
        ~DllLoader();

        void Load(const std::string& file_name);
        void Stop();

        //UnCopyable
        DllLoader(const DllLoader& other)        = delete;
        DllLoader& operator=(const DllLoader&)   = delete;

        //UnMoveable
        DllLoader(DllLoader&& other)             = delete;
        DllLoader& operator=(DllLoader&&)        = delete;

    private:
        Subscriber<std::string, DllLoader> m_sub;
        std::vector<void*> m_handle_vector;
    };
    
}//ILRD

 
#endif     /* __PLUG_AND_PLAY_H__ */

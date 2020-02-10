/*****************************************************************************
 * File name:   driver_proxy.h
 * Developer:   
 * Version: 	4
 * Date:        2019-03-27 10:26:28
 * Description: 
 *****************************************************************************/

#ifndef __DRIVER_PROXY_H__
#define __DRIVER_PROXY_H__

#include <memory>
#include <cstddef>      /* size_t */
#include <vector>
#include <thread>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <atomic>

#include "thread_pool.h"

namespace ilrd
{

const size_t id_size = 8;

// Manage the request 
class DataRequest
{
public:
    enum class RequestType
    {
        READ,
        WRITE,
        DISCONNECT,
        FLUSH,
        TRIM
    };
    DataRequest(const char req_id[id_size], const RequestType type, 
                const std::vector<char>& data, size_t offset, size_t length);
    ~DataRequest();

    const char* GetID() const;
    RequestType GetType() const;
    const char* GetData() const;
    size_t GetOffset() const;
    size_t GetLen() const;
    unsigned int GetError() const;
    
    void SetData(std::vector<char>&& data);
    void SetOffset(size_t offset);
    void SetLength(size_t length);
    void SetError(unsigned int error);

    DataRequest(const DataRequest&) = delete;
    DataRequest& operator=(const DataRequest&) = delete;    
private:
    char m_id[id_size];
    RequestType m_type; 
    std::vector<char> m_data;
    size_t m_offset;
    size_t m_len;
    unsigned int m_error;
};

// like BUSE, control streaming between <device&driver> and <memory dest>
class DriverProxy final // cannot be inherited
{
public:    
    // static std::unique_ptr<DriverProxy> CreateDriverProxy(const std::string& dev);
    DriverProxy(const std::string& dev_path, size_t storage_size);
    DriverProxy(const std::string& dev_path, size_t block_size, 
                                                        size_t num_of_blocks);
    
    ~DriverProxy();
    std::unique_ptr<DataRequest> GetRequest();
    void Reply(std::unique_ptr<DataRequest> request);
    
    void Disconnect();
    int GetFD();

    //Uncopyable 
    DriverProxy(const DriverProxy&) = delete;               
    DriverProxy& operator=(const DriverProxy&) = delete;     
private:
    class FD
    {
    public:
        explicit FD(int fd = -1) : m_fd(fd) { }
        ~FD() { if (m_fd != -1) close(m_fd); }

        int GetFD() const { return m_fd; }

        FD& operator=(FD&& other) noexcept
        {
            if (m_fd != -1)
            {
                close(m_fd);
            }

            m_fd = other.m_fd;
            
            other.m_fd = -1;
            
            return *this;
        }
        // operator int() { return m_fd; }
        FD(const FD&) = delete;
        FD& operator=(const FD&) = delete;
    private:
        int m_fd;
    };

    void Init();
    void ThreadFunc();
    const std::string m_dev_path;
    size_t m_storage_size;
    std::thread m_thread;
    
    FD m_sp[2];
    FD m_nbd;
    
    // v.0.3
    ThreadPool m_pool;

    // v.0.4
    std::mutex m_mutex;
};


}
#endif     /* __DRIVER_PROXY_H__ */

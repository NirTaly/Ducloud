/******************************************************************************
                                    DuCloud
                                    Driver Proxy                  
                                    Nir Tali
                                    18.03.19
 *****************************************************************************/

#include <sys/epoll.h>
#include <iostream>
#include <signal.h>
#include <cstring>
#include <thread>
#include <mutex>

#include "driver_proxy.h"
#include "handleton.h"
#include "logger.h"
#include "thread_pool.h"
#include "factory.h"
#include "storage.h"
#include "utils.h"

using namespace ilrd;

#define KB          (1024)
#define MB          (KB * KB)
#define GB          (KB * MB)
#define MAX_EVENTS  (2)
#define READ_SIZE   (KB)
#define SEC         (1000)
#define TIMEOUT     (3 * SEC)
#define STORAGE_SIZE (200*MB)

int SetSignals();
int SetEpoll(struct epoll_event* event_stdin, struct epoll_event* event_nbd);

Handleton<Logger> log;
bool to_disconnect = false;
DriverProxy* dp;
char buffer[READ_SIZE];
Storage storage(STORAGE_SIZE);

/*****************************************************************************/
/*****************************************************************************/
struct Params
{
    Params(std::mutex& mtx, std::unique_ptr<DataRequest> req)
        : m_mutex(mtx), m_request(std::move(req)) { }

    Params(const Params& p)
        : m_mutex(p.m_mutex), m_request(std::move(p.m_request)) { }

    std::mutex& m_mutex;
    mutable std::unique_ptr<DataRequest> m_request;
};

/*****************************************************************************/
class DPTask : public Task
{
public:
    DPTask(const Params& params)
        : m_mutex(params.m_mutex), m_request(std::move(params.m_request)) { }

    virtual ~DPTask() { }
    
    void Lock() { m_mutex.lock(); }
    void UnLock() { m_mutex.unlock(); }
    std::unique_ptr<DataRequest>& GetUniqueRequest() { return m_request; }
private:
    virtual void Execute() = 0;

    std::mutex& m_mutex;
    std::unique_ptr<DataRequest> m_request;
};

/*****************************************************************************/
class WriteTask : public DPTask
{
public:
    WriteTask(const Params& params)
        : DPTask(params) { }
private:
    void Execute()  
    { 
        std::cout << "WriteTask::Execute\n";
        storage.Write(GetUniqueRequest()->GetData(), 
                GetUniqueRequest()->GetOffset(), GetUniqueRequest()->GetLen());
    
        Lock();
        dp->Reply(std::move(GetUniqueRequest()));
        UnLock();
    }
};

/*****************************************************************************/
class ReadTask : public DPTask
{
public:
    ReadTask(const Params& params)
        : DPTask(params) { }
private:
    void Execute()  
    { 
        std::cout << "ReadTask::Execute\n";
        std::vector<char> data;

        storage.Read(data, GetUniqueRequest()->GetOffset(), 
                                                GetUniqueRequest()->GetLen());
        
        GetUniqueRequest()->SetData(std::move(data));
    
        Lock();
        dp->Reply(std::move(GetUniqueRequest()));
        UnLock();
    }
};

/*****************************************************************************/
class DiscTask : public DPTask
{
public:
    DiscTask(const Params& params)
        : DPTask(params) { }
private:
    void Execute()  
    { 
        std::cout << "DiscTask::Execute\n";
        to_disconnect = true;        
    }
};

/*****************************************************************************/
class OtherTask : public DPTask
{
public:
    OtherTask(const Params& params)
        : DPTask(params) { }
private:
    void Execute()  
    { 
        std::cout << "Other Event" << std::endl;

        Lock();
        dp->Reply(std::move(GetUniqueRequest()));
        UnLock();
    }
};

/*****************************************************************************/
/*****************************************************************************/


static int SetSigaction(int sig, const struct sigaction * act) 
{
    struct sigaction oact;
    int r = sigaction(sig, act, &oact);
    if (r == 0 && oact.sa_handler != SIG_DFL) 
    {
        log->Write("overriden non-default signal handler");
    }
    
    return r;
}


static void DisconnectHandler(int signal) 
{
    (void)signal;
    std::cout << "Handler" << std::endl;
    to_disconnect = true;
}


/*****************************************************************************/


int main()
{
    if (EXIT_FAILURE == SetSignals())
    {
        return EXIT_FAILURE;
    }

    struct epoll_event event_stdin, event_nbd, events[MAX_EVENTS];
    int epoll_fd = SetEpoll(&event_stdin, &event_nbd);
    if (epoll_fd == -1)
    {
        return EXIT_FAILURE;
    }
    
    try
    {
        DriverProxy driver_proxy("/dev/nbd1", STORAGE_SIZE);
        dp = &driver_proxy;
 
        Factory<Task, int, Params> factory;

        factory.Add(0, [](Params params) 
                    { return std::unique_ptr<Task>( new ReadTask(params)); });
        factory.Add(1, [](Params params) 
                    { return std::unique_ptr<Task>( new WriteTask(params)); });
        factory.Add(2, [](Params params) 
                    { return std::unique_ptr<Task>( new DiscTask(params)); });
        factory.Add(3, [](Params params) 
                    { return std::unique_ptr<Task>( new OtherTask(params)); });
        factory.Add(4, [](Params params) 
                    { return std::unique_ptr<Task>( new OtherTask(params)); });

        event_nbd.data.fd = dp->GetFD();

        if( epoll_ctl(epoll_fd, EPOLL_CTL_ADD, STDIN_FILENO, &event_stdin) ||
            epoll_ctl(epoll_fd, EPOLL_CTL_ADD, dp->GetFD(), &event_nbd))
        {
            log->Write("Failed to add file descriptor to epoll");
            close(epoll_fd);

            return 1;
        }
          
        bzero(events, sizeof(events[0]) * MAX_EVENTS);
        
        std::mutex mtx;
        ThreadPool m_pool;

        while (to_disconnect == false)
        {
            std::cout << "Enter main While" << std::endl;
            bzero(buffer, READ_SIZE);
            
            int event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, TIMEOUT);
            if(to_disconnect == true)
            {
                dp->Disconnect();
                break;
            }

            for (int i = 0; i < event_count; ++i)
            {
                // Received stdin event
                if (STDIN_FILENO == events[i].data.fd)
                {
                    std::cout << "Received stdin Event" << std::endl;

                    int bytes_read = read(events[i].data.fd, buffer, READ_SIZE);
                    buffer[bytes_read] = '\0';
                    // Quit By User
                    if (0 == strcmp(buffer, "q\n"))
                    {
                        std::cout << "Disconnecting..." << std::endl;
                        to_disconnect = true;
                        dp->Disconnect();  
                    }
                }
                // Received nbd event
                else if (dp->GetFD() == events[i].data.fd)
                {
                    std::cout << "Received nbd Event" << std::endl;
                    std::unique_ptr<DataRequest> req = dp->GetRequest();
                    if (!req )
                    {
                        log->Write("Error: Req = NULL");
                        return -1;
                    }
                    
                    int request_type = static_cast<int>(req->GetType());
                    m_pool.AddTask(factory.CreateShared
                            (request_type, Params(mtx, std::move(req))));
                }
            }
        }
    }
    catch(const std::exception& e)
    {
        log->Write(e.what());        
    }

    std::cout << "Exit Normally" << std::endl;
}

/******************************************************************************/
void ThreadPerRequest(std::unique_ptr<DataRequest> req, std::mutex& mtx)
{
    char buffer[READ_SIZE];

    req->SetError(0);

    memset(buffer, 1, READ_SIZE);

    switch(req->GetType())
    {
        case DataRequest::RequestType::READ:
            std::cout << "Read Event" << std::endl;
            req->SetData(std::vector<char>(req->GetLen() ));
            std::cout << "GetData: " << req->GetData() << std::endl;

            break;

        case DataRequest::RequestType::WRITE:

            std::cout << "Write Event" << std::endl;
            memset(buffer, 'W', READ_SIZE);
            // req->SetError(ReadAll(dp.GetFD(), buffer, req->GetLen()));

            break;

        case DataRequest::RequestType::DISCONNECT:
            to_disconnect = true;

            break;

        default:
            std::cout << "Other Event" << std::endl;                            ;
            
        // case DataRequest::RequestType::FLUSH:
        //     ...
        //     break;

        // case DataRequest::RequestType::TRIM:
        //     ...
        //     break;
    }

    std::cout << "Reply buffer... " << std::endl;

    mtx.lock();
    dp->Reply(std::move(req));
    mtx.unlock();
}
/******************************************************************************/
int SetSignals()
{
    struct sigaction act;
    act.sa_handler = DisconnectHandler;
    act.sa_flags = SA_RESTART;

    if (sigemptyset(&act.sa_mask) != 0 ||
        sigaddset(&act.sa_mask, SIGINT) != 0) 
    {
        log->Write("SetSignals(): sigempty and sigadd");        
        // perror("sigempty and sigadd");
        return EXIT_FAILURE;
    }
    /* Sets signal action like regular sigaction but is suspicious. */
    if (SetSigaction(SIGINT, &act) != 0) 
    {
        log->Write("SetSignals(): failed to register signal handlers in parent");        
        // fprintf(stderr, "failed to register signal handlers in parent");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
/******************************************************************************/

int SetEpoll(struct epoll_event* event_stdin, struct epoll_event* event_nbd)
{
    int epoll_fd = epoll_create1(0);
    if(epoll_fd == -1)
    {
        log->Write("SetEpoll(): Failed to create epoll file descriptor");        
        // fprintf(stderr, "Failed to create epoll file descriptor\n");
        return -1;
    }
    
    bzero(event_stdin, sizeof(struct epoll_event));
    bzero(event_nbd, sizeof(struct epoll_event));
    
    event_stdin->events = EPOLLIN;
    event_stdin->data.fd = STDIN_FILENO;

    event_nbd->events = EPOLLIN;

    return epoll_fd;
}
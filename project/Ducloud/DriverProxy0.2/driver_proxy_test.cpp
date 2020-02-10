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

#include "../../include/driver_proxy.h"
#include "../../include/handleton.h"
#include "../../include/logger.h"

using namespace ilrd;

/*****************************************************************************/
Handleton<Logger> log;

static int SetSigaction(int sig, const struct sigaction * act) 
{
    struct sigaction oact;
    int r = sigaction(sig, act, &oact);
    if (r == 0 && oact.sa_handler != SIG_DFL) 
    {
        log->Write("overriden non-default signal handler");
        // fprintf(stderr, "overriden non-default signal handler");
    }
    
    return r;
}

bool to_disconnect = false;
static void DisconnectHandler(int signal) 
{
    (void)signal;
    std::cout << "Handler" << std::endl;
    to_disconnect = true;
}


/*****************************************************************************/

#define KB (1024)
#define MB (KB * KB)
#define GB (KB * MB)
#define MAX_EVENTS (2)
#define READ_SIZE (KB)

int SetSignals();
int SetEpoll(struct epoll_event* event_stdin, struct epoll_event* event_nbd);
void ThreadPerRequest(std::unique_ptr<DataRequest> req, std::mutex& mtx);

DriverProxy* dp;

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
        DriverProxy driver_proxy("/dev/nbd2", 200*MB);
        dp = &driver_proxy;

        event_nbd.data.fd = dp->GetFD();

        if( epoll_ctl(epoll_fd, EPOLL_CTL_ADD, STDIN_FILENO, &event_stdin) ||
            epoll_ctl(epoll_fd, EPOLL_CTL_ADD, dp->GetFD(), &event_nbd))
        {
            log->Write("Failed to add file descriptor to epoll");
            // fprintf(stderr, "Failed to add file descriptor to epoll\n");
            close(epoll_fd);
            return 1;
        }

        char buffer[READ_SIZE];
                
        bzero(events, sizeof(events[0]) * MAX_EVENTS);
        std::mutex mtx;

        while (to_disconnect == false)
        {
            bzero(buffer, READ_SIZE);
            
            int event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, 2);
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
                    std::unique_ptr<DataRequest> req = dp->GetRequest();
                    if (req == NULL)
                    {
                        log->Write("Error: Req = NULL");
                        // std::cout << "Error: Req = NULL" << std::endl;
                    }

                    std::thread th(ThreadPerRequest, std::move(req), std::ref(mtx));
                    th.detach();
                    // th.join();
                }
            }
        }
            
    }
    catch(const std::exception& e)
    {
        log->Write(e.what());        
        // std::cerr << "Ctor: " << e.what() << std::endl;
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
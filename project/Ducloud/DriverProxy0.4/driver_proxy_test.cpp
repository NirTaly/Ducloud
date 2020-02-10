/******************************************************************************
                                    DuCloud
                                    Driver Proxy                  
                                    Nir Tali
                                    18.03.19
 *****************************************************************************/

#include <iostream>
#include <signal.h>
#include <mutex>

#include "request_engine.h"
#include "driver_proxy.h"
#include "storage.h"

using namespace ilrd;

#define MAX_EVENTS  (2)
#define READ_SIZE   (KB)

int SetSignals();


/*****************************************************************************/
/*****************************************************************************/
struct Params
{
    Params(std::unique_ptr<DataRequest> req)
        : m_request(std::move(req)) { }

    Params(const Params& p)
        : m_request(std::move(p.m_request)) { }

    mutable std::unique_ptr<DataRequest> m_request;
};

/*****************************************************************************/
/*****************************************************************************/
RequestEngine<int, Params> req_eng(  
                    "/home/user/git/project/Ducloud/DriverProxy0.4/monitored/");
DriverProxy* g_dp;
Handleton<Logger> logger;
Storage storage(STORAGE_SIZE);

/*****************************************************************************/
/*****************************************************************************/
class DPTask : public RequestEngine<int,Params>::Task
{
public:
    DPTask(const Params& params)
        : m_request(std::move(params.m_request)) { }

    virtual ~DPTask() { }

    std::unique_ptr<DataRequest>& GetUniqueRequest() { return m_request; }
private:
    virtual void Execute() = 0;

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
    
        g_dp->Reply(std::move(GetUniqueRequest()));
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
    
        g_dp->Reply(std::move(GetUniqueRequest()));
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
        req_eng.Stop();
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

        g_dp->Reply(std::move(GetUniqueRequest()));
    }
};

/*****************************************************************************/
/*****************************************************************************/
class NBDGateway : public RequestEngine<int, Params>::Gateway
{
public:
    NBDGateway(DriverProxy& dp)
        : RequestEngine<int, Params>::Gateway(dp.GetFD()), m_dp(&dp) { }

    ~NBDGateway() 
    { 
        m_dp->Disconnect();
        req_eng.Stop();
    }

    std::pair<int, Params> Read()
    {
        std::unique_ptr<DataRequest> req = m_dp->GetRequest();
        if (!req )
        {
            logger->Write("Error: Req = NULL");
            throw std::runtime_error("NBDGateway::Read() - req==NULL");
        }
        
        int request_type = static_cast<int>(req->GetType());
        return (std::make_pair(request_type, Params(std::move(req))));
    }
private:
    DriverProxy* m_dp;
};
/*****************************************************************************/
/*****************************************************************************/


static int SetSigaction(int sig, const struct sigaction * act) 
{
    struct sigaction oact;
    int r = sigaction(sig, act, &oact);
    if (r == 0 && oact.sa_handler != SIG_DFL) 
    {
        logger->Write("overriden non-default signal handler");
    }
    
    return r;
}


static void DisconnectHandler(int signal) 
{
    (void)signal;
    std::cout << "Handler" << std::endl;
    req_eng.Stop();
}


/*****************************************************************************/
int main()
{
    if (EXIT_FAILURE == SetSignals())
    {
        return EXIT_FAILURE;
    }

    try
    {
        DriverProxy driver_proxy("/dev/nbd2", STORAGE_SIZE);
        g_dp = &driver_proxy;
        
        NBDGateway nbd_gateway(driver_proxy);

        req_eng.AddGateway(nbd_gateway);

        req_eng.ConfigTask(0, [](Params params) 
                    { return std::unique_ptr<RequestEngine<int,Params>::Task>
                                                    ( new ReadTask(params)); });
        req_eng.ConfigTask(1, [](Params params) 
                    { return std::unique_ptr<RequestEngine<int,Params>::Task>
                                                    ( new WriteTask(params)); });
        req_eng.ConfigTask(2, [](Params params) 
                    { return std::unique_ptr<RequestEngine<int,Params>::Task>
                                                    ( new DiscTask(params)); });
        req_eng.ConfigTask(3, [](Params params) 
                    { return std::unique_ptr<RequestEngine<int,Params>::Task>
                                                    ( new OtherTask(params)); });
        req_eng.ConfigTask(4, [](Params params) 
                    { return std::unique_ptr<RequestEngine<int,Params>::Task>
                                                    ( new OtherTask(params)); });

        req_eng.Run();
    }
    catch(const std::exception& e)
    {
        logger->Write(e.what());        
    }

    std::cout << "Exit Normally" << std::endl;
    return EXIT_SUCCESS;
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
        logger->Write("SetSignals(): sigempty and sigadd");        
        // perror("sigempty and sigadd");
        return EXIT_FAILURE;
    }
    /* Sets signal action like regular sigaction but is suspicious. */
    if (SetSigaction(SIGINT, &act) != 0) 
    {
        logger->Write("SetSignals(): failed to register signal handlers in parent");        
        // fprintf(stderr, "failed to register signal handlers in parent");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
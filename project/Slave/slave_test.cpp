/******************************************************************************
                                    DuCloud
                                    Driver Proxy                  
                                    Nir Tali
                                    18.03.19
 *****************************************************************************/

#include <iostream>
#include <signal.h>
#include <libconfig.h++>
#include <arpa/inet.h>
#include <sstream>      // std::stringstream

#include "utils.h"
#include "request_engine.h"
#include "driver_proxy.h"
#include "storage.h"

using namespace ilrd;
using namespace libconfig;

#define READ_SIZE   (4*KB + 40)
struct UDPMessage;

int SetSignals();
void ReplyToMaster(UDPMessage *msg);

/*****************************************************************************/
/*****************************************************************************/
struct UDPMessage
{
	UDPMessage(int socket_fd, int req_id, DataRequest::RequestType type, std::vector<char> data,
													size_t offset, size_t len)
		: m_socket(socket_fd), m_req_id(req_id), m_type(type), m_data(std::move(data)), 
													m_offset(offset), m_len(len)
		{ }
	
	UDPMessage(const UDPMessage& msg): m_req_id(msg.m_req_id), m_type(msg.m_type),
		m_data(msg.m_data), m_offset(msg.m_offset), m_len(msg.m_len)
		{
			m_socket = std::move(msg.m_socket); 
		}

	int m_socket;
	int m_req_id;
    DataRequest::RequestType m_type; 
	std::vector<char> m_data;
    size_t m_offset;
    size_t m_len;
};
/*****************************************************************************/

struct Params
{
    Params(std::unique_ptr<UDPMessage> msg)
        : m_msg(std::move(msg)) {  }

    Params(const Params& p)
        : m_msg(std::move(p.m_msg)) { }
    
    mutable std::unique_ptr<UDPMessage> m_msg;
};

/*****************************************************************************/
/*****************************************************************************/
RequestEngine<int, Params> req_eng(  
                    "/home/user/git/project/Ducloud/DriverProxy0.4/monitored/");
Handleton<Logger> logger;
Storage storage(8*MB);
struct sockaddr_in address;

/*****************************************************************************/
/*****************************************************************************/
class DPTask : public RequestEngine<int,Params>::Task
{
public:
    DPTask(const Params& params)
        : m_msg(std::move(params.m_msg)) { }

    virtual ~DPTask() { }

    std::unique_ptr<UDPMessage>& GetUniqueMsg() { return m_msg; }
private:
    virtual void Execute() = 0;

    std::unique_ptr<UDPMessage> m_msg;
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
        // std::cout << "WriteTask::Execute\n";

        storage.Write(GetUniqueMsg()->m_data.data(), 
                GetUniqueMsg()->m_offset, GetUniqueMsg()->m_len);
    
        if (GetUniqueMsg()->m_offset < 4*MB)
        {
            ReplyToMaster(GetUniqueMsg().get());
        }
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
        // std::cout << "ReadTask::Execute\n";
      
        std::vector<char> data(READ_SIZE);

        // std::cout << "offset: " << GetUniqueMsg()->m_offset << std::endl;
        // std::cout << "len: " << GetUniqueMsg()->m_len << std::endl;
        storage.Read(data, GetUniqueMsg()->m_offset, GetUniqueMsg()->m_len);

        GetUniqueMsg()->m_data = data;

        ReplyToMaster(GetUniqueMsg().get());
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

        ReplyToMaster(GetUniqueMsg().get());
    }
};

/*****************************************************************************/
/*****************************************************************************/
class MasterGateway : public RequestEngine<int, Params>::Gateway
{
public:
    MasterGateway(int port, std::string master_ip)
        : RequestEngine<int, Params>::Gateway(
                                        socket(AF_INET,SOCK_DGRAM, IPPROTO_UDP))
    {
        if (GetFD() == -1)
        {
            logger->Write("MasterGateway::socket failed"); 
            throw std::runtime_error("MasterGateway::socket failed");
        }

        bzero(&address, sizeof(struct sockaddr_in));
        address.sin_family = AF_INET; 
        address.sin_port = htons(port); 
        address.sin_addr.s_addr = INADDR_ANY;

        if (inet_pton(AF_INET, master_ip.c_str(), &address.sin_addr) <= 0)  
        { 
            logger->Write("Gateway::inet_pton()-Invalid address/Address not supported"); 
            throw std::runtime_error("Gateway::inet_pton()");
        }

        // connection messege
        char buffer[] = "Hi Master I am your slave";
        sendto(GetFD(), buffer,sizeof(buffer), 0, (const struct sockaddr*)&address, 
                                                    sizeof(struct sockaddr_in));
    }

    ~MasterGateway() = default;

    std::pair<int, Params> Read()
    {
        int struct_size = sizeof(struct sockaddr_in); 
        std::vector<char> buffer(READ_SIZE);
        struct sockaddr_in client_addr;
        bzero(&client_addr, sizeof(struct sockaddr_in));

        int n = recvfrom(GetFD(), buffer.data(), READ_SIZE, 0,
                (struct sockaddr *) &client_addr, 
                reinterpret_cast<socklen_t *>(&struct_size));
        if (-1 == n)
        {
            throw std::runtime_error("MasterGateway()::Read::recvfrom");
        }

        logger->Write("Recv - data:");
        logger->Write(buffer.data());
        logger->Write("");

        std::unique_ptr<UDPMessage> unq_msg(new UDPMessage(Deserialize(buffer)));

        int msg_type = static_cast<int>(unq_msg->m_type);
        return (std::make_pair(msg_type, Params(std::move(unq_msg))));
    }
    
    UDPMessage Deserialize(std::vector<char> vec)
    {
        char* runner    = vec.data();
        size_t offset = 0, len = 0;
        int req_id = 0, type = 0;
        std::vector<char> data(READ_SIZE);
        
        sscanf(runner, "%lu ; %lu ; %d ; %d ; ", &offset,&len,&req_id, &type);
        
        runner          = strchr(runner, '\0') + 1;
        memcpy(data.data(), runner, len);

        std::cout << "len = " << len << std::endl;
        std::cout << "offset = " << offset << std::endl;

        DataRequest::RequestType m_type;
        ((0 == type) ?  m_type = DataRequest::RequestType::READ : 
                        m_type = DataRequest::RequestType::WRITE);

        return (UDPMessage(GetFD(),req_id, m_type, data, offset, len));
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
        logger->Write("overriden non-default signal handler");
    }
    
    return r;
}


static void DisconnectHandler(int signal) 
{
    (void)signal;
    // std::cout << "Handler" << std::endl;
    req_eng.Stop();
}


/*****************************************************************************/
int main()
{
    if (EXIT_FAILURE == SetSignals())
    {
        return EXIT_FAILURE;
    }

    Config cfg;

    try
    {
        cfg.readFile("slave_config.cfg");        
    }
    catch(const FileIOException &fioex)
    {
        std::cerr << "I/O error while reading file." << std::endl;
        return(EXIT_FAILURE);
    }
    catch(const ParseException &pex)
    {
        std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
                << " - " << pex.getError() << std::endl;
        return(EXIT_FAILURE);
    }
    
    std::string master_ip = cfg.lookup("master_ip");
    int socket_port = cfg.lookup("slave_socket_port");
    MasterGateway master_gateway(socket_port, master_ip);

    req_eng.AddGateway(master_gateway);

    req_eng.ConfigTask(0, [](Params params) 
                { return std::unique_ptr<RequestEngine<int,Params>::Task>
                                                ( new ReadTask(params)); });
    req_eng.ConfigTask(1, [](Params params) 
                { return std::unique_ptr<RequestEngine<int,Params>::Task>
                                                ( new WriteTask(params)); });

    req_eng.Run();

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

/******************************************************************************/

void Serialize(std::vector<char>& serial_vec, UDPMessage& msg)
{
    int data_index = sprintf(serial_vec.data(), "%lu;%d;%d;%c", msg.m_len, msg.m_req_id, 
                            static_cast<int>(msg.m_type), 0);
    
    memcpy(&serial_vec.data()[data_index],msg.m_data.data(),msg.m_len);
}

/******************************************************************************/
void ReplyToMaster(UDPMessage *msg)
{
    std::vector<char> buffer(READ_SIZE);
    Serialize(buffer, *msg);

    int struct_size = sizeof(struct sockaddr_in); 
    // std::cout << "stream.str().size(): " << stream.str().size() << std::endl;

    logger->Write("Reply - data: ");
    logger->Write(buffer.data());
    logger->Write("");

    if (-1 == sendto(msg->m_socket, buffer.data(), READ_SIZE, 0, 
                                (const struct sockaddr*)&address, struct_size))
    {
        throw std::runtime_error("ReplyToMaster()::sendto()");
    }
}
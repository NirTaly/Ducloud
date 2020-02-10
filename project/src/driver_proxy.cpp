/******************************************************************************
                                    DuCloud
                                    Driver Proxy                  
                                    Nir Tali
                                    18.03.19
 *****************************************************************************/

#include <linux/nbd.h>
#include <cstring>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <linux/nbd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "../include/driver_proxy.h"
#include "../include/handleton.h"
#include "../include/logger.h"

#include <iostream>
namespace ilrd
{

static int WriteAll(int fd, char* buf, size_t count);
static int ReadAll(int fd, char* buf, size_t count);
/*
 * These helper functions were taken from cliserv.h in the nbd distribution.
 */
#ifdef WORDS_BIGENDIAN
u_int64_t ntohll(u_int64_t a) 
    {
        return a;
    }
#else
    u_int64_t ntohll(u_int64_t a) 
    {
        u_int32_t lo = a & 0xffffffff;
        u_int32_t hi = a >> 32U;
        lo = ntohl(lo);
        hi = ntohl(hi);
        return ((u_int64_t) lo) << 32U | hi;
    }
#endif


/*****************************************************************************/
DataRequest::DataRequest(const char req_id[id_size], const RequestType type, 
            const std::vector<char>& data, size_t offset, size_t length) :

m_type(type), m_data(data), m_offset(offset), m_len(length), m_error(0)
{
    memcpy(m_id, req_id, id_size);
}

/*****************************************************************************/
DataRequest::~DataRequest() { }

/*****************************************************************************/
const char* DataRequest::GetID() const { return m_id; }

/*****************************************************************************/
DataRequest::RequestType DataRequest::GetType() const { return m_type; }

/*****************************************************************************/
const char* DataRequest::GetData() const { return m_data.data(); }

/*****************************************************************************/
size_t DataRequest::GetOffset() const { return m_offset; }

/*****************************************************************************/
size_t DataRequest::GetLen() const { return m_len; }

/*****************************************************************************/
unsigned int DataRequest::GetError() const { return m_error; }

/*****************************************************************************/
void DataRequest::SetData(std::vector<char>&& data) { m_data = std::move(data);}

/*****************************************************************************/
void DataRequest::SetOffset(size_t offset) { m_offset = offset; }

/*****************************************************************************/
void DataRequest::SetLength(size_t length) { m_len = length; }

/*****************************************************************************/
void DataRequest::SetError(unsigned int error) { m_error = error; }

/*****************************************************************************/
/*****************************************************************************/

DriverProxy::DriverProxy(const std::string& dev_path, size_t storage_size)
    : m_dev_path(dev_path), m_storage_size(storage_size)
{
    Init();
}
/*****************************************************************************/

DriverProxy::DriverProxy(const std::string& dev_path, size_t block_size, 
                                                        size_t num_of_blocks)
    : m_dev_path(dev_path), m_storage_size(block_size * num_of_blocks)
                       
{
    Init();
}
/*****************************************************************************/
void DriverProxy::Init()
{
    Handleton<Logger> log;

    int sp[2];
    if (0 != socketpair(AF_UNIX, SOCK_STREAM, 0, sp))
    {
        log->Write("Failure: socketpair");
        throw std::runtime_error("socketpair");
    }

    m_sp[0] = std::move( FD(sp[0]));
    m_sp[1] = std::move( FD(sp[1]));
    
    m_nbd = std::move( FD(open(m_dev_path.data(), O_RDWR)) );
    if (m_nbd.GetFD() == -1) 
    {
        log->Write("Failure: open nbd");
        throw std::runtime_error("open");
    }

    if (-1 == ioctl(m_nbd.GetFD(), NBD_SET_SIZE, m_storage_size))
    {
        log->Write("Failure: ioctl #1");
        throw std::runtime_error("ioctl #1");
    }

    if (-1 == ioctl(m_nbd.GetFD(), NBD_CLEAR_SOCK))
    {
        log->Write("Failure: ioctl #2");
        throw std::runtime_error("ioctl #2");
    }

    m_thread = std::thread(&DriverProxy::ThreadFunc, this);

}

/*****************************************************************************/
void DriverProxy::ThreadFunc()
{
    int flags;
    Handleton<Logger> log;
    /* Block all signals to not get interrupted in ioctl(NBD_DO_IT), 
     * as it seems there is no good way to handle such interruption.
     */
    sigset_t sigset;
    if (sigfillset(&sigset) != 0 ||
        pthread_sigmask(SIG_SETMASK, &sigset, NULL) != 0) 
    {
        log->Write("Failure: block signals in thread");
        // perror("failed to block signals in thread");
        return;
    }

    if (ioctl(m_nbd.GetFD(), NBD_SET_SOCK, m_sp[1].GetFD()) == -1)
    {
        log->Write("Failure: ioctl(nbd, NBD_SET_SOCK, m_sp[1])");
        // perror("ioctl(nbd, NBD_SET_SOCK, m_sp[1]) failed");
        return;
    }
    else
    {
        #if defined NBD_SET_FLAGS
        flags = 0;
        #if defined NBD_FLAG_SEND_TRIM
        flags |= NBD_FLAG_SEND_TRIM;
        #endif
        #if defined NBD_FLAG_SEND_FLUSH
        flags |= NBD_FLAG_SEND_FLUSH;
        #endif
        if (flags != 0 && ioctl(m_nbd.GetFD(), NBD_SET_FLAGS, flags) == -1)
        {
            log->Write("Failure: ioctl(nbd, NBD_SET_FLAGS, flags)");
            // fprintf(stderr, "ioctl(nbd, NBD_SET_FLAGS, %d) failed.[%s]\n", flags, strerror(errno));
            return;
        }

        #endif
        
       ioctl(m_nbd.GetFD(), NBD_DO_IT);
    }
    
    return;
}
/*****************************************************************************/

DriverProxy::~DriverProxy()
{    
    m_thread.join();
}
/*****************************************************************************/
std::unique_ptr<DataRequest> DriverProxy::GetRequest()
{
    Handleton<Logger> log;
    u_int64_t offset;
    u_int32_t len;
    ssize_t bytes_read;
    struct nbd_request request;

    std::vector<char> data;

    DataRequest::RequestType req_type;
    if ((bytes_read = read(m_sp[0].GetFD(), &request, sizeof(request))) > 0)
    {
        len    = ntohl(request.len);
        offset = ntohll(request.from);
        
       /*  std::cout << "GetRequest(): bytes_read:" << bytes_read << std::endl;
        std::cout << "GetRequest(): len:   " << len << std::endl;
        std::cout << "GetRequest(): offset:" << offset << std::endl; */
        
        char* buf = new char[len];
        switch(ntohl(request.type)) 
        {
            case NBD_CMD_READ:
                req_type = DataRequest::RequestType::READ;
                break;

            case NBD_CMD_WRITE:
                req_type = DataRequest::RequestType::WRITE;
                
                ReadAll(m_sp[0].GetFD(), buf , len);
                data.insert(data.end(), buf, buf + len);
                break;

            case NBD_CMD_DISC:
                req_type = DataRequest::RequestType::DISCONNECT;
                break;

            case NBD_CMD_FLUSH:
                req_type = DataRequest::RequestType::FLUSH;
                break;

            case NBD_CMD_TRIM:
                req_type = DataRequest::RequestType::TRIM;
                break;

            default:
                throw std::exception();
        }
        delete[] buf;
    }

    if (bytes_read == -1)
    {
        log->Write("Failure: GetRequest() Read");
        // std::cerr << "GetRequest() Error Read!!" << std::endl;
    }
    return (std::unique_ptr<DataRequest>( new DataRequest(
                request.handle, req_type, data, offset, len)));
}

/*****************************************************************************/
void DriverProxy::Reply(std::unique_ptr<DataRequest> request)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    Handleton<Logger> log;

    struct nbd_reply reply;

    reply.magic = htonl(NBD_REPLY_MAGIC);
    reply.error = htonl(0);

    memcpy(reply.handle, request->GetID(), sizeof(reply.handle));
    reply.error = htonl(0);

    try
    {
        WriteAll(m_sp[0].GetFD(), (char*)&reply, sizeof(struct nbd_reply));
       
        if (request->GetType() == DataRequest::RequestType::READ)
        {
            // std::cerr << "Reply - READ: len = " << request->GetLen() << std::endl;
            WriteAll(m_sp[0].GetFD(), (char*)request->GetData(), request->GetLen());
        }
    }
    catch(const std::exception& e)
    {
        log->Write(std::string("Failure: Reply() Exception") + std::string(e.what()));
        // std::cerr << "Exception: " << e.what() << std::endl;
    }
}
/*****************************************************************************/

void DriverProxy::Disconnect()
{
    Handleton<Logger> log;

    if (m_nbd.GetFD() != -1) 
    {
        if (ioctl(m_nbd.GetFD(), NBD_DISCONNECT) != -1 &&
            ioctl(m_nbd.GetFD(), NBD_CLEAR_SOCK) != -1 &&
            ioctl(m_nbd.GetFD(), NBD_CLEAR_QUE)  != -1)
        {
            m_nbd = std::move( FD(-1));
        } 
        else 
        {
            log->Write("Failure: Disconnect");
            throw std::runtime_error("failed to disconnect");
        }
    }
}
/*****************************************************************************/
static int WriteAll(int fd, char* buf, size_t count)
{
    Handleton<Logger> log;

    int bytes_written;

    while (count > 0) 
    {
        bytes_written = write(fd, buf, count);
        if (bytes_written == -1)
        {
            log->Write("Failure: WriteAll(): write()");
            // std::cerr << "WriteAll(): write() Error" << std::endl;
        }
        buf += bytes_written;
        count -= bytes_written;
    }
    if  (count != 0)
    {
        log->Write("Failure: WriteAll(): write() - not all bytes");
        throw std::runtime_error("write - not all bytes");
    }

    return 0;
}

/*****************************************************************************/

int DriverProxy::GetFD() { return m_sp[0].GetFD(); }
/*****************************************************************************/


static int ReadAll(int fd, char* buf, size_t count)
{
    Handleton<Logger> log;

    int bytes_read;

    while (count > 0) 
    {
        bytes_read = read(fd, buf, count);
        if (bytes_read < 0)
        {
            log->Write("Failure: ReadAll(): read()");
            // std::cerr << "ReadAll(): read() Error" << std::endl;
            return -1;
        }

        buf += bytes_read;
        count -= bytes_read;
    }
    if (count != 0)
    {
        log->Write("Failure: ReadAll()(): read() - not all bytes");
        // std::cerr << "ReadAll(): count Error" << std::endl;
        return -1;
    }

  return 0;
}
} // ilrd

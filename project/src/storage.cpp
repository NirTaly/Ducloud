/*****************************************************************************
 * File name:   storage.cpp
 * Developer:   Yaron Meister
 * Reviewer: 
 * Date:        2019-04-01 17:54:44
 * Description: 
 *****************************************************************************/
 
#include <iostream>     /* 	cout, cerr	*/
#include <stdlib.h>     /* EXIT_SUCCESS */
#include <stdio.h>      /*    printf    */
#include <cstring>      /*    memcpy    */
#include <unistd.h>     /*     read     */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "utils.h"      /*     DBG      */
#include "storage.h"    

using namespace ilrd;
/*******************************************************************************
                           Functions Implementations
*******************************************************************************/  
/******************************************************************************/
Storage::Storage(size_t size, const std::string& s_name) : m_filename(s_name)
{
    int fd = open(m_filename.c_str(), O_CREAT | O_RDWR , 0666);
    if (-1 == fd) 
    {
        throw std::runtime_error("Storage open failed");
    }
    
    if (-1 == ftruncate(fd, size)) 
    {
        throw std::runtime_error("Storage ftruncate failed"); 
    }
    
    m_stream.open(m_filename.c_str(), std::ios::binary |
                                                  std::ios::out | std::ios::in);
}

/******************************************************************************/
Storage::~Storage()
{
    m_stream.close();
}

/******************************************************************************/
void Storage::Read(std::vector<char>& vec, size_t offset, size_t length)
{
    DBG(printf("R - %lu\n", length));

    std::lock_guard<std::mutex> lock(m_mutex);

    try
    {
        vec.resize(length);
        m_stream.seekg(offset);
        m_stream.read(vec.data(), length);
    }
    catch(const std::exception& e)
    {
        DBG(std::cerr << "Storage::Read:" << e.what() << '\n');
    }
}

/******************************************************************************/
void Storage::Write(const char* buffer, size_t offset, size_t length)
{
    DBG(printf("W - %lu\n", length));
    
    std::lock_guard<std::mutex> lock(m_mutex);

    try
    {
        m_stream.seekg(offset);
        m_stream.write(buffer, length);    
    }
    catch(const std::exception& e)
    {
        DBG(std::cerr << "Storage::Write:" << e.what() << '\n';)
    }
}

/******************************************************************************/
void Storage::Disconnect()
{
    std::cout << "Received a DISCONNECT request\n";
}

/******************************************************************************/
void Storage::Flush()
{
    std::cout << "Received a FLUSH request\n";
}

/******************************************************************************/
void Storage::Trim()
{
    std::cout << "Received a TRIM request\n";
}


/*******************************************************************************
******************************** END OF FILE ***********************************
*******************************************************************************/ 

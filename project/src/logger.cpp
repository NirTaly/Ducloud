/*****************************************************************************
 * File name:   logger.cpp
 * Developer:   HRD07
 * Version: 	0
 * Date:        2019-04-07
 * Description: 
 *****************************************************************************/

// #include <iostream>

#include "logger.h"

namespace ilrd
{

Logger::Logger()
{
    m_file.open ("log.txt");
}

Logger::~Logger()
{
    m_file.close();
}

void Logger::Write(const std::string& data)
{
    try
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_file << data << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        throw;
    }
    
}

} // ilrd

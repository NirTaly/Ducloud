/*****************************************************************************
 * File name:   logger.h
 * Developer:   HRD07
 * Version: 	0
 * Date:        2019-03-27 10:26:28
 * Description: 
 *****************************************************************************/

#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <fstream>      // std::fstream
#include <string>

#include "singleton.h"
#include "handleton.h"

namespace ilrd
{

class Logger
{
public:
    // friend class Handleton<Logger>;
    friend class Singleton<Logger>;
    
    ~Logger();
    void Write(const std::string& data);
    
    
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
private:
    Logger();
    std::mutex m_mutex;
    std::ofstream m_file;
};

// Instantiation of Singleton<Logger> ... for .so -> explicit and implicit
template class Singleton<Logger>;

}
#endif     /* __LOGGER_H__ */

/*****************************************************************************
 * File name:   storage.h
 * Developer:   HRD7
 * Version: 
 * Date:        2019-04-01 18:06:02
 * Description: 
 *****************************************************************************/

#ifndef __STORAGE_H__
#define __STORAGE_H__

#include <cstddef>      /* size_t       */
#include <vector>       /* std::vector  */
#include <fstream>      /* std::fstream */
#include <mutex>        /* std::mutex   */

namespace ilrd
{
class Storage
{
    public:
        Storage(size_t size,
                            const std::string& s_name = std::string("DP_disk"));
        ~Storage();

        Storage(const Storage&) = delete;
        Storage& operator=(const Storage&) = delete;

        void Read(std::vector<char>& vec, size_t offset, size_t length);
        void Write(const char* buffer, size_t offset, size_t length);
        void Disconnect();
        void Flush();
        void Trim();

    private:
        std::fstream m_stream;
        const std::string& m_filename;
        std::mutex m_mutex;
};

} // ilrd 
 
#endif     /* __STORAGE_H__ */


/*******************************************************************************
******************************** END OF FILE ***********************************
*******************************************************************************/

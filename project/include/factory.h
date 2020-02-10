/*****************************************************************************
 * File name:   factory.h
 * Developer:   HRD07
 * Version: 	1
 * Date:        2019-04-07 14:42:44
 * Description: Factory header
 *****************************************************************************/

#ifndef __FACTORY_H__
#define __FACTORY_H__


#include <unordered_map> /* std::unordered_map */
#include <functional>    /* std::function      */
#include <memory>        /* std::unique_ptr    */
#include <exception>     /* std::runtime_error */

namespace ilrd
{
// when <Params> is the param the enters Derived Ctor
template <typename Base, typename Key, typename Params>
class Factory
{
public:
    using FactoryFunc = std::function<std::unique_ptr<Base>(Params)>;

    Factory() = default;

    // Un-copyable
    Factory(const Factory&) = delete;
    Factory& operator=(const Factory&) = delete;

    std::unique_ptr<Base> Create(const Key& key, Params params);
    std::shared_ptr<Base> CreateShared(const Key& key, Params params);

    // Adding for an already created <key> results to a <false>
    // But! it is override the current inserted <value>
    bool Add(const Key& key, FactoryFunc creator);

    class BadKey : public std::logic_error
    {
    public:
        explicit BadKey(const std::string& message): 
                    logic_error(message.c_str()) { }
        explicit BadKey(const char* message): logic_error(message) { }
    };

    class BadCreate : public std::runtime_error
    {
    public:
        explicit BadCreate(const std::string& message): 
                    runtime_error(message.c_str()) { }                         
        explicit BadCreate(const char* message): runtime_error(message) { } 
    };

private:
    std::unordered_map<Key, FactoryFunc> m_map;
    
};
    

/******************************************************************************/
/******************************************************************************/
template <typename Base, typename Key, typename Params>
std::shared_ptr<Base> Factory<Base, Key, Params>::
            CreateShared(const Key& key, Params params)
{
    return (std::move(Create(key,params)));
}

/******************************************************************************/
template <typename Base, typename Key, typename Params>
std::unique_ptr<Base> Factory<Base, Key, Params>::
            Create(const Key& key, Params params)
{
    try
    {
        auto search = m_map.find(key);
        if (search == m_map.end())
        {
            throw BadKey("Key Not Found");
        }

        return search->second(params);
    }
    catch(const BadKey& e)
    {
        throw;
    }
    catch(const std::exception& e)
    {
        throw BadCreate("Return New Object");
    }     
}

template <typename Base, typename Key, typename Params>
bool Factory<Base, Key, Params>::
            Add(const Key& key, Factory<Base, Key, Params>::FactoryFunc creator)
{
    bool isnt_inserted;
    // if key isn't inserted already inserted
    if ((isnt_inserted = m_map.insert({key, creator}).second) == false)
    {
        m_map[key] = creator;
    }

    return isnt_inserted;
}


} // namespace ilrd
 

 
#endif     /* __FACTORY_H__ */
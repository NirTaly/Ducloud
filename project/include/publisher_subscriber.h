/*****************************************************************************
 * File name:   publisher_subscriber.h
 * Developer:   HRD07
 * Version: 	1
 * Date:        2019-04-18 12:57:57
 * Description: 
 *****************************************************************************/

#ifndef __PUBLISHER_SUBSCRIBER_H__
#define __PUBLISHER_SUBSCRIBER_H__

#include <cstddef>      // size_t 
#include <memory>       // shared_ptr
#include <list>         // list
#include <algorithm>    // std::find

namespace ilrd
{

template <typename Msg>
class Publisher;
/*****************************************************/
template <typename Msg>
class SubscriberBase		// CallbackBase
{
public: 
    friend class Publisher<Msg>;

    // "CallBack" has to be subscribed to any publisher
    SubscriberBase(Publisher<Msg>* publisher);
    virtual ~SubscriberBase();

    // Uncopiable
    SubscriberBase(const SubscriberBase&) = delete;
    SubscriberBase& operator=(const SubscriberBase&) = delete;

private:
    Publisher<Msg>* m_publisher;

    virtual void Notify(const Msg&) = 0;
    virtual void NotifyDeath() = 0;
};

/*****************************************************/
template <typename Msg, typename Observer>
class Subscriber : public SubscriberBase<Msg>	// Callback
{
public:
    // function for handling message on Notify()
    using ActionFunc = void (Observer::*)(const Msg&);  
    // function for handling NotifyDeath()
    using StopFunc   = void (Observer::*)();

    Subscriber(Publisher<Msg>* publisher,
               ActionFunc action_func   ,
               StopFunc stop_func       ,
               Observer& obj      );
    virtual ~Subscriber() override;

private:
    ActionFunc m_action_func;
    StopFunc m_stop_func;
    Observer& m_obj;
    // Subscriber get notified from <Publisher> with new <msg>
    void Notify(const Msg& msg) override;
    // Subscriber get notified from <Publisher> about <Publisher> death
    void NotifyDeath() override;

};

/*****************************************************/
template <typename Msg>    
class Publisher final		// Dispatcher
{
public:
    friend class SubscriberBase<Msg>;

    Publisher() = default;
    ~Publisher();

    // Uncopiable
    Publisher(const Publisher&) = delete;
    Publisher& operator=(const Publisher&) = delete;

    // Publishes <msg> to all Subscribers
    void Publish(const Msg& msg);
    void PublishDeath();

private:
    std::list<SubscriberBase<Msg>*> m_subscribers;
								      
    // Add/Remove <sub> to/from list of Subscribers 
    void Subscribe(SubscriberBase<Msg>* subscriber);
    void UnSubscribe(SubscriberBase<Msg>* subscriber);
};

/*****************************************************************************/
/*****************************************************************************/
template <typename Msg>    
SubscriberBase<Msg>::SubscriberBase(Publisher<Msg>* publisher)
    : m_publisher(publisher) 
{ 
    m_publisher->Subscribe(this); 
}

/*****************************************************************************/
template <typename Msg>    
SubscriberBase<Msg>::~SubscriberBase()
{
    if (m_publisher)
    {
        m_publisher->UnSubscribe(this);
        m_publisher = nullptr;
    }
}

/*****************************************************************************/
/*****************************************************************************/
template <typename Msg, typename Observer>
Subscriber<Msg,Observer>::Subscriber(Publisher<Msg>* publisher,
            ActionFunc action_func, StopFunc stop_func, Observer& obj)
               : SubscriberBase<Msg>(publisher), m_action_func(action_func),
                    m_stop_func(stop_func), m_obj(obj) { }

/*****************************************************************************/
template <typename Msg, typename Observer>
Subscriber<Msg,Observer>::~Subscriber()
{
    NotifyDeath();
}

/*****************************************************************************/
template <typename Msg, typename Observer>
void Subscriber<Msg,Observer>::Notify(const Msg& msg)
{
    (m_obj.*m_action_func)(msg);
}

/*****************************************************************************/
template <typename Msg, typename Observer>
void Subscriber<Msg,Observer>::NotifyDeath()
{
    (m_obj.*m_stop_func)();
}

/*****************************************************************************/
/*****************************************************************************/
template <typename Msg>    
Publisher<Msg>::~Publisher()
{
    PublishDeath();
}

/*****************************************************************************/
template <typename Msg>    
void Publisher<Msg>::Publish(const Msg& msg)
{
    for (auto iter = m_subscribers.begin(); iter != m_subscribers.end(); iter++)
    {
        if (*iter != nullptr)
        {
            (*iter)->Notify(msg);
        }
    }
}
 
/*****************************************************************************/
template <typename Msg>    
void Publisher<Msg>::PublishDeath()
{
    while (!m_subscribers.empty())
    {
        m_subscribers.back()->NotifyDeath();

        // reset subscriber's publisher
        m_subscribers.back()->m_publisher = nullptr;

        // reset & remove publisher's subscriber
        m_subscribers.pop_back();
    }
}

/*****************************************************************************/
template <typename Msg>    
void Publisher<Msg>::Subscribe(SubscriberBase<Msg>* subscriber)
{
    m_subscribers.push_front(subscriber);
}

/*****************************************************************************/
template <typename Msg>    
void Publisher<Msg>::UnSubscribe(SubscriberBase<Msg>* subscriber)
{
    auto found = std::find(m_subscribers.begin(), m_subscribers.end(), subscriber);
    if (found != m_subscribers.end())
    {
        // reset subscriber's publisher
        (*found)->m_publisher = nullptr;
        // reset & remove publisher's subscriber
        *found = nullptr;
        m_subscribers.erase(found);
    }
}

} // ILRD
 
#endif     /* __PUBLISHER_SUBSCRIBER_H__ */

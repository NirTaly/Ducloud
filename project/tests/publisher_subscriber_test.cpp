/*****************************************************************************
 * File name:   publisher_subscriber.h
 * Developer:   HRD07
 * Version: 	1
 * Date:        2019-04-18 12:57:57
 * Description: 
 *****************************************************************************/
#include <iostream>

#include "publisher_subscriber.h"

using namespace ilrd;

class Thermostat
{
public:
    Thermostat(float temp, Publisher<float>& publisher): m_temp(temp), m_publisher(publisher) { }
    void SetTemp(float temp) { m_temp = temp; m_publisher.Publish(m_temp); }
    float GetTemp() { return m_temp; }
    Publisher<float>* GetPublisher() { return &m_publisher; }
private:
    float m_temp;
    Publisher<float>& m_publisher;    
};

class AC1
{
public:
    AC1(Publisher<float>* pub):
                    m_subscriber(pub ,&AC1::ActionFunc, &AC1::StopFunc, *this) { }

    void ActionFunc(const float& temp) 
    {
        if (temp < 25)
            std::cout << "AC1: Action: " << temp << std::endl; 
        else
            std::cout << "AC1: It's Too Hot! " << temp << std::endl; 
        
    }
    void StopFunc() { std::cout << "AC1: Stop!!" << std::endl; }
    Subscriber<float, AC1>& GetSub() { return m_subscriber; }

private:
    Subscriber<float, AC1> m_subscriber;    
};

class AC2
{
public:
    AC2(Publisher<float>* pub):
                    m_subscriber(pub ,&AC2::ActionFunc, &AC2::StopFunc, *this) { }

    void ActionFunc(const float& temp) 
    {
        if (temp < 25)
            std::cout << "AC2: Action: " << temp << std::endl; 
        else
            std::cout << "AC2: It's Too Hot! " << temp << std::endl; 
        
    }
    void StopFunc() { std::cout << "AC2: Stop!!" << std::endl; }
    Subscriber<float, AC2>& GetSub() { return m_subscriber; }

private:
    Subscriber<float, AC2> m_subscriber;    
};


int main()
{
    Publisher<float>* publisher(new Publisher<float>);

    Thermostat thermo(20,*publisher);

    AC1 ac1(publisher);
    AC2 ac2(publisher);

    float temp = 22.2;
    while (temp < 40)
    {
        temp += 0.1;
        thermo.SetTemp(temp);
    }

    thermo.GetPublisher()->PublishDeath();

    thermo.SetTemp(100);

    delete publisher;
}
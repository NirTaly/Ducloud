/*****************************************************************************
 * File name:   plug_and_play_test.cpp
 * Developer:   HRD07
 * Version: 	0
 * Date:        2019-04-22 09:54:54
 * Description: 
 *****************************************************************************/

#include <iostream>

#include "plug_and_play.h"
#include "handleton.h"
#include "factory.h"

using namespace ilrd;
/*****************************************************************************/
class Base
{
public:
    Base(int num): m_num(num) { }
    virtual int GetNum() { return m_num; }
private:
    int m_num;
};

class D1: public Base
{
public:
    D1(int num): Base(num), m_num(num) { }
    int GetNum() { return m_num; }
private:
    int m_num;
};

class D2: public Base
{
public:
    D2(int num): Base(num), m_num(num) { }
    int GetNum() { return m_num; }
private:
    int m_num;
};

class D3: public Base
{
public:
    D3(int num): Base(num), m_num(num) { }
    int GetNum() { return m_num; }
private:
    int m_num;
};
/*****************************************************************************/
void FactoryIntegration();

int main()
{
    // DirMonitor monitor("/home/student/git/project/util/monitored/");
    // DllLoader loader(monitor.GetPublisher());

    // sleep(10);

    FactoryIntegration();
}


void FactoryIntegration()
{
    static DirMonitor monitor("/home/user/git/project/util/monitored/");
    static DllLoader loader(monitor.GetPublisher());

    Handleton<Factory<Base,std::string,int>> factory;

    factory->Add("D1", [](int num) { return std::unique_ptr<Base>( new D1(num)); });
    factory->Add("D2", [](int num) { return std::unique_ptr<Base>( new D2(num)); });
    factory->Add("D3", [](int num) { return std::unique_ptr<Base>( new D3(num)); });

    sleep(4);

    std::shared_ptr<Base> d5 = factory->CreateShared("D5", 15);
    std::cout << "d5: " << d5->GetNum() << std::endl;
}
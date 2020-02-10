#include "include/factory.h"
#include "include/handleton.h"

using namespace ilrd;

class Base
{
public:
    Base(int num): m_num(num) { }
    virtual int GetNum() { return m_num; }
private:
    int m_num;
};

class D5: public Base
{
public:
    D5(int num): Base(num), m_num(num) { }
    int GetNum() { return m_num; }
private:
    int m_num;
};


void FactoryUpdate() __attribute__((constructor));


void FactoryUpdate()
{
    Handleton<Factory<Base,std::string,int>> factory;
    
    factory->Add("D5", [](int num) { return std::unique_ptr<Base>( new D5(num)); });
}
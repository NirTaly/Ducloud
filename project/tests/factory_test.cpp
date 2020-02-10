/*****************************************************************************
 * File name:   factory.h
 * Developer:   Nir Tali
 * Version: 	0
 * Date:        2019-04-07 14:42:44
 * Description: Factory header
 *****************************************************************************/
#include <iostream>

#include "../../cpp/utils.h"
#include "factory.h"

using namespace ilrd;
/****************************************************************************/
class Undefined
{
public:
    Undefined(int num): m_num(num) { }
    virtual int GetNum() { return m_num; }
private:
    int m_num;
};

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
/****************************************************************************/

void GoodTest();
void KeyTest();
/****************************************************************************/

int main()
{
    GoodTest();
    KeyTest();

    return 0;
}

void GoodTest()
{
    Factory<Base, std::string, int> factory;

    factory.Add("D1", [](int num) { return std::unique_ptr<Base>( new D1(num)); });
    factory.Add("D2", [](int num) { return std::unique_ptr<Base>( new D2(num)); });
    factory.Add("D3", [](int num) { return std::unique_ptr<Base>( new D3(num)); });

    std::unique_ptr<Base> d1 = factory.Create("D1", 5);
    
    TEST_INT(1, d1->GetNum(), 5, "Create Add Good Test");
}

void KeyTest()
{
    Factory<Base, std::string, int> factory;
    bool is_inserted;
    is_inserted = factory.Add("D1", [](int num) { return std::unique_ptr<Base>( new D1(num)); });
    factory.Add("D2", [](int num) { return std::unique_ptr<Base>( new D2(num)); });
    factory.Add("D3", [](int num) { return std::unique_ptr<Base>( new D3(num)); });

    TEST_INT(2, is_inserted, true, "Add Function\t");

    try
    {
        std::unique_ptr<Base> d1 = factory.Create("D6", 5);
    }
    catch(const Factory<Base, std::string, int>::BadKey& badkey)
    {
        TEST_INT(3, true, true, "Exception - key");
    }
    
    Factory<Undefined, std::string, int> undef_factory;
    try
    {
        std::unique_ptr<Undefined> undef = undef_factory.Create("D5", 1);
    }
    catch(const Factory<Undefined, std::string, int>::BadKey& badkey)
    {
        TEST_INT(4, true, true, "Exception - class");
    }
    
    is_inserted = factory.Add("D3", nullptr);
    TEST_INT(5, is_inserted, false, "Duplicate key\t");
    
}
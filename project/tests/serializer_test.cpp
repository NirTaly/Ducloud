/*******************************************************************************
 * File name:   serializer_test.cpp
 * Developer:   Nir Tali	
 * Version:		0 
 * Date:        2019-04-08 13:09:47
 * Description: 
 ******************************************************************************/

#include <sstream>      // std::stringstream

#include "../../cpp/utils.h"
#include "serializer.h"

class Base
{
public:
    Base(const std::string& name, int age): m_name(name), m_age(age) { }
    virtual void Serialize(std::ostream& os) const
    {
        os << m_name << " " << m_age << " " ;
    }

    std::string GetName() { return m_name; }
    int GetAge() { return m_age; }

private:
    std::string m_name;
    int m_age;
};

class Derived1 : public Base
{
public:
    Derived1(const std::string& name, int age, const std::string& color, size_t height): 
            Base(name,age), m_color(color), m_height(height) { }

    std::string GetColor() { return m_color; }
    size_t GetHeight() { return m_height; }

    void Serialize(std::ostream& os) const
    {
        Base::Serialize(os);
        os << m_color << " " << m_height << std::endl;
    }
    bool operator==(Derived1& other)
    {
        return (GetName() == other.GetName() && GetAge() == other.GetAge() &&
                m_color == other.m_color && m_height == other.GetHeight());
    }
private:
    std::string m_color;
    size_t m_height;
};

class Derived2 : public Base
{
public:
    Derived2(const std::string& name, int age, const std::string& color, size_t height): 
            Base(name,age), m_id(1), m_color(color), m_height(height) { }
    void Serialize(std::ostream& os) const
    {
        Base::Serialize(os);
        os << m_color << " " << m_height << " " << m_id << std::endl;
    }
private:
    int m_id;
    std::string m_color;
    size_t m_height;
};

class Unrelated
{
    int what;
};
/******************************************************************************/
namespace ilrd
{
template <>
template <>
std::unique_ptr<Base> Serializer<Base>::Creator<Derived1>(std::istream& stream)
{
    std::string name, color;
    int age;
    size_t height;

    stream >> name >> age >> color >> height;
    
    return std::unique_ptr<Base>(new Derived1(name, age, color, height));
}

template <>
template <>
std::unique_ptr<Base> Serializer<Base>::Creator<Derived2>(std::istream& stream)
{
    std::string name, color, type_name;
    int age, id;
    size_t height;

    stream >> name >> age >> color >> height >> id;

    return std::unique_ptr<Base>(new Derived2(name, age, color, height));
}


} // ilrd

/******************************************************************************/

using namespace ilrd;

void UnitTest();
void AntiTest();

int main()
{
    UnitTest();

    AntiTest();

    return 0;
}

void UnitTest()
{
    Serializer<Base> serial;

    Derived1 d1("Abc", 23, "Green", 185);
    Derived2 d2("Grow", 99, "Yellow", 77);

    std::stringstream strs;
    serial.Serialize(strs, d1);
    serial.Serialize(strs, d2);

    std::cout << strs.str() << std::endl;
    serial.Add<Derived1>();
    serial.Add<Derived2>();

    std::unique_ptr<Base> equal_d1 = serial.Deserialize(strs);
    std::unique_ptr<Base> equal_d2 = serial.Deserialize(strs);

    if (d1 == *(dynamic_cast<Derived1*>(equal_d1.get())))
    {
        TEST_INT(1, true, true, "Serial + Deserial");
    }
}

void AntiTest()
{
    Serializer<Base> serial;

    Derived2 d1("Bing", 15, "Blue", 144);

    std::stringstream strs;
    serial.Serialize(strs, d1);

    std::cout << strs.str() << std::endl;
    serial.Add<Derived2>();

    std::unique_ptr<Base> equal_d1 = serial.Deserialize(strs);
    std::unique_ptr<Base> equal_d2 = serial.Deserialize(strs);

    if (equal_d2.get() == nullptr)
    {
        TEST_INT(2, true, true, "AntiTest + Exception");
    }
}
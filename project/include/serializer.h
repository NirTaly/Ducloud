/*******************************************************************************
 * File name:   serializer.h
 * Developer:   HRD07	
 * Version:		0 
 * Date:        2019-04-08 13:09:47
 * Description: 
 ******************************************************************************/

#ifndef __SERIALIZER_H__
#define __SERIALIZER_H__

#include <memory>       	/* std::unique_ptr								*/
#include <iostream>    		/* std::istream, std::ostream					*/
#include <string>    		/* std::string									*/

#include "../include/factory.h"    

namespace ilrd
{
template <typename Base>
class Serializer final
{
public:
	Serializer() = default;

	/* Should allow calling on objects of types that were not registered
		* with Add<Type>();
		*/
	void Serialize(std::ostream& stream, const Base& obj);

	/* throws BadKey when an unregistered class appears in the stream
		* throws BadCreate when Creator<Type> fails for some reason
		*/
	std::unique_ptr<Base> Deserialize(std::istream& stream);

	template <typename Derived>
	void Add();

	// Un-copyable
	Serializer(const Serializer&) = delete;
	Serializer& operator=(const Serializer&) = delete;

private:
	Factory<Base, std::string, std::istream& > m_factory; // Used by Deserialize

	template <typename Derived>
	static std::unique_ptr<Base> Creator(std::istream&);
};	

/******************************************************************************/
/******************************************************************************/

template <typename Base>
void Serializer<Base>::Serialize(std::ostream& stream,const Base& obj)
{
	stream << typeid(obj).name() << " ";
	obj.Serialize(stream);
}
/******************************************************************************/

template <typename Base>
std::unique_ptr<Base> Serializer<Base>::Deserialize(std::istream& stream)
{
	std::string type_name;
	stream >> type_name;
	
	try
	{
		return m_factory.Create(type_name, stream);
	}
	catch(const typename Factory <Base, std::string, std::istream&>::BadKey&)
	{
		return nullptr;
	}
	catch(const typename Factory <Base, std::string, std::istream&>::BadCreate& e)
	{
		return nullptr;
	}
	
}
/******************************************************************************/

template <typename Base>
template <typename Derived>
void Serializer<Base>::Add()
{
	m_factory.Add(typeid(Derived).name(), Serializer<Base>::Creator<Derived>);
}
 
} // namespace ilrd

#endif     /* __SERIALIZER_H__ */
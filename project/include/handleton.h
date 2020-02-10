/*****************************************************************************
 * File name:   handleton.h
 * Developer:   HRD07
 * Version: 	0
 * Date:        2019-03-27 10:26:28
 * Description: 
 *****************************************************************************/

#ifndef __HANDLETON_H__
#define __HANDLETON_H__

#include "singleton.h"

namespace ilrd
{

template<typename T>
class Handleton
{
public:
    Handleton();

    T& operator*();
    T* operator->();
private:
    T* m_instance;
};

template<typename T>
Handleton<T>::Handleton(): m_instance(Singleton<T>::GetInstance()) { }

template<typename T>
T& Handleton<T>::operator*() { return *m_instance; }

template<typename T>
T* Handleton<T>::operator->() { return m_instance; }
}
#endif     /* __HANDLETON_H__ */

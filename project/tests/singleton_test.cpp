#include <iostream>
#include <dlfcn.h>

#include "../../cpp/utils.h"
#include "singleton.h"
#include "header_so.h"

using namespace ilrd;

void DynamicLoad();
void StaticLoad();

int main()
{
    StaticLoad();
    // DynamicLoad();
}

void StaticLoad()
{
    int* i = Singleton<int>::GetInstance();
    *i = 6;
    std::cout << std::endl << "i = " << i << std::endl;

    // foo();
    
    *i = 5;

    std::cout << std::endl << "i = " << i << std::endl;
    // foo();
}

// void DynamicLoad()
// {
//     void* handle = dlopen ("liba.so", RTLD_LAZY);
//     if (!handle) 
//     {
//         fputs (dlerror(), stderr);
//         exit(1);
//     }

//     int* i = Singleton<int>::GetInstance();
//     int* j = Singleton<int>::GetInstance();
    
//     void (*foofunc)(void) = (void(*)(void)) dlsym(handle, "foo");

//     *i = 6;

//     std::cout << "i = " << i << std::endl;
//     std::cout << "j = " << i << std::endl;
//     foofunc();
//     std::cout << std::endl;

//     *j = 5;

//     std::cout << "i = " << i << std::endl;
//     std::cout << "j = " << i << std::endl;
//     foofunc();

//     dlclose(handle);
// }
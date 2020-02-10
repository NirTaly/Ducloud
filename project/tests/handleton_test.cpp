#include <iostream>

#include "../../cpp/utils.h"
#include "handleton.h"

using namespace ilrd;

struct X
{
    int num;
};

void Test();
int main()
{
    Test();
}


void Test()
{
    Handleton<X> h;
    h->num = 6;
    
    Handleton<X> copy;
    copy->num = 2;

    TEST_INT(1, h->num, 2, "Singleton + Opertor->");

    Handleton<int> i;

    *i = 12;
    TEST_INT(2, *i, 12, "Operator*\t");
}


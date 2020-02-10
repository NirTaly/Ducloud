/*******************************************************************************
 * File name:   serializer_test.cpp
 * Developer:   Nir Tali	
 * Version:		0 
 * Date:        2019-04-08 13:09:47
 * Description: 
 ******************************************************************************/

#include <iostream>
#include <thread>

#include "../../cpp/utils.h"
#include "wpq.h"


/******************************************************************************/

using namespace ilrd;

void SizeEmptyTests();
void PushPopTest();
void MultiThreadTest();
void PushThread(int num);
void PopThread();

int main()
{
    SizeEmptyTests();

    PushPopTest();

    MultiThreadTest();

    return 0;
}

void SizeEmptyTests()
{
    WPQ<int> pq;

    TEST_INT(1, pq.IsEmpty(), true, "IsEmpty - empty");
    TEST_INT(2, pq.Size(), 0, "Size - empty\t");

    for(int n : {1,8,5,6,3,4,0,9,7,2})
    {
        pq.Push(n);
    }

    TEST_INT(3, pq.IsEmpty(), false, "IsEmpty - unempty");
    TEST_INT(4, pq.Size(), 10, "Size - unempty\t");
}

void PushPopTest()
{
    WPQ<size_t> pq;

    for(size_t n : {1,8,5,6,3,4,0,9,7,2})
    {
        pq.Push(n);
    }

    std::vector<size_t> tops(10);
    size_t size = pq.Size();
    for (size_t i = size - 1; i <= size; i--)
    {
        pq.Pop(tops[i]);
    }

    size_t i = 0;
    for (; i < tops.size(); i++)
    {
        if (tops[i] != i)
        {
            break;
        }
    }

    TEST_INT(5, i, tops.size(), "Push Pop\t");
}

WPQ <int> pq;
void MultiThreadTest()
{
    std::thread pushes[3];
    std::thread pops[5];

    for (int n = 0; n < 3; n++)
    {
        pushes[n] = std::thread(PushThread, n);
    }

    for (int n = 0; n < 5; n++)
    {
        pops[n] = std::thread(PopThread);
    }

    for (int n = 0; n < 3; n++)
    {
        pushes[n].join();
    }

    for (int n = 0; n < 5; n++)
    {
        pops[n].join();
    }

    std::cout << std::endl;
    TEST_INT(6, pq.IsEmpty(), true, "Multithread + Timed pop");
}

void PushThread(int num)
{
    pq.Push(num);
}

void PopThread()
{
    while (1)
    {
        int num;
        std::chrono::seconds sec(3);  

        if (false == pq.Pop(num, sec))  // if reached timeout
        {
            break;
        }

        std::cout << num << " ";
        fflush(stdout);
    }
}
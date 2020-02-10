/*****************************************************************************
 * File name:   logger_test.cpp
 * Developer:   HRD07
 * Version: 	0
 * Date:        2019-04-07
 * Description: 
 *****************************************************************************/

#include "logger.h"
// #include "handleton.h"

using namespace ilrd;

int main()
{
    Handleton<Logger> log;
    Handleton<Logger> log1;
    Handleton<Logger> log2;

    log->Write("This is a Test For logger");

    log1->Write("log1 is now Writing");
    log2->Write("log2 is now Writing");
}
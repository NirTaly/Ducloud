/*****************************************************************************
 * File name:   thread_pool_test.cpp
 * Developer:   Nir Tali
 * Version: 
 * Date:        2019-04-14 10:43:22
 * Description: 
 *****************************************************************************/

#include <iostream>
#include <unistd.h>

#include "utils.h"
#include "thread_pool.h"

using namespace ilrd;

/* Class Impl */ 
class UserTask : public Task
{
public:
    UserTask(Task::Priority prio): Task(prio) { }
private:
    void Execute()  
    { 
        std::cout << "UserTask" << std::endl;
        sleep (2);
        m_num++;
    }

    int m_num;
};



/* Forward Declarations */ 


int main()
{
    ThreadPool pool(2);

    std::vector < std::shared_ptr<Task> > tasks(4);
    tasks.insert(tasks.begin(),std::make_shared<UserTask>((Task::Priority::MEDIUM)));
    tasks.insert(tasks.begin(),std::make_shared<UserTask>((Task::Priority::LOW)));
    tasks.insert(tasks.begin(),std::make_shared<UserTask>((Task::Priority::HIGH)));
    tasks.insert(tasks.begin(),std::make_shared<UserTask>((Task::Priority::VERY_LOW)));

    pool.AddTask(tasks[0]);
    pool.AddTask(tasks[1]);

    pool.SetNumOfThreads(10);

    pool.AddTask(tasks[2]);
    pool.AddTask(tasks[3]);

    std::chrono::seconds timeout(5);
    pool.Pause(timeout);
    std::cout << "Paused\n";
    // sleep(5);

    pool.AddTask(tasks[0]);
    pool.AddTask(tasks[0]);
    pool.AddTask(tasks[0]);

    pool.Resume();
    std::cout << "Resumed\n";
}

#include <iostream>
#include <unistd.h>

#include "thread_pool.h"
#include "factory.h"

using namespace ilrd;
/****************************************************************************/

class UserTask1 : public Task
{
public:
    UserTask1(Task::Priority prio): Task(prio) { }
private:
    void Execute()  
    { 
        std::cout << "Execute 1\n";
    }
};

class UserTask2 : public Task
{
public:
    UserTask2(Task::Priority prio): Task(prio) { }
private:
    void Execute()  
    { 
        std::cout << "Execute 2\n";
    }
};

class UserTask3 : public Task
{
public:
    UserTask3(Task::Priority prio): Task(prio) { }
private:
    void Execute()  
    { 
        std::cout << "Execute 3\n";
    }
};

/****************************************************************************/
/****************************************************************************/

int main()
{
    Factory<Task, std::string, Task::Priority> factory;

    factory.Add("UT1", [](Task::Priority prio) { return std::unique_ptr<Task>
                                                    ( new UserTask1(prio)); });
    factory.Add("UT2", [](Task::Priority prio) { return std::unique_ptr<Task>
                                                    ( new UserTask2(prio)); });
    factory.Add("UT3", [](Task::Priority prio) { return std::unique_ptr<Task>
                                                    ( new UserTask3(prio)); });

    ThreadPool pool(2);

    pool.Pause();

    pool.AddTask(factory.CreateShared("UT2", Task::Priority::MEDIUM));
    pool.AddTask(factory.CreateShared("UT1", Task::Priority::LOW));
    pool.AddTask(factory.CreateShared("UT3", Task::Priority::VERY_HIGH));

    pool.Resume();
    
    pool.WaitForTasksToFinish();
}
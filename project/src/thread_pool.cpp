/*****************************************************************************
 * File name:   thread_pool.cpp
 * Developer:   Nir Tali
 * Version: 
 * Date:        2019-04-14 10:43:22
 * Description: 
 *****************************************************************************/
#include <iostream>
#include <unistd.h>

#include "../include/thread_pool.h"

namespace ilrd
{
/*****************************************************************************/

class TerminateTask : public Task
{
public:
    TerminateTask(): Task(Task::Priority::SYSTEM_TERM) { }
private:
    inline void Execute()  { }
};
class PauseTask : public Task
{
public:
    PauseTask(): Task(Task::Priority::SYSTEM_PAUSE) { }
private:
    inline void Execute()  { }
};

/*****************************************************************************/
Task::~Task() { }
/*****************************************************************************/
Task::Task(Priority priority): m_priority(priority) { }

/*****************************************************************************/
Task::Priority Task::GetPriority() { return m_priority; }
/*****************************************************************************/
bool Task::operator<(const Task& other) const
{
    return (m_priority < other.m_priority);
}

/*****************************************************************************/
/*****************************************************************************/
void ThreadPool::ThreadRoutine()
{
    while (1)
    {
        std::shared_ptr<Task> task;
        
        m_task_pq.Pop(task);
        
        {
            std::unique_lock<std::mutex> lock(m_pause_mutex);
            
            // if received Pause
            if (m_pause)
            {
                m_pause_counter++;
                m_pause_cond.wait(lock);
            }
        
        }
        
        task->Execute();
        
        // if need to terminate
        if (task->m_priority == Task::Priority::SYSTEM_TERM)
        {
            break;
        }
    }
    // find current thread in pool
    std::unique_lock<std::mutex> lock(m_threads_mutex);
    for (auto th_iter = m_pool.begin(); th_iter != m_pool.end(); th_iter++)
    {
        if (th_iter->get_id() == std::this_thread::get_id())
        {
            // move to collector and erase from pool
            
            m_collector.push_back(std::move(*th_iter));

            m_pool.erase(th_iter);
            
            // notify that join() can happend in Dtor
            // m_sem.Post();
            break;
        }
    }
}
/*****************************************************************************/
ThreadPool::ThreadPool(size_t num_of_threads): 
    m_pause_counter(0), m_pause(false), m_available_threads(0)
{
    SetNumOfThreads(num_of_threads);
}

/*****************************************************************************/
ThreadPool::~ThreadPool()
{
    SetNumOfThreads(0);

    Resume();

    while (!m_pool.empty()) { }

    size_t current_size = m_collector.size();
    for (size_t i = 0; i < current_size; i++)
    {
        m_collector[i].join();
    }
}
/*****************************************************************************/
void ThreadPool::AddTask(std::shared_ptr<Task> task)
{
    // std::cerr << "AddTask: " << (void*) task.get() << std::endl;
    // fprintf(stderr, "AddTask: %p\n", (void*) task.get());
    m_task_pq.Push(task);
}

/*****************************************************************************/
void ThreadPool::SetNumOfThreads(size_t new_num_of_threads)
{
    size_t current_size = GetNumOfThreads();

    m_available_threads = new_num_of_threads;

    // need to kill some threads
    if (current_size > new_num_of_threads)
    {
        std::shared_ptr<Task> term(new TerminateTask);
        for (size_t i = 0; i < current_size - new_num_of_threads; i++)
        {
            AddTask(term);
        }
    }
    // need to create some threads
    else
    {
        std::unique_lock<std::mutex> lock(m_threads_mutex);
        for (size_t i = 0; i < new_num_of_threads - current_size; i++)
        {
            m_pool.push_back(std::thread(&ThreadPool::ThreadRoutine, this));
        }
    }
}

/*****************************************************************************/
void ThreadPool::Pause()
{
    
    m_pause = true;

    std::shared_ptr<Task> pause(new PauseTask);

    for (size_t i = 0; i < GetNumOfThreads(); i++)
    {
        AddTask(pause);
    }

    while (m_pause_counter != m_available_threads) { }
}

/*****************************************************************************/
size_t ThreadPool::GetNumOfThreads() const
{
    // std::unique_lock<std::mutex> lock(m_threads_mutex);
    // return m_pool.size();
    return m_available_threads;
}

/*****************************************************************************/
void ThreadPool::Resume()
{
    m_pause = false;
    m_pause_cond.notify_all();
    m_pause_counter = 0;
}
/*****************************************************************************/

bool ThreadPool::CompareImpl::operator()
        (const std::shared_ptr<Task>& lhs, const std::shared_ptr<Task>& rhs)
{
    return (*(rhs.get()) < *(lhs.get()));
}
/*****************************************************************************/

void ThreadPool::WaitForTasksToFinish()
{
    while (!m_task_pq.IsEmpty()) { }
}

} // namespace ilrd

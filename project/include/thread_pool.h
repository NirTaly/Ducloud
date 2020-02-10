/*****************************************************************************
 * File name:   thread_pool.h
 * Developer:   
 * Version:     1
 * Date:        2019-04-14 10:43:22
 * Description: 
 *****************************************************************************/

#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <functional>
#include <atomic>

#include "wpq.h"

namespace ilrd
{

class Task;

class ThreadPool final
{
public:    
    static const size_t DEFAULT_THREADS = 4;
    explicit ThreadPool(size_t num_of_threads = DEFAULT_THREADS);
    ~ThreadPool();
    
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(const ThreadPool&) = delete;

    // Adding Task for threads to run
    // After finish running the task get removed from the pool
    void AddTask(std::shared_ptr<Task> task);

    // Adding threads means adding free threads to the pool
    // Removing threads means that <delta> threads that finish their tasks
    // first get removed from the pool
    void SetNumOfThreads(size_t num_of_threads);
    size_t GetNumOfThreads() const;

    // Blocks for at most <timeout> putting all the threads that finish their
    // tasks into paused state (see Resume())
    // returns num of running threads
    template< class Rep, class Period >
    std::size_t Pause(std::chrono::duration<Rep, Period> const& timeout);

    // Put all the threads in the pool into paused state
    // Blocks until all the threads finish their current tasks
    void Pause();

    // Resumes all the paused threads allowing them to proceed with the work
    void Resume();

    // Proposal:
    // Blocks until all the tasks in the pool are completed
    void WaitForTasksToFinish();
private:
    struct CompareImpl
    {
        bool operator()(const std::shared_ptr<Task>& lhs, const std::shared_ptr<Task>& rhs);
    };

    WPQ<std::shared_ptr<Task>, std::vector<std::shared_ptr<Task>>, CompareImpl>
                                                                     m_task_pq;
    std::vector<std::thread>    m_pool;
    std::vector<std::thread>    m_collector;
    mutable std::mutex          m_threads_mutex;

    // Pause mechanism
    std::atomic_size_t          m_pause_counter;
    std::atomic_bool            m_pause;
    std::mutex                  m_pause_mutex;
    std::condition_variable     m_pause_cond;
    
    std::atomic_size_t          m_available_threads;

    void ThreadRoutine();
};

class Task
{
public:
    friend class ThreadPool;
    
    enum class Priority
    {
        SYSTEM_TERM = 0,
        SYSTEM_PAUSE,

        VERY_HIGH,
        HIGH,
        MEDIUM,
        LOW,
        VERY_LOW
    };

    explicit Task(Priority priority = Priority::LOW);
    virtual ~Task() = 0;

    bool operator<(const Task& other) const;
    Priority GetPriority();
    Task& operator=(const Task&) = delete;
    Task(const Task&) = delete;
private:
    virtual void Execute() = 0;
    Priority m_priority;
};

/*****************************************************************************/
template< class Rep, class Period >
std::size_t ThreadPool::Pause(std::chrono::duration<Rep, Period> const& timeout)
{
    if (m_pause == false)
    {
        m_pause_counter = 0;
        m_pause = true;
    }

    std::this_thread::sleep_for(timeout);

    m_pause = false;
    
    // still running
    return (GetNumOfThreads() - m_pause_counter);
}

} // namespace ilrd

#endif //__THREAD_POOL_H__


/*******************************************************************************
******************************** END OF FILE ***********************************
*******************************************************************************/

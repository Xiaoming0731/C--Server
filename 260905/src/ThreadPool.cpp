#include "ThreadPool.h"

ThreadPool::ThreadPool(int size):stop(false)
{
    if(size<=0)
    {
        throw std::invalid_argument("thread pool size must > 0");
    }
    for(int i=0;i<size;i++)
    {
        threads.emplace_back(std::thread([this]()
        {
            while(true)
            {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(task_mtx);
                    cv.wait(lock,[this](){return stop||!tasks.empty();});
                    if(stop&&tasks.empty()){
                        return ;
                    }
                    task=tasks.front();
                    tasks.pop();
                }
                try{
                    task();
                }catch(...){}
            }
        }));
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(task_mtx);
        stop=true;
    }
    cv.notify_all();
    for(std::thread &th:threads)
    {
        if(th.joinable())
        {
            th.join();
        }
    }
}

void ThreadPool::add(std::function<void()> func)
{
    {
        std::unique_lock<std::mutex> lock(task_mtx);
        if(stop)
        {
            throw std::runtime_error("ThreadPool already stop, can't add task any more");
        }
        tasks.emplace(std::move(func));
    }
    cv.notify_one();
}

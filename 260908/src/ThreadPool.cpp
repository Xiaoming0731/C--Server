// #include "ThreadPool.h"

// ThreadPool::ThreadPool(int size):stop(false)
// {
//     if(size<=0)
//     {
//         throw std::invalid_argument("thread pool size must > 0");
//     }
//     for(int i=0;i<size;i++)
//     {
//         threads.emplace_back(std::thread([this]()
//         {
//             while(true)
//             {
//                 std::function<void()> task;
//                 {
//                     std::unique_lock<std::mutex> lock(task_mtx);
//                     cv.wait(lock,[this](){return stop||!tasks.empty();});
//                     if(stop&&tasks.empty()){
//                         return ;
//                     }
//                     task=tasks.front();
//                     tasks.pop();
//                 }
//                 try{
//                     task();
//                 }catch(...){}
//             }
//         }));
//     }
// }

// ThreadPool::~ThreadPool()
// {
//     {
//         std::unique_lock<std::mutex> lock(task_mtx);
//         stop=true;
//     }
//     cv.notify_all();
//     for(std::thread &th:threads)
//     {
//         if(th.joinable())
//         {
//             th.join();
//         }
//     }
// }

// void ThreadPool::add(std::function<void()> func)
// {
//     {
//         std::unique_lock<std::mutex> lock(task_mtx);
//         if(stop)
//         {
//             throw std::runtime_error("ThreadPool already stop, can't add task any more");
//         }
//         tasks.emplace(std::move(func));
//     }
//     cv.notify_one();
// }




#include "ThreadPool.h"

ThreadPool::ThreadPool(int size):stop_(false)
{
    if(size <= 0)
    {
        throw std::invalid_argument("thread pool size must > 0");
    }
    for(int i = 0; i < size; i++)
    {
        threads_.emplace_back(std::thread([this]()
        {
            while(true)
            {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(task_mtx_);
                    cv_.wait(lock, [this](){ return stop_ || !tasks_.empty(); });
                    if(stop_ && tasks_.empty()){
                        return ;
                    }
                    task = std::move(tasks_.front());
                    tasks_.pop();
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
        std::unique_lock<std::mutex> lock(task_mtx_);
        stop_ = true;
    }
    cv_.notify_all();
    for(std::thread &th : threads_)
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
        std::unique_lock<std::mutex> lock(task_mtx_);
        if(stop_)
        {
            throw std::runtime_error("ThreadPool already stop, can't add task any more");
        }
        tasks_.emplace(std::move(func));
    }
    cv_.notify_one();
}

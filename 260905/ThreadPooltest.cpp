#include <iostream>
#include <string>
#include "src/ThreadPool.h"

void print(int a,double b,const char* c,std::string d)
{
    std::cout<<a<<b<<c<<d<<std::endl;
}
void test()
{
    std::cout<<"help"<<std::endl;
}

int main(int argc,char const *argv[])
{
    ThreadPool *pool=new ThreadPool();
    std::function<void()> func=std::bind(print,1,3.14,"hello",std::string("world"));
    pool->add(func);
    func=test;
    pool->add(func);

    // 测试短程序调度竞态，保留usleep仅用于测试，正式项目删掉
    // usleep(100000);

    delete pool;
    return 0;
}

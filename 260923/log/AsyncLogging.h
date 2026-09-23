#pragma once
#include <vector>
#include <memory>
#include <thread>
#include "Latch.h"
#include "Logging.h"
static const double BufferWriteTimeout = 3.0;
static const int64_t FileMaximumSize=1024*1024*1024;
class AsyncLogging{
    public:
        typedef FixedBuffer<FixedLargeBufferSize> Buffer;

        AsyncLogging(const char *filepath = nullptr);
        ~AsyncLogging();

        void Stop();
        void Start();
        
        void Append(const char *data,int len);
        void Flush();
        void ThreadFunc();
    private:
        bool running_;
        const char *filepath_;
        std::mutex mutex_;
        std::condition_variable cv_;
        Latch latch_;
        std::thread thread_;

        std::unique_ptr<Buffer> current_;
        std::unique_ptr<Buffer>next_;
        std::vector<std::unique_ptr<Buffer>> buffers_;
};
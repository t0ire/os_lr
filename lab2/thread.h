#pragma once

#include <thread>
#include <memory>
#include <stdexcept>

namespace thread {
    
using ThreadFunc = void*(*)(void*);

class Thread {
public:
    Thread(ThreadFunc func);
    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;
    Thread(Thread&& other) noexcept;
    Thread& operator=(Thread&& other) noexcept;
    
    void Join();
    void Detach();
    void Run(void* data);
    
    ~Thread();

private:
    ThreadFunc func_;
    std::unique_ptr<std::thread> thread_;
    bool is_running_ = false;
};

} 
#include <iostream>
#include <memory>

#include "thread.h"

namespace thread {

struct Wrapper {
    ThreadFunc func;
    void* data;
};

Thread::Thread(ThreadFunc func) : func_(func) {
}

void Thread::Join() {
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }
    is_running_ = false;
}

void Thread::Detach() {
    if (thread_ && thread_->joinable()) {
        thread_->detach();
    }
    is_running_ = false;
}

Thread::Thread(Thread&& other) noexcept 
    : func_(other.func_), 
      thread_(std::move(other.thread_)),
      is_running_(other.is_running_) {
    other.func_ = nullptr;
    other.is_running_ = false;
}

Thread& Thread::operator=(Thread&& other) noexcept {
    if (this != &other) {
        if (thread_ && thread_->joinable()) {
            thread_->join();
        }
        
        func_ = other.func_;
        thread_ = std::move(other.thread_);
        is_running_ = other.is_running_;
        
        other.func_ = nullptr;
        other.is_running_ = false;
    }
    return *this;
}

void Thread::Run(void* data) {
    if (is_running_) {
        throw std::runtime_error("Thread is already running");
    }
    
    // Адаптер для совместимости с pthread-style функциями
    auto adapter = [](void* arg) -> void* {
        Wrapper* wrapper = static_cast<Wrapper*>(arg);
        wrapper->func(wrapper->data);
        delete wrapper; // Очищаем память
        return nullptr;
    };
    
    Wrapper* wrapper = new Wrapper;
    wrapper->func = func_;
    wrapper->data = data;
    
    thread_ = std::make_unique<std::thread>(adapter, wrapper);
    is_running_ = true;
}

Thread::~Thread() {
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }
}

} 
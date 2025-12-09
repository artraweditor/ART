// -*- C++ -*-
//
// Adapted from https://github.com/progschj/ThreadPool
/*
Copyright (c) 2012 Jakob Progsch, Václav Zeman

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
*/

#pragma once

#include <condition_variable>
#include <functional>
#include <future>
#include <limits>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>

#include "noncopyable.h"

namespace rtengine {

class ThreadPool: public NonCopyable {
public:
    enum class Priority { LOWEST, LOW, NORMAL, HIGH, HIGHEST };

    template <class F, class... Args>
    static auto add_task(Priority p, F &&f, Args &&...args)
        -> std::future<typename std::result_of<F(Args...)>::type>;

    static void init(size_t num_workers);
    static void cleanup();

private:
    ThreadPool(size_t);
    template <class F, class... Args>
    auto enqueue(Priority p, F &&f, Args &&...args)
        -> std::future<typename std::result_of<F(Args...)>::type>;

    template <class F, class... Args>
    auto enqueue(F &&f, Args &&...args)
        -> std::future<typename std::result_of<F(Args...)>::type>
    {
        return enqueue(Priority::NORMAL, f, args...);
    }

public:
    ~ThreadPool();

private:
    // need to keep track of threads so we can join them
    std::vector<std::thread> workers_;
    // the task queue
    class Task: public std::function<void()> {
    public:
        template <class Fn>
        explicit Task(Priority p, size_t i, Fn func)
            : std::function<void()>(func), p_(p), i_(i)
        {
        }

        bool operator<(const Task &other) const
        {
            return p_ == other.p_ ? i_ > other.i_ : p_ < other.p_;
        }

    private:
        Priority p_;
        size_t i_;
    };
    std::priority_queue<Task> tasks_;

    // synchronization
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    bool stop_;
    size_t idx_;

    static std::unique_ptr<ThreadPool> instance_;
};

// the constructor just launches some amount of workers
inline ThreadPool::ThreadPool(size_t threads): stop_(false), idx_(0)
{
    for (size_t i = 0; i < threads; ++i) {
        workers_.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex_);
                    this->condition_.wait(lock, [this] {
                        return this->stop_ || !this->tasks_.empty();
                    });
                    if (this->stop_ && this->tasks_.empty()) {
                        return;
                    }
                    task = std::move(this->tasks_.top());
                    this->tasks_.pop();
                }

                task();
            }
        });
    }
}

// add new work item to the pool
template <class F, class... Args>
auto ThreadPool::enqueue(Priority p, F &&f, Args &&...args)
    -> std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);

        // don't allow enqueueing after stopping the pool
        if (stop_) {
            throw std::runtime_error("enqueue on stopped ThreadPool");
        }

        tasks_.emplace(p, idx_++, [task]() { (*task)(); });
    }
    condition_.notify_one();
    return res;
}

// the destructor joins all threads
inline ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        stop_ = true;
    }
    condition_.notify_all();
    for (std::thread &worker : workers_) {
        worker.join();
    }
}

inline void ThreadPool::init(size_t num_workers)
{
    instance_.reset(new ThreadPool(num_workers));
}

inline void ThreadPool::cleanup() { instance_.reset(nullptr); }

template <class F, class... Args>
auto ThreadPool::add_task(Priority p, F &&f, Args &&...args)
    -> std::future<typename std::result_of<F(Args...)>::type>
{
    return instance_->enqueue(p, f, args...);
}

} // namespace rtengine

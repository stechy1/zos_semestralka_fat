/*
 *         Copyright 2016 Petr Štechmüller
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 *         limitations under the License.
 */

#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include "ThreadSafeQueue.hpp"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

//http://roar11.com/2016/01/a-platform-independent-thread-pool-using-c14/
class ThreadPool {
private:
    class IThreadTask {
    public:
        IThreadTask() = default;

        virtual ~IThreadTask() = default;

        IThreadTask(const IThreadTask &rhs) = delete;

        IThreadTask &operator=(const IThreadTask &rhs) = delete;

        IThreadTask(IThreadTask &&other) = default;

        IThreadTask &operator=(IThreadTask &&other) = default;

        virtual void execute() = 0;
    };

    template<typename Func>
    class ThreadTask : public IThreadTask {
    public:
        ThreadTask(Func &&func)
                : m_func{std::move(func)} {
        }

        ~ThreadTask() override = default;

        ThreadTask(const ThreadTask &rhs) = delete;

        ThreadTask &operator=(const ThreadTask &rhs) = delete;

        ThreadTask(ThreadTask &&other) = default;

        ThreadTask &operator=(ThreadTask &&other) = default;

        /**
         * Run the task.
         */
        void execute() override {
            m_func();
        }

    private:
        Func m_func;
    };

public:
    /**
     * A wrapper around a std::future that adds the behavior of futures returned from std::async.
     * Specifically, this object will block and wait for execution to finish before going out of scope.
     */
    template<typename T>
    class TaskFuture {
    public:
        TaskFuture(std::future<T> &&future) : m_future{std::move(future)} {}

        TaskFuture(const TaskFuture &rhs) = delete;

        TaskFuture &operator=(const TaskFuture &rhs) = delete;

        TaskFuture(TaskFuture &&other) = default;

        TaskFuture &operator=(TaskFuture &&other) = default;

        ~TaskFuture() {
            if (m_future.valid()) {
                m_future.get();
            }
        }

        T get() {
            return m_future.get();
        }


    private:
        std::future<T> m_future;
    };

public:
    ThreadPool() : ThreadPool{std::max(std::thread::hardware_concurrency(), 2u) - 1u} {
    }

    explicit ThreadPool(const std::uint32_t numThreads) : m_done{false},
              m_workQueue{},
              m_threads{} {
        try {
            for (std::uint32_t i = 0u; i < numThreads; ++i) {
                m_threads.emplace_back(std::thread(&ThreadPool::worker, this));
            }
        } catch (...) {
            destroy();
            throw;
        }
    }

    ThreadPool(const ThreadPool &rhs) = delete;

    ThreadPool &operator=(const ThreadPool &rhs) = delete;

    ~ThreadPool() {
        destroy();
    }

    template<typename Func, typename... Args>
    auto submit(Func &&func, Args &&... args) {
        auto boundTask = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
        using ResultType = std::result_of_t<decltype(boundTask)()>;
        using PackagedTask = std::packaged_task<ResultType()>;
        using TaskType = ThreadTask<PackagedTask>;

        PackagedTask task{std::move(boundTask)};
        TaskFuture<ResultType> result{task.get_future()};
        m_workQueue.push(std::make_unique<TaskType>(std::move(task)));
        return result;
    }

private:
    /**
     * Constantly running function each thread uses to acquire work items from the queue.
     */
    void worker() {
        while (!m_done) {
            std::unique_ptr<IThreadTask> pTask{nullptr};
            if (m_workQueue.waitPop(pTask)) {
                pTask->execute();
            }
        }
    }

    /**
     * Invalidates the queue and joins all running threads.
     */
    void destroy() {
        m_done = true;
        m_workQueue.invalidate();
        for (auto &thread : m_threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

private:
    std::atomic_bool m_done;
    ThreadSafeQueue<std::unique_ptr<IThreadTask>> m_workQueue;
    std::vector<std::thread> m_threads;
};

namespace DefaultThreadPool {

inline ThreadPool &getThreadPool() {
    static ThreadPool defaultPool;
    return defaultPool;
}

template<typename Func, typename... Args>
inline auto submitJob(Func &&func, Args &&... args) {
    return getThreadPool().submit(std::forward<Func>(func), std::forward<Args>(args)...);
}
}

#endif
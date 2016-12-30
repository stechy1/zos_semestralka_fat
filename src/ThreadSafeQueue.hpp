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

#ifndef THREADSAFEQUEUE_HPP
#define THREADSAFEQUEUE_HPP

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>

// http://roar11.com/2016/01/a-platform-independent-thread-pool-using-c14/
template<typename T>
class ThreadSafeQueue {
public:

    ~ThreadSafeQueue() {
        invalidate();
    }

    bool tryPop(T &out) {
        std::lock_guard<std::mutex> lock{m_mutex};
        if (m_queue.empty() || !m_valid) {
            return false;
        }
        out = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    bool waitPop(T &out) {
        std::unique_lock<std::mutex> lock{m_mutex};
        m_condition.wait(lock, [this]() {
            return !m_queue.empty() || !m_valid;
        });
        /*
         * Using the condition in the predicate ensures that spurious wakeups with a valid
         * but empty queue will not proceed, so only need to check for validity before proceeding.
         */
        if (!m_valid) {
            return false;
        }
        out = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    void push(T value) {
        std::lock_guard<std::mutex> lock{m_mutex};
        m_queue.push(std::move(value));
        m_condition.notify_one();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock{m_mutex};
        return m_queue.empty();
    }

    void clear() {
        std::lock_guard<std::mutex> lock{m_mutex};
        while (!m_queue.empty()) {
            m_queue.pop();
        }
        m_condition.notify_all();
    }

    void invalidate() {
        std::lock_guard<std::mutex> lock{m_mutex};
        m_valid = false;
        m_condition.notify_all();
    }

    bool isValid() const {
        std::lock_guard<std::mutex> lock{m_mutex};
        return m_valid;
    }

private:
    std::atomic_bool m_valid{true};
    mutable std::mutex m_mutex;
    std::queue<T> m_queue;
    std::condition_variable m_condition;
};

#endif
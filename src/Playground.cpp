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

#include <iostream>

#include "ThreadPool.hpp"

uint32_t worker(uint32_t index) {
    using namespace std::chrono_literals;
    for (int i = 0; i < 10; ++i) {

        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "Thread" << index << " status: " << i << std::endl;
    }

    return index;
}

int main(int argc, char *argv[]) {

    ThreadPool threadPool(10);

    std::vector<ThreadPool::TaskFuture<uint32_t >> v;
    for (std::uint32_t i = 0u; i < 21u; ++i) {
        v.push_back(threadPool.submit(worker, i));
    }

    std::cout << "Jdu si vybrat odmenu" << std::endl;

    for (auto &item: v) {
        auto res = item.get();
        std::cout << res << std::endl;
    }

    std::cout << "Koncim..." << std::endl;

}
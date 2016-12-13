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

void worker() {
    using namespace std::chrono_literals;
    for (int i = 0; i < 100; ++i) {

        std::cout << i << std::endl;
    }
}

int main (int argc, char *argv[]) {

    ThreadPool threadPool(10);

    for (int i = 0; i < 5; ++i) {
        std::cout << "Nakladam mu praci" << std::endl;
        threadPool.submit(worker);
    }
}
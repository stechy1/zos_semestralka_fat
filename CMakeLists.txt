cmake_minimum_required(VERSION 3.0)
project(zos_semestralka_fat)

set(CMAKE_CXX_FLAGS "-std=c++14 -pthread")

set(SOURCE_FILES src/Fat.hpp src/main.cpp src/Fat.cpp src/Defragmenter.cpp src/Defragmenter.hpp src/ThreadSafeQueue.hpp src/ThreadPool.hpp)

add_executable(zos_semestralka_fat ${SOURCE_FILES})
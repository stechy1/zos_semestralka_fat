cmake_minimum_required(VERSION 3.6)
project(zos_semestralka_fat)

set(CMAKE_CXX_STANDARD 14)

set(SOURCE_FILES src/Fat.hpp src/main.cpp src/Fat.cpp)

add_executable(zos_semestralka_fat ${SOURCE_FILES})
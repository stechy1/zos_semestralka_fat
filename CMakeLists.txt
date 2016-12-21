cmake_minimum_required(VERSION 3.0)
project(zos_semestralka_fat)

set(CMAKE_CXX_FLAGS "-std=c++14 -lncurses")

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

set(SOURCE_FILES src/Playground.cpp)

add_executable(zos_semestralka_fat ${SOURCE_FILES})
target_link_libraries(zos_semestralka_fat ${CURSES_LIBRARIES})
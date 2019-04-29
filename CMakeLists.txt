cmake_minimum_required(VERSION 3.13)
project(HW2)

set(CMAKE_CXX_STANDARD 14)

add_executable(HW2
        cacheSim.cpp CacheInterface.h CacheInterface.cpp)

cmake_minimum_required(VERSION 3.31)
project(SoupReaper)

set(CMAKE_CXX_STANDARD 20)

add_executable(SoupReaper main.cpp
        MemoryTools.h
        MemoryTools.cpp
        Input.h
        Input.cpp
        MemoryManipulation.h
)

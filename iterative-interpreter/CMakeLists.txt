cmake_minimum_required(VERSION 3.26)
project(iterative_interpreter)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-fstack-protector-all -m32 -O3")

include_directories(include)
add_executable(iterative_interpreter src/main.cpp
        src/byterun.c
        src/runtime.c
        src/gc.c
        src/iterative_interpreter.cpp)

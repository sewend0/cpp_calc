cmake_minimum_required(VERSION 3.29)
project(calculator)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXE_LINKER_FLAGS "-static")

add_executable(calculator main.cpp)

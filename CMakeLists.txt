cmake_minimum_required(VERSION 3.21)
project(safe_IO)

set(CMAKE_CXX_STANDARD 14)

add_executable(safe_IO main.cpp safe_IO.cpp safe_IO.h)
target_link_libraries(safe_IO pthread)

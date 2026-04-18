cmake_minimum_required(VERSION 3.21)
project(test_package CXX)

find_package(argsbarg CONFIG REQUIRED)

add_executable(example example.cpp)
target_link_libraries(example PRIVATE argsbarg::argsbarg)

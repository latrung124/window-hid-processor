cmake_minimum_required(VERSION 3.0)

project(window-hid-processor VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(window-hid-processor main.cpp)

#target link libraries
target_link_libraries(window-hid-processor PRIVATE setupapi hid)
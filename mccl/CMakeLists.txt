cmake_minimum_required(VERSION 3.10)

project(mccl VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(mccl main.cpp)

target_link_libraries(mccl event_core btpro btdef)

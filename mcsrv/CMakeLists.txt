cmake_minimum_required(VERSION 3.10)

project(mcsrv VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(mcsrv main.cpp)

target_link_libraries(mcsrv event_core btpro btdef)

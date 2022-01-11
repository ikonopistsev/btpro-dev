cmake_minimum_required(VERSION 3.7)

project(evheap VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (WIN32)
  add_definitions(-DNOMINMAX -DWIN32_LEAN_AND_MEAN)
endif()

include_directories(evheap ../)

add_executable(evheap main.cpp)

if (WIN32)
  target_link_libraries(evheap event event_threads)
else()
  target_link_libraries(evheap event event_pthreads pthread)
endif()
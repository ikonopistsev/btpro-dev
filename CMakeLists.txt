cmake_minimum_required(VERSION 3.10)

project(btpro VERSION 1.0.1)


include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

if (WIN32)
  add_subdirectory(libevent)
  add_definitions(-DNOMINMAX -DWIN32_LEAN_AND_MEAN)
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/libevent/include")
# fast fix for windows, build dir may be wrong
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/build/libevent/include")
endif()

add_subdirectory(btpro)
add_subdirectory(evheap)


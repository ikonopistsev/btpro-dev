cmake_minimum_required(VERSION 3.10)

project(btpro VERSION 1.0.1)

if (WIN32)
  # on windwos build static libevent
  add_subdirectory(libevent)
  add_definitions(-DNOMINMAX -DWIN32_LEAN_AND_MEAN)
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/libevent/include")
  # fast fix for windows, build dir may be wrong, i use vscode
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/build/libevent/include")
endif()

if (APPLE)
  # brew install libevent
  # my homebrew path's 
  include_directories("/usr/local/include")
  link_directories("/usr/local/lib")
endif()

add_subdirectory(btpro)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
add_subdirectory(evheap)
add_subdirectory(mcsrv)
add_subdirectory(mccl)


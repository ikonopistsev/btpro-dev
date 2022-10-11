cmake_minimum_required(VERSION 3.10)

project(btpro VERSION 1.0.1)

if (WIN32)
  # on windwos build static libevent
  set(EVENT__LIBRARY_TYPE "STATIC" CACHE STRING "libevent static")
  set(EVENT__DISABLE_OPENSSL ON CACHE BOOL "libevent no openssl")
  set(EVENT__DISABLE_DEBUG_MODE ON CACHE BOOL "libevent no debug")
  set(EVENT__DISABLE_BENCHMARK ON CACHE BOOL "libevent no benchmark")
  set(EVENT__DISABLE_MBEDTLS ON CACHE BOOL "libevent no mbedtls")
  set(EVENT__DISABLE_TESTS ON CACHE BOOL "libevent no tests")
  set(EVENT__DISABLE_SAMPLES ON CACHE BOOL "libevent no samples")
  set(EVENT__DISABLE_REGRESS ON CACHE BOOL "libevent no regress")
  set(EVENT__DOXYGEN OFF CACHE BOOL "libevent no doxygen")
  add_subdirectory(libevent)
  add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX)
endif()

if (APPLE)
  # brew install libevent
  # my homebrew path's 
  include_directories("/usr/local/include")
  link_directories("/usr/local/lib")
endif()

add_subdirectory(btdef)
add_subdirectory(btpro)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
add_subdirectory(evheap)
add_subdirectory(mcsrv)
add_subdirectory(mccl)


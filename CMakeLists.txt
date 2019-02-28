cmake_minimum_required(VERSION 3.10)
project(server LANGUAGES C CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)

# If system has ccache - use it
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

## GCC 7.x brinds implicit-fallthrough warning which is fails in various places in rocksdb and
## zstd. Turn it off for now
if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 7.0)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wimplicit-fallthrough=0")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wimplicit-fallthrough=0")
endif()

## Libuv
add_subdirectory(third-party/libuv-1.11.0)

## Logging
add_subdirectory(third-party/spdlog-1.3.1)

set(CMAKE_CXX_STANDARD 11)

## Build services
add_subdirectory(src)

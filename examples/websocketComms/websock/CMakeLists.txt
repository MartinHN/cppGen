cmake_minimum_required(VERSION 3.1)
project(websowk)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -Wall -Wpedantic ") # -ftime-trace
# for small builds
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -O3 -fno-rtti -fno-exceptions")
# wasm
set(CMAKE_VERBOSE_MAKEFILE on)

add_executable(websowk main.cpp)
target_include_directories(websowk PUBLIC websocketpp  /opt/homebrew/Cellar/asio/1.26.0/include)

cmake_minimum_required(VERSION 3.1)
project(test)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic ") # -ftime-trace

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 ") # -ftime-trace

# for small builds
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -O3 -fno-rtti -fno-exceptions")

# wasm
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lembind")
set(CMAKE_EXECUTABLE_SUFFIX ".mjs")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -s EXPORT_ES6=1 -s WASM_BIGINT ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -DEMSCRIPTEN_HAS_UNBOUND_TYPE_NAMES=0")

set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build.emscripten)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  --target=wasm32 --no-standard-libraries")
# SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} --export-all --no-entry")
add_executable(test main.cpp)

cmake_minimum_required(VERSION 3.14)

project(message_tst LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

# set(CMAKE_CXX_COMPILER /opt/homebrew/Cellar/llvm/16.0.3/bin/clang++)
set(CMAKE_CXX_COMPILER gcc-13)

# ---- Declare executable ----
add_executable(message_tst main.cpp)
set_property(TARGET message_tst PROPERTY OUTPUT_NAME message_tst)

add_custom_command(TARGET message_tst PRE_BUILD COMMAND "${CMAKE_CXX_COMPILER}" ARGS "--version")

# features
target_compile_features(message_tst PRIVATE cxx_std_20)
target_compile_options(message_tst PUBLIC -O0 -g -Wall -Werror -Wpedantic -Wuninitialized)
target_compile_options(message_tst PUBLIC -Wno-gnu-zero-variadic-macro-arguments) # for simple test

# clang
# target_compile_options(message_tst PRIVATE -fsanitize=undefined,address -fsanitize-address-use-after-return=always -fno-omit-frame-pointer)
# target_link_options(message_tst PRIVATE -fsanitize=undefined,address)
# target_compile_options(message_tst PUBLIC -Weffc++)

# gcc
target_link_options(message_tst PUBLIC -lm -lstdc++)

# target_compile_options(message_tst PRIVATE -fsanitize=leak -fsanitize-address-use-after-scope -fno-omit-frame-pointer)
# target_link_options(message_tst PRIVATE -fsanitize=leak)

# includes
include_directories(message_tst PRIVATE ./gen/cpp/)

# include_directories(srv_exe PRIVATE ../gen/cpp/common/lib/wsServer/websocketpp /opt/homebrew/Cellar/asio/1.26.0/include)

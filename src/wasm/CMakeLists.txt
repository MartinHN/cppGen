cmake_minimum_required(VERSION 3.1)
project(wasmJsProject)
set(CMAKE_CXX_STANDARD 20)
add_compile_options(-Wall -Wpedantic) # -ftime-trace

option(BUILD_TYPESCRIPT "Build the typedefs")
option(ZIP_ASSETS "zip produced assets" FALSE)

# add_compile_options( -g -O0 ) # -ftime-trace

# wasm
add_link_options(-lembind)
add_link_options(-sMODULARIZE=1 --bind)
add_link_options(-sWASM_BIGINT)
add_link_options(-sEXPORT_NAME=createWasmModule)
add_link_options(-lwebsocket.js)

add_link_options(-sEXPORTED_RUNTIME_METHODS=stringToNewUTF8)

# add_link_options(--export-all --no-entry)

# for small builds
add_link_options(-fno-exceptions -fno-inline) # -fno-rtti
add_compile_options(-fno-exceptions -fno-inline) # -fno-rtti

# add_link_options(-fno-rtti)
# add_compile_options(-fno-rtti)
# add_compile_options(-DEMSCRIPTEN_HAS_UNBOUND_TYPE_NAMES=0)
set(CMAKE_VERBOSE_MAKEFILE on)

if(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    message(FATAL_ERROR "You must set CMAKE_RUNTIME_OUTPUT_DIRECTORY environment variable")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build_emscripten)
else()
endif()

if(NOT DEFINED GEN_DIR)
    message(FATAL_ERROR "You must set GEN_DIR environment variable to where uapi produced cpps")
else()
    message("using gen dir = " ${GEN_DIR})
endif()

if(NOT DEFINED ROOT_JS_CLASS)
    message(FATAL_ERROR "You must set ROOT_JS_CLASS environment variable main js class")
else()
    message("using ROOT_JS_CLASS = " ${ROOT_JS_CLASS})
    add_compile_definitions("ROOT_JS_CLASS=${ROOT_JS_CLASS}")
endif()

# add_compile_options(--target=wasm32 --no-standard-libraries)
set(CMAKE_EXECUTABLE_SUFFIX ".js")
add_executable(wasmJs main.cpp)
include_directories(${GEN_DIR}) # ../gen/cpp/)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    message("release mode")
    add_link_options(-flto)
    add_compile_options(-flto)
    target_compile_options(wasmJs PUBLIC -Oz --closure=2)
else()
    message("debug mode")
    target_compile_options(wasmJs PUBLIC -O0 --closure=0)

    # MAYBEEEEE >>>>>>>># target_link_options(wasmJs PUBLIC -fdebug-compilation-dir=../..)
    # prevent dwarf debugging...
    # target_link_options(wasmJs PUBLIC -gsource-map) # --sourcemap-base="loloooooo"

    # add_custom_command(TARGET wasmJs POST_BUILD COMMAND "node" ARGS "${CMAKE_CURRENT_SOURCE_DIR}/scripts/toAbsMap.js")
    # target_link_options(wasmJs PUBLIC -gseparate-dwarf=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/wasmJs.wasm.dwp)

    # add_custom_command(TARGET wasmJs POST_BUILD COMMAND "mv" ARGS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/wasmJs.wasm" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/wasmJs.good.wasm")
    # add_custom_command(TARGET wasmJs POST_BUILD COMMAND "mv" ARGS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/wasmJs.debug.wasm" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/wasmJs.wasm")

    # add_custom_command(TARGET wasmJs POST_BUILD COMMAND "llvm-dwarfdump" ARGS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/wasmJs.wasm" ">" "/tmp/wasmJs.dwarf")
    # set(sourceMapPy "../vendor/emscripten/tools/wasm-sourcemap.py" )
    # " $< -w $@ -p $(CURDIR) -s -u ./$(@:.wasm=.wasm.map) -o $(@:.wasm=.wasm.map) --dwarfdump-output=$(@:.wasm=.wasm.dwarf)
    # ")
    # add_custom_command(TARGET wasmJs POST_BUILD COMMAND sourceMapPy ARGS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/wasmJs.wasm" ">" "/tmp/wasmJs.dwarf")
endif()

# module are not supported by tsembind
target_link_options(wasmJs PUBLIC -sEXPORT_ES6=1)

if(BUILD_FOR_NODE)
    target_link_options(wasmJs PUBLIC -sENVIRONMENT=node)
else()
    target_link_options(wasmJs PUBLIC -sENVIRONMENT=web)
endif()

add_custom_command(TARGET wasmJs PRE_BUILD COMMAND "rm" ARGS "-r" ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
add_custom_command(TARGET wasmJs PRE_BUILD COMMAND "mkdir" ARGS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

if(ZIP_ASSETS)
    add_custom_command(TARGET wasmJs POST_BUILD COMMAND "gzip" ARGS "-k" ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/wasmJs.wasm)
    add_custom_command(TARGET wasmJs POST_BUILD COMMAND "gzip" ARGS "-k" ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/wasmJs.js)
endif()

if(BUILD_TYPESCRIPT)
    add_executable(wasmTs main.cpp)
    target_compile_options(wasmTs PUBLIC -O0 -DTYPESCRIPT_DEF=1)
    target_link_options(wasmTs PUBLIC -sHEADLESS=1)

    # hack to prevent being considered as module if inside another node directory
    add_custom_command(TARGET wasmTs POST_BUILD COMMAND "cp" ARGS "${CMAKE_CURRENT_SOURCE_DIR}/package.json" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/")
    add_custom_command(TARGET wasmTs POST_BUILD COMMAND "tsembind" ARGS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/wasmTs.js" ">" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/wasmJsTypes.d.ts") # tsembind wasm.js > types.d.ts
    add_custom_command(TARGET wasmTs POST_BUILD COMMAND "rm" ARGS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/package.json")
    add_custom_command(TARGET wasmTs POST_BUILD COMMAND "rm" ARGS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/wasmTs.js")
    add_custom_command(TARGET wasmTs POST_BUILD COMMAND "rm" ARGS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/wasmTs.wasm")
    add_custom_command(TARGET wasmTs POST_BUILD COMMAND "cp" ARGS "${CMAKE_CURRENT_SOURCE_DIR}/proxyHelpers.ts" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/")
endif()

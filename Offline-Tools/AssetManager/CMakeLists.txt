project("DOG_Offline_AssetManager")

cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_STANDARD_REQUIRED ON)

set(SRC "src")
set(INC "include")
set(LIB "lib")
set(BIN "bin")
set(DEPS "${SRC}/main.cpp")

add_executable(AssetManager ${DEPS})
target_link_directories(AssetManager PRIVATE ${LIB})
target_include_directories(AssetManager PRIVATE ${INC})

set_target_properties(AssetManager PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${BIN})

target_precompile_headers(AssetManager PRIVATE "${INC}/pch.h")


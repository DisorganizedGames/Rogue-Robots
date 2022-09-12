#ROOT

cmake_minimum_required(VERSION "3.20.0")
if(${CMAKE_VERSION} VERSION_LESS "3.20.0")
message(FATAL_ERROR "Cmake version is outdated.")
endif()

if (NOT WIN32)
message(FATAL_ERROR "Only Windows platform is supported.")
endif()

project("Rogue-Robots")

set(CMAKE_CXX_STANDARD "20")
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

add_subdirectory("DOGEngine")
add_subdirectory("Runtime")




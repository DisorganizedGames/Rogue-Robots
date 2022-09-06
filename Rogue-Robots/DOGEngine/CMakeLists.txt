#Root/DOGEngine
set(SourceFiles "EntryPoint.h" "DOGEngine.h" "src/pch.h" "src/Application.h" "src/Application.cpp" "src/Physics/Physics.h" "src/common/Utils.h" "src/Audio/AudioDevice.h" "src/Audio/AudioDevice.cpp")
set(LibraryName "DOGEngine")

add_library("${LibraryName}" STATIC "${SourceFiles}")
target_include_directories("${LibraryName}" PRIVATE "${CMAKE_SOURCE_DIR}/Core/src/")
target_compile_options("${LibraryName}" PRIVATE "/W4")
set_target_properties("${LibraryName}" PROPERTIES LINKER_LANGUAGE "CXX")
target_link_libraries("${LibraryName}" PRIVATE "xaudio2" "ole32")

target_precompile_headers("${LibraryName}" PUBLIC "src/pch.h")
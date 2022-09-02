#Root/Runtime
set(SourceFiles "src/Core/RuntimeApp.h" "src/Core/RuntimeApp.cpp"  )
set(ExecutableName "Runtime")

add_executable("${ExecutableName}" "${SourceFiles}")

target_include_directories("${ExecutableName}" PRIVATE "src/")
target_include_directories("${ExecutableName}" PRIVATE "${CMAKE_SOURCE_DIR}/DOGEngine/")

target_link_libraries("${ExecutableName}" PRIVATE "DOGEngine")
target_compile_options("${ExecutableName}" PRIVATE "/W4")
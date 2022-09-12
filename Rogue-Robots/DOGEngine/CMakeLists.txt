#Root/DOGEngine
set(SourceFiles "EntryPoint.h" "DOGEngine.h" "src/pch.h" "src/Core/Application.h" "src/Core/Application.cpp" "src/Core/Window.h" "src/Core/Window.cpp" 
	"src/common/Utils.h" "src/Audio/AudioDevice.h" "src/Audio/AudioDevice.cpp" 
	"src/Audio/SourceVoice.h" "src/Audio/SourceVoice.cpp" "src/Audio/AudioFileReader.h"  "src/Audio/SourceVoiceCallback.h" "src/Input/Mouse.h" "src/Input/Mouse.cpp" "src/Input/Keyboard.h" "src/Input/Keyboard.cpp"
    "src/Core/AssetManager.h" "src/Core/AssetManager.cpp" "src/Core/AssimpImporter.h" "src/Core/AssimpImporter.cpp" "src/Core/DOGEngineTypes.h" "src/Core/DOGEngineTypes.cpp" "src/Core/MaterialManager.h" "src/Core/MaterialManager.cpp" "src/EventSystem/Layer.h" "src/EventSystem/LayerStack.h" "src/EventSystem/LayerStack.cpp")

set(LibraryName "DOGEngine")
set (ExternalIncludePath "${CMAKE_SOURCE_DIR}/DOGEngine/vendor/includes/")
set (ExternalLibPath "${CMAKE_SOURCE_DIR}/DOGEngine/vendor/libs")

add_library("${LibraryName}" STATIC "${SourceFiles}")
target_include_directories("${LibraryName}" PRIVATE "${CMAKE_SOURCE_DIR}/Core/src/" "${ExternalIncludePath}")
target_compile_options("${LibraryName}" PRIVATE "/W4")
set_target_properties("${LibraryName}" PROPERTIES LINKER_LANGUAGE "CXX")
target_link_libraries("${LibraryName}" PRIVATE "xaudio2" "ole32")


target_precompile_headers("${LibraryName}" PUBLIC "src/pch.h")

##### Section for linking external libraries #####
file(GLOB_RECURSE libFiles
      ${ExternalLibPath}/${CMAKE_BUILD_TYPE}/*.lib)

foreach(var ${libFiles})
    target_link_libraries("${LibraryName}" PRIVATE "${var}")
endforeach()

file(GLOB_RECURSE dlls
      ${ExternalLibPath}/${CMAKE_BUILD_TYPE}/*.dll ${ExternalLibPath}/${CMAKE_BUILD_TYPE}/*.pdb)

foreach(var ${dlls})
    add_custom_command(
    TARGET "${LibraryName}" POST_BUILD 
    COMMAND ${CMAKE_COMMAND} -E copy 
    "${var}"
    ${CMAKE_BINARY_DIR}/Runtime/

    )
endforeach()

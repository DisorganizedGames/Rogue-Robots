#Root/DOGEngine
set(SourceFiles "EntryPoint.h" "DOGEngine.h" "src/pch.h" "src/Core/Application.h" "src/Core/Application.cpp" "src/Core/Window.h" "src/Core/Window.cpp" 
	"src/common/Utils.h" "src/Audio/AudioDevice.h" "src/Audio/AudioDevice.cpp" 
	"src/Audio/SourceVoice.h" "src/Audio/SourceVoice.cpp" "src/Audio/AudioFileReader.h"  "src/Audio/SourceVoiceCallback.h" "src/Input/Mouse.h" "src/Input/Mouse.cpp" "src/Input/Keyboard.h" "src/Input/Keyboard.cpp"
    "src/Core/AssetManager.h" "src/Core/AssetManager.cpp" "src/Core/AssimpImporter.h" "src/Core/AssimpImporter.cpp" "src/Core/DOGEngineTypes.h" "src/Core/DOGEngineTypes.cpp" "src/Core/MaterialManager.h" "src/Core/MaterialManager.cpp" 
	"src/EventSystem/Layer.h" "src/EventSystem/LayerStack.h" "src/EventSystem/LayerStack.cpp" "src/EventSystem/IEvent.h" "src/EventSystem/WindowEvents.h" 
	"src/EventSystem/EventPublisher.h"  "src/EventSystem/EventBus.h" "src/EventSystem/EventBus.cpp" "src/Core/CoreUtils.h" "src/EventSystem/MouseEvents.h" 
	"src/EventSystem/KeyboardEvents.h" "src/EventSystem/EventPublisher.cpp" "src/Network/Server.h" "src/Network/Server.cpp" "src/Network/Client.h" "src/Network/Client.cpp" 
    "src/Core/Time.h"
	"src/Audio/AudioManager.h" "src/Audio/AudioManager.cpp" "src/Audio/AudioPlayerComponent.h" "src/common/ErrorTypes.h"
    "src/Core/Types/MeshTypes.h" "src/Core/Types/MaterialTypes.h" "src/Core/Types/AssetTypes.h" "src/Graphics/Rendering/MaterialTable.h" "src/Graphics/Rendering/MaterialTable.cpp"
    "src/Core/AssetManager.h" "src/Core/AssetManager.cpp" "src/Core/AssimpImporter.h" "src/Core/AssimpImporter.cpp" "src/Core/DOGEngineTypes.h" "src/Core/DOGEngineTypes.cpp"
    "src/Core/MaterialManager.h" "src/Core/MaterialManager.cpp"
    "src/Graphics/Rendering/MaterialTable.h"  "src/Core/TextureFileImporter.h" "src/Core/TextureFileImporter.cpp" 
    "src/Graphics/Rendering/MaterialTable.cpp" "src/Graphics/Rendering/MaterialTable.h" "src/Graphics/Rendering/MaterialTable.cpp"   
    "src/Graphics/Rendering/TextureManager.h" "src/Graphics/Rendering/TextureManager.cpp" "src/Graphics/Rendering/Renderer.h" "src/Graphics/Rendering/Renderer.cpp")

	file(GLOB_RECURSE TempGraphics
    "src/Graphics/*.cpp")

set(LibraryName "DOGEngine")
set (ExternalIncludePath "${CMAKE_SOURCE_DIR}/DOGEngine/vendor/includes/")
set (ExternalLibPath "${CMAKE_SOURCE_DIR}/DOGEngine/vendor/libs")

add_library("${LibraryName}" STATIC "${SourceFiles}" "${TempGraphics}"   )
target_include_directories("${LibraryName}" PRIVATE "${CMAKE_SOURCE_DIR}/Core/src/" "${ExternalIncludePath}")
target_compile_options("${LibraryName}" PRIVATE "/W4")
set_target_properties("${LibraryName}" PROPERTIES LINKER_LANGUAGE "CXX")
target_link_libraries("${LibraryName}" PRIVATE "xaudio2" "ole32" "d3d12" "dxgi" "dxguid" "dxcompiler" "Ws2_32" "winmm.lib")

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


file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/Runtime/D3D12")


add_custom_command(
    TARGET ${LibraryName} POST_BUILD 
    COMMAND ${CMAKE_COMMAND} -E copy 
    "${CMAKE_BINARY_DIR}/Runtime/D3D12Core.dll"
    "${CMAKE_BINARY_DIR}/Runtime/D3D12/"
)

add_custom_command(
    TARGET ${LibraryName} POST_BUILD 
    COMMAND ${CMAKE_COMMAND} -E copy 
    "${CMAKE_BINARY_DIR}/Runtime/d3d12SDKLayers.dll"
    "${CMAKE_BINARY_DIR}/Runtime/D3D12/"
)

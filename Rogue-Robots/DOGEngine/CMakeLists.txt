#Root/DOGEngine
set(SourceFiles "EntryPoint.h" "DOGEngine.h" "src/pch.h" "src/Core/Application.h" "src/Core/Application.cpp" "src/Core/Window.h" "src/Core/Window.cpp" 
	"src/common/Utils.h"
	"src/Input/Mouse.h" "src/Input/Mouse.cpp" "src/Input/Keyboard.h" "src/Input/Keyboard.cpp"
    "src/Core/AssetManager.h" "src/Core/AssetManager.cpp" "src/Core/AssimpImporter.h" "src/Core/AssimpImporter.cpp" "src/Core/DOGEngineTypes.h" "src/Core/DOGEngineTypes.cpp" "src/Core/MaterialManager.h" "src/Core/MaterialManager.cpp" 
	"src/EventSystem/Layer.h" "src/EventSystem/LayerStack.h" "src/EventSystem/LayerStack.cpp" "src/EventSystem/IEvent.h" "src/EventSystem/WindowEvents.h" 
	"src/EventSystem/EventPublisher.h"  "src/EventSystem/EventBus.h" "src/EventSystem/EventBus.cpp" "src/Core/CoreUtils.h" "src/EventSystem/MouseEvents.h" 
	"src/EventSystem/KeyboardEvents.h" "src/EventSystem/EventPublisher.cpp"  
    "src/Core/Time.h"
	"src/Audio/Audio.h" "src/Audio/Audio.cpp" "src/Audio/AudioManager.cpp" "src/Audio/AudioManager.h" "src/common/ErrorTypes.h"
    "src/Core/Types/GraphicsTypes.h"  "src/Core/Types/AssetTypes.h" "src/Graphics/Rendering/MaterialTable.h" "src/Graphics/Rendering/MaterialTable.cpp"
    "src/Core/AssetManager.h" "src/Core/AssetManager.cpp" "src/Core/AssimpImporter.h" "src/Core/AssimpImporter.cpp" "src/Core/DOGEngineTypes.h" "src/Core/DOGEngineTypes.cpp"
    "src/Core/MaterialManager.h" "src/Core/MaterialManager.cpp"
    "src/Graphics/Rendering/MaterialTable.h"  "src/Core/TextureFileImporter.h" "src/Core/TextureFileImporter.cpp" 
    "src/Graphics/Rendering/MaterialTable.cpp" "src/Graphics/Rendering/MaterialTable.h" "src/Graphics/Rendering/MaterialTable.cpp"   
    "src/Graphics/Rendering/TextureManager.h" "src/Graphics/Rendering/TextureManager.cpp" "src/Graphics/Rendering/Renderer.h" "src/Graphics/Rendering/Renderer.cpp" 
      "src/Graphics/RHI/DX12/ImGUIBackend_DX12.h" "src/Graphics/RHI/DX12/ImGUIBackend_DX12.cpp" "src/Graphics/RHI/ImGUIBackend.h" "src/Graphics/RHI/D2DBackend.h" "src/Graphics/RHI/DX12/D2DBackend_DX12.h"
    "src/Graphics/RHI/DX12/D2DBackend_DX12.cpp" "src/Graphics/Rendering/UI.h" "src/Graphics/Rendering/UI.cpp" "src/Graphics/Rendering/GraphicsBuilder.h" "src/Graphics/Rendering/GraphicsBuilder.cpp"
    "src/ECS/EntityManager.h" "src/ECS/EntityManager.cpp" "src/ECS/Component.h" "src/ECS/Component.cpp" 
    "src/Scripting/LuaW.h" "src/Scripting/LuaW.cpp" "src/Scripting/LuaTable.h" "src/Scripting/LuaTable.cpp" "src/Scripting/LuaGlobal.h" "src/Scripting/LuaGlobal.cpp" "src/Scripting/LuaEvent.h" 
    "src/Scripting/LuaEvent.cpp" "src/Scripting/ScriptManager.h" "src/Scripting/ScriptManager.cpp" "src/Scripting/LuaContext.h" "src/Scripting/LuaContext.cpp" "src/Scripting/LuaMain.h" "src/Scripting/LuaMain.cpp"
    "src/Core/ManagedAssets.h" "src/Core/ManagedAssets.cpp"
    "src/Graphics/Rendering/RenderGraph/RenderGraph.h" "src/Graphics/Rendering/RenderGraph/RenderGraph.cpp" "src/Graphics/Rendering/RenderGraph/RGTypes.h" 
    "src/Graphics/Rendering/RenderGraph/RGResourceManager.h" "src/Graphics/Rendering/RenderGraph/RGResourceManager.cpp"
    "src/Core/AnimationManager.h" "src/Core/AnimationManager.cpp" "src/Core/ImGuiMenuLayer.h" "src/Core/ImGuiMenuLayer.cpp"
    "src/Physics/PhysicsEngine.h" "src/Physics/PhysicsEngine.cpp"
    "src/Core/ShapeCreator.h" "src/Core/ShapeCreator.cpp"
    "src/Graphics/Rendering/RenderGraph/RGBlackboard.h" "src/Graphics/Rendering/RenderEffects/ImGUIEffect.h" 
    "src/Graphics/Rendering/RenderEffects/ImGUIEffect.cpp" "src/Graphics/Rendering/RenderEffects/EffectData/GlobalEffectData.h"  
    "src/Graphics/Rendering/RenderEffects/TestComputeEffect.h" "src/Graphics/Rendering/RenderEffects/TestComputeEffect.cpp"    
    "src/Graphics/Rendering/RenderEffects/RenderEffect.h" "src/Graphics/Rendering/RenderEffects/RECommonIncludes.h"
    "src/ECS/System.h" "src/Graphics/RHI/Types/HardwareTypes.h"
    "src/Physics/PhysicsRigidbody.h" "src/Physics/PhysicsRigidbody.cpp"
    "src/Graphics/Rendering/LightTable.h" "src/Graphics/Rendering/LightTable.cpp"
    "src/Core/LightManager.h" "src/Core/LightManager.cpp" "src/common/MiniProfiler.h" "src/common/MiniProfiler.cpp"
    "src/Graphics/Rendering/RenderEffects/Bloom.h" "src/Graphics/Rendering/RenderEffects/Bloom.cpp"
     "src/Core/CustomMaterialManager.h" "src/Core/CustomMaterialManager.cpp" "src/Core/CustomMeshManager.h" "src/Core/CustomMeshManager.cpp")

	file(GLOB_RECURSE TempGraphics
    "src/Graphics/*.cpp")

add_compile_definitions(TRACY_ENABLE=1)

# Keep this commented this to fix memory leak
# Profiler leaks memory if to be used properly
# If you want to use tracy, uncomment this
#add_compile_definitions(PROFILER_ON)

set(LibraryName "DOGEngine")
set (ExternalIncludePath "${CMAKE_SOURCE_DIR}/DOGEngine/vendor/includes/")
set (ExternalLibPath "${CMAKE_SOURCE_DIR}/DOGEngine/vendor/libs")

add_library("${LibraryName}" STATIC "${SourceFiles}" "${TempGraphics}"   )
target_include_directories("${LibraryName}" PRIVATE "${CMAKE_SOURCE_DIR}/Core/src/" "${ExternalIncludePath}")
target_compile_options("${LibraryName}" PRIVATE "/W4")
set_target_properties("${LibraryName}" PROPERTIES LINKER_LANGUAGE "CXX")
target_link_libraries("${LibraryName}" PRIVATE "xaudio2" "ole32" "d3d12" "d3d11" "D2d1" "Dwrite" "dxgi" "dxguid" "dxcompiler" "Ws2_32" "winmm.lib")

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
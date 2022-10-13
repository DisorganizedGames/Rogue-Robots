#Root/Runtime
set(SourceFiles 
		"src/Core/RuntimeApplication.cpp" "src/Core/RuntimeApplication.h"
		"src/Game/GameLayer.h" "src/Game/GameLayer.cpp" 
		"src/Game/EmilFDebugLayer.h" "src/Game/EmilFDebugLayer.cpp"
		"src/Game/MainPlayer.h" "src/Game/MainPlayer.cpp" 
		"src/Game/NetCode.h" "src/Game/NetCode.cpp"
		"src/Game/MainPlayer.h" "src/Game/MainPlayer.cpp"
		"src/Pathfinder/Pathfinder.h" "src/Pathfinder/Pathfinder.cpp"
		"src/Game/LuaInterfaces.h" "src/Game/LuaInterfaces.cpp"
		 "src/Game/Agent.h" "src/Game/Agent.cpp"
		"src/Network/Server.h" "src/Network/Server.cpp" "src/Network/Client.h" "src/Network/Client.cpp")
		
set(ExecutableName "Runtime")
set (ExternalIncludePath "${CMAKE_SOURCE_DIR}/Runtime/vendor/includes/")
set (ExternalLibPath "${CMAKE_SOURCE_DIR}/Runtime/vendor/libs")

add_executable("${ExecutableName}" "${SourceFiles}")

target_include_directories("${ExecutableName}" PRIVATE "src/")
target_include_directories("${ExecutableName}" PRIVATE "${CMAKE_SOURCE_DIR}/DOGEngine/" "${ExternalIncludePath}")

target_link_libraries("${ExecutableName}" PRIVATE "DOGEngine")
target_compile_options("${ExecutableName}" PRIVATE "/W4")

target_compile_definitions("${ExecutableName}" PRIVATE RUNTIME_DIR="${CMAKE_CURRENT_SOURCE_DIR}/")
target_compile_definitions("${ExecutableName}" PRIVATE PROJECT_WORKSPACE="${CMAKE_SOURCE_DIR}/")
target_compile_definitions("${ExecutableName}" PRIVATE PROJECT_BIN="${CMAKE_BINARY_DIR}/${ExecutableName}")

##### Section for linking external libraries #####
file(GLOB_RECURSE libFiles
	  ${ExternalLibPath}/${CMAKE_BUILD_TYPE}/*.lib)

foreach(var ${libFiles})
	target_link_libraries("${ExecutableName}" PRIVATE "${var}")
endforeach()

file(GLOB_RECURSE dlls
	  ${ExternalLibPath}/${CMAKE_BUILD_TYPE}/*.dll ${ExternalLibPath}/${CMAKE_BUILD_TYPE}/*.pdb)

foreach(var ${dlls})
	add_custom_command(
	TARGET "${ExecutableName}" POST_BUILD 
	COMMAND ${CMAKE_COMMAND} -E copy 
	"${var}"
	${CMAKE_BINARY_DIR}/Runtime/

	)
endforeach()
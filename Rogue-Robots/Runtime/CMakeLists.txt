#Root/Runtime
set(SourceFiles 
	"src/Core/RuntimeApplication.cpp" "src/Core/RuntimeApplication.h"

	"src/Game/EmilFDebugLayer.h" "src/Game/EmilFDebugLayer.cpp"

	"src/Game/LuaInterfaces.h" "src/Game/LuaInterfaces.cpp"
	"src/Game/EntitesTypes.h"

	"src/Game/FakeComputeLightCulling.h" "src/Game/FakeComputeLightCulling.cpp"
	"src/Game/PrefabInstantiatorFunctions.h" "src/Game/PrefabInstantiatorFunctions.cpp"
	"src/Game/LoadSplitModels.h" "src/Game/LoadSplitModels.cpp"

	"src/Game/NetCode.h" "src/Game/NetCode.cpp"
	"src/Network/Server.h" "src/Network/Server.cpp" "src/Network/Client.h" "src/Network/Client.cpp"

	"src/Game/AgentManager/AgentComponents.h"
	"src/Game/AgentManager/AgentManager.h" "src/Game/AgentManager/AgentManager.cpp"
	"src/Game/AgentManager/AgentBehaviorSystems.h" "src/Game/AgentManager/AgentBehaviorSystems.cpp"

	"src/Game/GameLayer.h" "src/Game/GameLayer.cpp" 
	"src/Game/GameSystems.h" "src/Game/GameSystems.cpp"
	"src/Game/ExplosionSystems.h" "src/Game/ExplosionSystems.cpp" "src/Network/Network.h"
	"src/Game/HomingMissileSystem.h" "src/Game/HomingMissileSystem.cpp"
	"src/Game/TurretSystems.h" "src/Game/TurretSystems.cpp"
	"src/Game/HeartbeatTrackerSystem.h" "src/Game/HeartbeatTrackerSystem.cpp"

	"src/Game/Scene.h" "src/Game/Scene.cpp" "src/Game/PcgLevelLoader.h" "src/Game/PcgLevelLoader.cpp"
	"src/Game/PCGLevelScenes.h" "src/Game/PCGLevelScenes.cpp"
	"src/Game/LightScene.h" "src/Game/LightScene.cpp"
	"src/Game/TestScene.h" "src/Game/TestScene.cpp"  
	"src/Game/TestScenes/ParticleScene.h" "src/Game/TestScenes/ParticleScene.cpp"
	"src/Game/OldDefaultScene.h" "src/Game/OldDefaultScene.cpp"

	"src/Game/ItemManager/ItemManager.h" "src/Game/ItemManager/ItemManager.cpp"
	"src/Game/PlayerManager/PlayerManager.h" "src/Game/PlayerManager/PlayerMangager.cpp"
	"src/Core/QueryHelpers.h" "src/Core/QueryHelpers.cpp"

	"src/Pathfinder/Pathfinder.h" "src/Pathfinder/Pathfinder.cpp"
	"src/Pathfinder/PathfinderDebugLayer.h" "src/Pathfinder/PathfinderDebugLayer.cpp"
	"src/Pathfinder/PathfinderComponents.h" "src/Pathfinder/PathfinderComponents.cpp"
	"src/Pathfinder/PathfinderSystems.h" "src/Pathfinder/PathfinderSystems.cpp"

	"src/Game/AnimationIDs.h"
	"src/Game/PlayerEquipmentFunctions.h" "src/Game/PlayerEquipmentFunctions.cpp"
	"src/Game/GoalRadarSystem.h" "src/Game/GoalRadarSystem.cpp"
	"src/Game/InGameMenu.h" "src/Game/InGameMenu.cpp"
	)

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
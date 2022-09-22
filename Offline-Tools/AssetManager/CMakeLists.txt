project("DOG_Offline_AssetManager")

cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_STANDARD_REQUIRED ON)

set(INC "${CMAKE_SOURCE_DIR}/include")
set(LIB "${CMAKE_SOURCE_DIR}/lib/${CMAKE_BUILD_TYPE}")
set(BIN "${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}")
set(SRC "src/main.cpp" "src/AssetManager.h")

add_executable(AssetManager ${SRC})
target_link_directories(AssetManager PRIVATE ${LIB})
target_include_directories(AssetManager PRIVATE ${INC})

set_target_properties(AssetManager PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${BIN})

target_precompile_headers(AssetManager PRIVATE "${INC}/pch.h")

#GLOB libs
file(GLOB_RECURSE LIBS "${LIB}/*.lib")
foreach(lib ${LIBS})
	message("Lib: " ${lib})
	target_link_libraries(AssetManager PRIVATE ${lib})
endforeach()

# GLOB dlls
file(GLOB_RECURSE DLLS "${LIB}/*.dll")
foreach(dll ${DLLS})
	message("Dll: " ${dll})
	add_custom_command(TARGET AssetManager POST_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy "${dll}" "${BIN}"
	)

endforeach()

#Root/DOGEngine
set(SourceFiles "EntryPoint.h" "DOGEngine.h" "src/pch.h" "src/Application.h" "src/Application.cpp" "src/Physics/Physics.h" "src/common/Utils.h" "src/Audio/AudioDevice.h" "src/Audio/AudioDevice.cpp")
set(LibraryName "DOGEngine")
set (ExternalIncludePath "${CMAKE_SOURCE_DIR}/DOGEngine/vendor/includes")
set (ExternalLibPath "${CMAKE_SOURCE_DIR}/DOGEngine/vendor/libs")

add_library("${LibraryName}" STATIC "${SourceFiles}")
target_include_directories("${LibraryName}" PRIVATE "${CMAKE_SOURCE_DIR}/Core/src/")
target_compile_options("${LibraryName}" PRIVATE "/W4")
set_target_properties("${LibraryName}" PROPERTIES LINKER_LANGUAGE "CXX")
target_link_libraries("${LibraryName}" PRIVATE "xaudio2" "ole32")

target_precompile_headers("${LibraryName}" PUBLIC "src/pch.h")

target_link_directories("${LibraryName}" PRIVATE "${ExternalLibPath}")

##### Section for linking external libraries #####
#Example:
#target_link_libraries links one or more libraries for the target.
#The target is already specified with the macro ${ExecutableName} ,
#and the libraries are included in the variable "Libraries". Both are
#already used in target_link_libraries, so you will not touch that function ever.
#To include more libraries to be linked, simply add it after the current last 
#library in the set-function. Assuming "Lua/lua54" is the last and one wanted to also link
#Assimp/assimp.lib it would look like so: set(Libraries "Lua/lua54" "Assimp/assimp")
#Notice how file extensions are NOT included.

#set(Libraries file-name-here!) !!UNCOMMENT THIS!!
#target_link_libraries("${ExecutableName}" PRIVATE "${Libraries}") !!UNCOMMENT THIS!!

#This section deals with DLL-copying from your specified folder
#to the folder where the .exe resides. Simply copy the 4 lines
#starting from "TARGET". Now, change the third and fourth line to specify
#YOUR dll names, including relevant folder names. 
#Make sure a newline is present before the closing parenthesis.

#add_custom_command( !!UNCOMMENT THIS!!
#    TARGET "${ExecutableName}" POST_BUILD !!UNCOMMENT THIS!!
#    COMMAND ${CMAKE_COMMAND} -E copy !!UNCOMMENT THIS!!
#            "${ExternalLibPath}/folder-name/name-of-dll.dll" !!UNCOMMENT THIS!!
#            ${CMAKE_BINARY_DIR}/DOGEngine/name-of-dll.dll !!UNCOMMENT THIS!!
#	!!UNCOMMENT THIS!!
#    )	!!UNCOMMENT THIS!!

 #Last note: Include headers are automatically included correctly assuming you place
 #them accordingly in vendor/includes/your-folder-name in the directory tree hierarchy.
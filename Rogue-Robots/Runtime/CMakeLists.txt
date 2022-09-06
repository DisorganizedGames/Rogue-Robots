#Root/Runtime
set(SourceFiles "src/Core/RuntimeApp.h" "src/Core/RuntimeApp.cpp"  )
set(ExecutableName "Runtime")
set (ExternalIncludePath "${CMAKE_SOURCE_DIR}/Runtime/vendor/includes/")
set (ExternalLibPath "${CMAKE_SOURCE_DIR}/Runtime/vendor/libs")

add_executable("${ExecutableName}" "${SourceFiles}")

target_include_directories("${ExecutableName}" PRIVATE "src/")
target_include_directories("${ExecutableName}" PRIVATE "${CMAKE_SOURCE_DIR}/DOGEngine/" "${ExternalIncludePath}")

target_link_libraries("${ExecutableName}" PRIVATE "DOGEngine")
target_compile_options("${ExecutableName}" PRIVATE "/W4")

target_link_directories("${ExecutableName}" PRIVATE "${ExternalLibPath}")

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

set(Libraries "Lua/lua54")
target_link_libraries("${ExecutableName}" PRIVATE "${Libraries}")

#This section deals with DLL-copying from your specified folder
#to the folder where the .exe resides. Simply copy the 4 lines
#starting from "TARGET". Now, change the third and fourth line to specify
#YOUR dll names, including relevant folder names. 
#Make sure a newline is present before the closing parenthesis.

add_custom_command(
    TARGET "${ExecutableName}" POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy    
            "${ExternalLibPath}/Lua/lua54.dll"
            ${CMAKE_BINARY_DIR}/Runtime/lua54.dll

    )

 #Last note: Include headers are automatically included correctly assuming you place
 #them accordingly in vendor/includes/your-folder-name in the directory tree hierarchy.
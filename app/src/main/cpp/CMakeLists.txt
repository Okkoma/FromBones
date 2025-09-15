# Define target name
set (TARGET_NAME FromBonesLib)

# Don't Work on RPI3 Cross Compile
set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_CXX_STANDARD 17)

# Define source files : add C files for Wren
file (GLOB_RECURSE SOURCE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp" "*.c" "*.hpp" "*.h")

# Add C_FLAGS Compiler c99 idiom => enable "for (int i= ...)" syntax
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

if (URHO3D_LIB_TYPE STREQUAL SHARED)
    add_definitions (-DFROMBONES_EXPORTS)
endif ()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Actors ${CMAKE_CURRENT_SOURCE_DIR}/AI
                    ${CMAKE_CURRENT_SOURCE_DIR}/Components ${CMAKE_CURRENT_SOURCE_DIR}/Generators
                    ${CMAKE_CURRENT_SOURCE_DIR}/GraphicEffects ${CMAKE_CURRENT_SOURCE_DIR}/Managers
                    ${CMAKE_CURRENT_SOURCE_DIR}/Map ${CMAKE_CURRENT_SOURCE_DIR}/MapEditor
                    ${CMAKE_CURRENT_SOURCE_DIR}/NodePool
                    ${CMAKE_CURRENT_SOURCE_DIR}/Objects ${CMAKE_CURRENT_SOURCE_DIR}/ObjectsCore
                    ${CMAKE_CURRENT_SOURCE_DIR}/Resources ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Wren
                    ${CMAKE_CURRENT_SOURCE_DIR}/States
                    ${CMAKE_CURRENT_SOURCE_DIR}/UI 
                    ${CMAKE_CURRENT_SOURCE_DIR}/Libs
                    ${CMAKE_CURRENT_SOURCE_DIR} )

setup_library(${URHO3D_LIB_TYPE})


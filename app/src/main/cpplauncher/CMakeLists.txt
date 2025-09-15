set (TARGET_NAME FromBones)

# Define source files
file (GLOB_RECURSE SOURCE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp" "*.h")

# Add TODO files
set (SOURCE_FILES "${SOURCE_FILES}"
     "../../../../notes/GAME-AI.TODO"
     "../../../../notes/GAME-BEHAVIOR.TODO"
     "../../../../notes/GAME-EDITOR.TODO"
     "../../../../notes/GAME-GENERAL.TODO"
     "../../../../notes/GAME-GRAPHICS.TODO"
     "../../../../notes/GAME-MAPWORLD.TODO"
     "../../../../notes/GAME-NETWORK.TODO"
     "../../../../notes/GAME-SOUND.TODO"
     "../../../../notes/GAME-STORY.TODO"
     "../../../../notes/GAME-UI.TODO" )

# Add C_FLAGS Compiler c99 idiom => enable "for (int i= ...)" syntax
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

set (SOURCE_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../cpp)

if (URHO3D_LIB_TYPE STREQUAL SHARED)
    add_definitions (-DFROMBONES_EXPORTS)
endif ()

include_directories(${SOURCE_LIB_DIR}/Actors ${SOURCE_LIB_DIR}/AI
                    ${SOURCE_LIB_DIR}/Components ${SOURCE_LIB_DIR}/Generators
                    ${SOURCE_LIB_DIR}/GraphicEffects ${SOURCE_LIB_DIR}/Managers
                    ${SOURCE_LIB_DIR}/Map ${SOURCE_LIB_DIR}/MapEditor
                    ${SOURCE_LIB_DIR}/NodePool
                    ${SOURCE_LIB_DIR}/Objects ${SOURCE_LIB_DIR}/ObjectsCore
                    ${SOURCE_LIB_DIR}/Resources ${SOURCE_LIB_DIR}/Resources/Wren
                    ${SOURCE_LIB_DIR}/States
                    ${SOURCE_LIB_DIR}/UI 
                    ${SOURCE_LIB_DIR}/Libs
                    ${SOURCE_LIB_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
					
# Setup target with resource copying
setup_main_executable(NODEPS)

add_dependencies(${TARGET_NAME} FromBonesLib)

target_link_libraries(${TARGET_NAME} FromBonesLib)

setup_test()

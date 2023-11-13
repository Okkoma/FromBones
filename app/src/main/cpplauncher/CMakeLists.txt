set (TARGET_NAME FromBones)

# Define source files
file (GLOB_RECURSE SOURCE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp" "*.h")

# Add TODO files
set (SOURCE_FILES "${SOURCE_FILES}"
     "../../../../GAME-AI.TODO"
     "../../../../GAME-BEHAVIOR.TODO"
     "../../../../GAME-EDITOR.TODO"
     "../../../../GAME-GENERAL.TODO"
     "../../../../GAME-GRAPHICS.TODO"
     "../../../../GAME-MAPWORLD.TODO"
     "../../../../GAME-NETWORK.TODO"
     "../../../../GAME-SOUND.TODO"
     "../../../../GAME-STORY.TODO"
     "../../../../GAME-UI.TODO" )

# Add C_FLAGS Compiler c99 idiom => enable "for (int i= ...)" syntax
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

set (SOURCE_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../cpp)

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

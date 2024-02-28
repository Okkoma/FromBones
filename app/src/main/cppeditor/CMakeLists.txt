
# Only shared Library

set (TARGET_NAME FromBonesEditor)

# Define source files : add C files for Wren
set (SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/MapEditorLib.cpp)

# Add C_FLAGS Compiler c99 idiom => enable "for (int i= ...)" syntax
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

add_definitions (-DFROMBONES_EXPORTS)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../cpp
                    ${CMAKE_CURRENT_SOURCE_DIR}/../cpp/Map 
                    ${CMAKE_CURRENT_SOURCE_DIR}/../cpp/MapEditor
                    ${CMAKE_CURRENT_SOURCE_DIR}/../cpp/GraphicEffects
                    ${CMAKE_CURRENT_SOURCE_DIR}/../cpp/ObjectsCore
                    ${CMAKE_CURRENT_SOURCE_DIR}/../cpp/Resources )
                    
setup_library(SHARED)

# Add flag PIC for Dynamic Library Behavior
set_property(TARGET ${TARGET_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

add_dependencies(${TARGET_NAME} FromBonesLib)

target_link_libraries(${TARGET_NAME} FromBonesLib)



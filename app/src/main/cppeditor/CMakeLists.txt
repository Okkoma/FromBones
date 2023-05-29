
# Only shared Library

set (TARGET_NAME FromBonesEditor)

# Create shared library
add_library(${TARGET_NAME} SHARED ${CMAKE_CURRENT_SOURCE_DIR}/MapEditorLib.cpp)

# Include header files
target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../cpp)
target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../cpp/Map)
target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../cpp/MapEditor)
target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../cpp/ObjectsCore)
target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../cpp/GraphicEffects)
target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../cpp/Resources)

# Add flag PIC for Dynamic Library Behavior
set_property(TARGET ${TARGET_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

add_dependencies(${TARGET_NAME} FromBonesLib)

# Link Against Urho3D
target_link_libraries(${TARGET_NAME} ${URHO3D_HOME}/lib/libUrho3D.so)




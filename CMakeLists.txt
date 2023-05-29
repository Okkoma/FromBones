cmake_minimum_required(VERSION 3.10.2)
project(FromBones)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)
include(UrhoCommon)

add_subdirectory (app/src/main/cpp ${ARGN})

if (URHO3D_LIB_TYPE STREQUAL SHARED)
    add_subdirectory (app/src/main/cppeditor ${ARGN})
endif()

add_subdirectory (app/src/main/cpplauncher ${ARGN})

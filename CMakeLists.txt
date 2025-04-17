cmake_minimum_required (VERSION 3.10.2)

project (FromBones)

#set (URHO3D_HOME ${CMAKE_SOURCE_DIR}/../Urho3D-FromBones-git-local)
set (GIT_U3D_REPO "https://github.com/Okkoma/Urho3D-FromBones.git")

include (${CMAKE_SOURCE_DIR}/cmake/Urho3D.cmake)

message (STATUS "Project configuration:")
message (STATUS "   Host      ${CMAKE_HOST_SYSTEM_NAME}")
message (STATUS "   Platform  ${CMAKE_SYSTEM_NAME}")

if (URHO3D_HOME)
    message (STATUS "   Build     ${CMAKE_BUILD_TYPE}")
    message (STATUS "   Library   ${URHO3D_LIB_TYPE}")
    
    add_subdirectory (app/src/main/cpp ${ARGN})

    if (URHO3D_LIB_TYPE STREQUAL SHARED)
        add_subdirectory (app/src/main/cppeditor ${ARGN})
    endif()

    add_subdirectory (app/src/main/cpplauncher ${ARGN})
else ()
    message ("!! FromBones - wait to add source because URHO3D_HOME is undefined here !")
endif ()

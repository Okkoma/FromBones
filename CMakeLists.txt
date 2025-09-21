cmake_minimum_required (VERSION 3.10.2)

project (FromBones VERSION 0.6.0)

# clangd helper
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Search a urho source or lib
# TODO chris : resolve cmake error when NumFounded > 1
#set (URHO3D_SEARCH_PATH ${CMAKE_SOURCE_DIR}/../Urho3D-FromBones-git-local)

# Use specific urho lib
#set (URHO3D_HOME ${CMAKE_SOURCE_DIR}/../Urho3D-FromBones-git-local/build/linux/gl/static/release)

# Build urho as submodule
set (URHO3D_HOME ${CMAKE_SOURCE_DIR}/../Urho3D-FromBones-git-local)
set (GIT_U3D_REPO "https://github.com/Okkoma/Urho3D-FromBones.git")

# Disable some thirdparty/engine build options.
option (SDL_ATOMIC "Enable the Atomic subsystem" TRUE)
option (SDL_HAPTIC "Enable the Haptic subsystem" FALSE)
option (SDL_LOCALE "Enable the Locale subsystem" FALSE)
option (SDL_POWER "Enable the Power subsystem" FALSE)
option (SDL_SENSOR "Enable the Sensor subsystem" TRUE)
# Audio system need SDL_THREADS (semaphore).
option (SDL_THREADS "Enable the Threads subsystem" TRUE) 
option (SDL_TIMERS "Enable the Timers subsystem" TRUE)

include (${CMAKE_SOURCE_DIR}/cmake/Urho3D.cmake)

if (NOT URHO3D_HOME)
    message ("!! FromBones - wait to add source because URHO3D_HOME is undefined here !")
    return ()
endif ()

if (URHO3D_VULKAN)
    set (GRAPHICS_API "Vulkan")
elseif (URHO3D_OPENGL)
    set (GRAPHICS_API "OpenGL")
elseif (CMAKE_SYSTEM_NAME STREQUAL Windows)
    set (GRAPHICS_API "DirectX")
else ()
    set (GRAPHICS_API "OpenGL")
endif ()

# Dev Game Options

option (FROMBONES_TEST "Enable Testing" FALSE)

message (STATUS "Project configuration:")
message (STATUS "   Version   ${CMAKE_PROJECT_VERSION}")
message (STATUS "   Host      ${CMAKE_HOST_SYSTEM_NAME}")
message (STATUS "   Platform  ${CMAKE_SYSTEM_NAME}")

if (URHO3D_HOME)
    message (STATUS "   Build     ${CMAKE_BUILD_TYPE}")
    message (STATUS "   Library   ${URHO3D_LIB_TYPE}")
    message (STATUS "   Graphics  ${GRAPHICS_API}")

    # Add Main Sources
    add_subdirectory (app/src/main/cpp)
    if (URHO3D_LIB_TYPE STREQUAL SHARED)
        add_subdirectory (app/src/main/cppeditor)
    endif()
    add_subdirectory (app/src/main/cpplauncher)

    # Test
    if (FROMBONES_TEST)
        include (CTest)
        enable_testing ()
        add_subdirectory (app/src/main/cpptest)
    endif ()

    if (NOT ANDROID)
        # Install
        # install stuff is included in UrhoCommon module
        if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT) # Default installation folder	
            set_property (CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE ${CMAKE_SOURCE_DIR}/exe)
        endif ()
        install (FILES ${CMAKE_SOURCE_DIR}/LICENSE DESTINATION .)
        install (FILES ${CMAKE_SOURCE_DIR}/bin/Data/EngineConfig/linux.xml DESTINATION bin RENAME engine_config.xml)

        # CPack
        set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "FromBones - a 2D plateformer game")
        set (CPACK_PACKAGE_VENDOR "https://github.com/Okkoma")
        set (CPACK_PACKAGE_CONTACT "okkoman0xx0@gmail.com")
        set (CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
        set (CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
        set (CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
        set (CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
        set (CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE)
        set (CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_NAME})
        set (CPACK_PACKAGE_FILE_NAME "FromBones-${CMAKE_SYSTEM_NAME}-${GRAPHICS_API}-${CPACK_PACKAGE_VERSION}")
        set (CPACK_GENERATOR ZIP)
        include (CPack)
    endif ()
else ()
    message ("!! FromBones - wait to add source because URHO3D_HOME is undefined here !")
endif ()

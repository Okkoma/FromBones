add_subdirectory(catch2)

# Helper function to create a test
function(add_unit_test TEST_NAME)
    set(TEST_TARGET "test_${TEST_NAME}")
    add_executable(${TEST_TARGET} ${ARGN})
    target_include_directories(${TEST_TARGET} PRIVATE 
    	${CMAKE_CURRENT_SOURCE_DIR}/../cpp
    	${CMAKE_CURRENT_SOURCE_DIR}/../cppeditor
    	${CMAKE_CURRENT_SOURCE_DIR}/../cpplauncher
	${URHO3D_INCLUDE_DIRS})
    target_link_libraries(${TEST_TARGET} PRIVATE Catch2::Catch2WithMain Urho3D)
    add_test(NAME ${TEST_TARGET} COMMAND ${TEST_TARGET} WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endfunction()


add_unit_test(
    "Catch2"
    test_catch2.cpp
)

add_unit_test(
     "ShortIntVector2"
     test_ShortIntVector2.cpp
     ../cpp/ObjectsCore/ShortIntVector2.cpp
)

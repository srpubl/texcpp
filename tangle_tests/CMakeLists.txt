cmake_minimum_required(VERSION 3.14)
project(tangle_tests CXX)

# Fix the timestamp extraction policy bug (prevents infinite rebuild warnings)
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

set(SOURCE_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data")
set(TEST_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/output")
file(MAKE_DIRECTORY "${TEST_OUTPUT_DIR}")
set(TEST_RUNTIME_DATA_DIR "${CMAKE_CURRENT_BINARY_DIR}/data")
file(MAKE_DIRECTORY "${TEST_RUNTIME_DATA_DIR}")

file(GLOB TANGLE_ASSETS "${SOURCE_DATA_DIR}/*")
foreach(ASSET_FILE ${TANGLE_ASSETS})
    get_filename_component(ASSET_NAME "${ASSET_FILE}" NAME)    
    configure_file("${ASSET_FILE}" "${TEST_RUNTIME_DATA_DIR}/${ASSET_NAME}" COPYONLY)
endforeach()

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(tangle_tests tangle_tests.cpp)

target_compile_definitions(tangle_tests PRIVATE
    DATA_DIR="${CMAKE_CURRENT_BINARY_DIR}/data"
    OUTPUT_DIR="${TEST_OUTPUT_DIR}"
)

target_link_libraries(tangle_tests PRIVATE gtest_main tangle_lib)

include(GoogleTest)
gtest_discover_tests(tangle_tests DISCOVERY_MODE PRE_TEST)



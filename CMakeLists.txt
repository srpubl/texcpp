cmake_minimum_required(VERSION 3.14)
project(texcpp CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    # The generator expression enforces that these flags apply ONLY to C++ files
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:/Zc:char8_t;/permissive-;/w14242>")
endif()

enable_testing()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

add_subdirectory(tangle)
add_subdirectory(tangle_tests)

find_program(CLANG_FORMAT_EXE clang-format)
if(CLANG_FORMAT_EXE)
    file(GLOB_RECURSE ALL_FORMAT_SOURCES 
         "tangle/*.cpp" "tangle/*.h"
         "tangle_tests/*.cpp" "tangle_tests/*.h"
         "include/*.h"
    )
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXE} -i -style=file ${ALL_FORMAT_SOURCES}
        COMMENT "Formatting all C++ files with your .clang-format rules..."
    )
endif()

# Crucial: Tells Visual Studio to find tests dynamically after compiling, rather than during generation
include(GoogleTest)
gtest_discover_tests(tangle_tests DISCOVERY_MODE PRE_TEST)

cmake_minimum_required(VERSION 3.14)
project(tangle CXX)

file(GLOB TANGLE_SOURCES "*.cpp" "*.h")

set(LIB_SOURCES ${TANGLE_SOURCES})
list(REMOVE_ITEM LIB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

add_library(tangle_lib STATIC ${LIB_SOURCES})
target_include_directories(tangle_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(tangle_bin main.cpp)
target_link_libraries(tangle_bin PRIVATE tangle_lib)
set_target_properties(tangle_bin PROPERTIES OUTPUT_NAME "tangle")

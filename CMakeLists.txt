cmake_minimum_required(VERSION 3.24)
project(CPPUI LANGUAGES CXX)

add_library(cppui INTERFACE)
add_library(CPPUI::cppui ALIAS cppui)
target_include_directories(cppui INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(cppui INTERFACE cxx_std_11)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

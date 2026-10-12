cmake_minimum_required(VERSION 3.24)
project(minimal)

set(CMAKE_CXX_EXTENSIONS  OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

qt_add_executable(minimal main.cpp)

target_link_libraries(minimal PRIVATE Qt6::Widgets)

include(CTest)
if(BUILD_TESTING)
    include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/CPPUItesting.cmake")
    cppui_add_test(example.minimal_qt minimal --smoke)
endif()

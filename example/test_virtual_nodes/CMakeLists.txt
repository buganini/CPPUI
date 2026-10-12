cmake_minimum_required(VERSION 3.24)
project(test_virtual_nodes)

set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

qt_add_executable(test_virtual_nodes main.cpp)
target_link_libraries(test_virtual_nodes PRIVATE Qt6::Widgets)

include(CTest)
if(BUILD_TESTING)
    include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/CPPUItesting.cmake")
    cppui_add_test(example.test_virtual_nodes test_virtual_nodes --smoke)
endif()

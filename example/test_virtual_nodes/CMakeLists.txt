cmake_minimum_required(VERSION 3.24)
project(test_virtual_nodes)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

qt_add_executable(test_virtual_nodes main.cpp)
target_link_libraries(test_virtual_nodes PRIVATE Qt6::Widgets)

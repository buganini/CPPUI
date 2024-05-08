cmake_minimum_required(VERSION 3.24)
project(minimal)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS  OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

qt_add_executable(minimal main.cpp)

target_link_libraries(minimal PRIVATE Qt6::Widgets)

set_target_properties(minimal PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)
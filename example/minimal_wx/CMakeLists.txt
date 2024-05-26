cmake_minimum_required(VERSION 3.24)
project(minimal)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS  OFF)

find_package(wxWidgets REQUIRED COMPONENTS net core base)
include(${wxWidgets_USE_FILE})

add_executable(minimal main.cpp)
target_link_libraries(minimal ${wxWidgets_LIBRARIES})
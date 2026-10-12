cmake_minimum_required(VERSION 3.24)
project(CPPUICookbookLVGLX11 LANGUAGES C CXX)

set(CPPUI_LVGL_DRIVER lvgl_x11)
include("${CMAKE_CURRENT_LIST_DIR}/../lvgl/build.cmake")

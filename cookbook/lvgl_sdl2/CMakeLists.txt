cmake_minimum_required(VERSION 3.24)
project(CPPUICookbookLVGLSDL2 LANGUAGES C CXX)

set(CPPUI_LVGL_DRIVER lvgl_sdl2)
include("${CMAKE_CURRENT_LIST_DIR}/../lvgl/build.cmake")

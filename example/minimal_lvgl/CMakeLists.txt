cmake_minimum_required(VERSION 3.24)
project(minimal)

option(LV_USE_DRAW_SDL "Use SDL draw unit" OFF)
option(LV_USE_LIBPNG "Use libpng to decode PNG" OFF)
option(LV_USE_LIBJPEG_TURBO "Use libjpeg turbo to decode JPEG" OFF)
option(LV_USE_FFMPEG "Use libffmpeg to display video using lv_ffmpeg" OFF)
option(LV_USE_FREETYPE "Use freetype lib" OFF)

set(CMAKE_C_STANDARD 99)#C99 # lvgl officially support C99 and above
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS  OFF)

find_package(SDL2 REQUIRED SDL2)

add_compile_definitions($<$<BOOL:${LV_USE_DRAW_SDL}>:LV_USE_DRAW_SDL=1>)
add_compile_definitions($<$<BOOL:${LV_USE_LIBPNG}>:LV_USE_LIBPNG=1>)
add_compile_definitions($<$<BOOL:${LV_USE_LIBJPEG_TURBO}>:LV_USE_LIBJPEG_TURBO=1>)
add_compile_definitions($<$<BOOL:${LV_USE_FFMPEG}>:LV_USE_FFMPEG=1>)

add_subdirectory(lvgl)
target_include_directories(lvgl PUBLIC ${PROJECT_SOURCE_DIR} ${SDL2_INCLUDE_DIRS})

add_executable(minimal main.cpp)
target_link_libraries(minimal lvgl lvgl::thorvg ${SDL2_LIBRARIES} m pthread)

if(LV_USE_DRAW_SDL)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
    # Need to install libsdl2-image-dev
    # `sudo apt install libsdl2-image-dev`
    # `brew install sdl2_image`
    find_package(SDL2_image REQUIRED)
    target_include_directories(lvgl PUBLIC ${SDL2_IMAGE_INCLUDE_DIRS})
    target_link_libraries(minimal ${SDL2_IMAGE_LIBRARIES})
endif(LV_USE_DRAW_SDL)

if(LV_USE_LIBPNG)
    find_package(PNG REQUIRED)
    target_include_directories(lvgl PUBLIC ${PNG_INCLUDE_DIR})
    target_link_libraries(minimal ${PNG_LIBRARY})
endif(LV_USE_LIBPNG)

if(LV_USE_LIBJPEG_TURBO)
    # Need to install libjpeg-turbo8-dev
    # `sudo apt install libjpeg-turbo8-dev`
    # `brew install libjpeg-turbo`
    find_package(JPEG REQUIRED)
    target_include_directories(lvgl PUBLIC ${JPEG_INCLUDE_DIRS})
    target_link_libraries(minimal ${JPEG_LIBRARIES})
endif(LV_USE_LIBJPEG_TURBO)

if(LV_USE_FFMPEG)
    target_link_libraries(minimal avformat avcodec avutil swscale)
endif(LV_USE_FFMPEG)

if(LV_USE_FREETYPE)
    find_package(Freetype REQUIRED)
    target_link_libraries(minimal ${FREETYPE_LIBRARIES})
    target_include_directories(lvgl PUBLIC ${FREETYPE_INCLUDE_DIRS})
endif(LV_USE_FREETYPE)

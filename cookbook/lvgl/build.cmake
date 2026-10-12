set(CPPUI_LVGL_SOURCE_DIR "" CACHE PATH "LVGL v9.6.0 source checkout")
set(cppui_lvgl_dir "${CMAKE_CURRENT_LIST_DIR}")
set(cppui_lvgl_target "cppui_cookbook_${CPPUI_LVGL_DRIVER}")
if(NOT EXISTS "${CPPUI_LVGL_SOURCE_DIR}/lvgl.h")
    message(FATAL_ERROR "Set CPPUI_LVGL_SOURCE_DIR to an LVGL 9 source checkout")
endif()
set(LV_BUILD_CONF_PATH "${cppui_lvgl_dir}/lv_conf.h" CACHE PATH "" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "" FORCE)
set(LV_FETCH_DEPENDENCIES OFF CACHE BOOL "" FORCE)
add_subdirectory("${CPPUI_LVGL_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/lvgl")
target_compile_definitions(lvgl PUBLIC
    CPPUI_LVGL_SDL2=$<STREQUAL:${CPPUI_LVGL_DRIVER},lvgl_sdl2>
    CPPUI_LVGL_X11=$<STREQUAL:${CPPUI_LVGL_DRIVER},lvgl_x11>)
if(CPPUI_LVGL_DRIVER STREQUAL "lvgl_sdl2")
    find_package(SDL2 REQUIRED CONFIG)
    target_link_libraries(lvgl PUBLIC SDL2::SDL2)
else()
    find_package(X11 REQUIRED)
    find_package(Threads REQUIRED)
    target_link_libraries(lvgl PUBLIC X11::X11 Threads::Threads)
    include("${cppui_lvgl_dir}/x11-lifetime.cmake")
endif()
if(UNIX)
    target_link_libraries(lvgl PUBLIC m)
endif()

include("${cppui_lvgl_dir}/../common.cmake")
cppui_add_cookbook(${cppui_lvgl_target} lvgl ${CPPUI_LVGL_DRIVER})
target_link_libraries(${cppui_lvgl_target} PRIVATE lvgl::lvgl)
set_target_properties(${cppui_lvgl_target} PROPERTIES
    CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)

if(BUILD_TESTING)
    add_executable(cppui_lvgl_lifetime "${cppui_lvgl_dir}/../../tests/lvgl_lifetime.cpp")
    target_include_directories(cppui_lvgl_lifetime PRIVATE "${cppui_lvgl_dir}/../..")
    target_link_libraries(cppui_lvgl_lifetime PRIVATE lvgl::lvgl)
    set_target_properties(cppui_lvgl_lifetime PROPERTIES
        CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
    cppui_add_test(lvgl.lifetime cppui_lvgl_lifetime)

    add_executable(cppui_lvgl_canvas "${cppui_lvgl_dir}/../../tests/lvgl_canvas.cpp")
    target_include_directories(cppui_lvgl_canvas PRIVATE "${cppui_lvgl_dir}/../..")
    target_link_libraries(cppui_lvgl_canvas PRIVATE lvgl::lvgl)
    set_target_properties(cppui_lvgl_canvas PROPERTIES
        CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
    cppui_add_test(lvgl.canvas cppui_lvgl_canvas)

    add_executable(cppui_lvgl_spacer "${cppui_lvgl_dir}/../../tests/lvgl_spacer.cpp")
    target_include_directories(cppui_lvgl_spacer PRIVATE "${cppui_lvgl_dir}/../..")
    target_link_libraries(cppui_lvgl_spacer PRIVATE lvgl::lvgl)
    set_target_properties(cppui_lvgl_spacer PROPERTIES
        CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
    cppui_add_test(lvgl.spacer cppui_lvgl_spacer)

    add_executable(cppui_lvgl_widgets "${cppui_lvgl_dir}/../../tests/lvgl_widgets.cpp")
    target_include_directories(cppui_lvgl_widgets PRIVATE "${cppui_lvgl_dir}/../..")
    target_link_libraries(cppui_lvgl_widgets PRIVATE lvgl::lvgl)
    set_target_properties(cppui_lvgl_widgets PROPERTIES
        CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
    cppui_add_test(lvgl.widgets cppui_lvgl_widgets)

    if(CPPUI_LVGL_DRIVER STREQUAL "lvgl_sdl2")
        set_property(TEST cookbook.lvgl_sdl2 APPEND PROPERTY ENVIRONMENT
            "SDL_VIDEODRIVER=dummy" "SDL_AUDIODRIVER=dummy" "SDL_RENDER_DRIVER=software")
        if(CPPUI_VALGRIND_TESTS)
            set_property(TEST valgrind.cookbook.lvgl_sdl2 APPEND PROPERTY ENVIRONMENT
                "SDL_VIDEODRIVER=dummy" "SDL_AUDIODRIVER=dummy" "SDL_RENDER_DRIVER=software")
        endif()
    endif()

    foreach(header application base button canvas divider label layout spacer text window)
        set(source "${CMAKE_CURRENT_BINARY_DIR}/header_lvgl_${header}.cpp")
        file(WRITE "${source}"
            "#include \"cppui/lvgl/${header}.hpp\"\n#include \"cppui/lvgl/${header}.hpp\"\n")
        add_library(cppui_header_lvgl_${header} OBJECT "${source}")
        target_include_directories(cppui_header_lvgl_${header} PRIVATE "${cppui_lvgl_dir}/../..")
        target_link_libraries(cppui_header_lvgl_${header} PRIVATE lvgl::lvgl)
    endforeach()
endif()

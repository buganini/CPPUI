#ifndef LV_CONF_H
#define LV_CONF_H
#define LV_COLOR_DEPTH 32
// Individual allocations let ASan and Valgrind observe LVGL-owned resources.
#define LV_USE_STDLIB_MALLOC LV_STDLIB_CLIB
#define LV_USE_LOG 0
// Desktop display refresh and input sampling should run at about 60 Hz.
#define LV_DEF_REFR_PERIOD 16
#define LV_FONT_MONTSERRAT_28 1
#define LV_USE_SDL CPPUI_LVGL_SDL2
// Deliver wheel steps through the pointer device used by the cookbook.
#define LV_SDL_MOUSEWHEEL_MODE LV_SDL_MOUSEWHEEL_MODE_CROWN
#define LV_USE_X11 CPPUI_LVGL_X11
// Allow startup to perform ordered shutdown after deleting an X11 display.
#define LV_X11_DIRECT_EXIT 0
// SDL2::SDL2 supplies the directory containing SDL.h, including custom prefixes.
#define LV_SDL_INCLUDE_PATH <SDL.h>
#endif

#ifndef LV_CONF_H
#define LV_CONF_H
#define LV_COLOR_DEPTH 32
// Individual allocations let ASan and Valgrind observe LVGL-owned resources.
#define LV_USE_STDLIB_MALLOC LV_STDLIB_CLIB
#define LV_USE_LOG 0
#define LV_FONT_MONTSERRAT_28 1
#define LV_USE_SDL CPPUI_LVGL_SDL
// SDL2::SDL2 supplies the directory containing SDL.h, including custom prefixes.
#define LV_SDL_INCLUDE_PATH <SDL.h>
#endif

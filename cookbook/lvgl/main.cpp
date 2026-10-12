#include "backend.hpp"
#include "../recipes.hpp"
#include <cstdint>
#include <iostream>

namespace {
    void shutdown() {
#if LV_USE_SDL
        // LVGL 9.3 deinit quits SDL before releasing its display's texture.
        // Dispose native handles while SDL is live, then release both runtimes.
        while(auto input = lv_indev_get_next(nullptr)) lv_indev_delete(input);
        while(auto display = lv_display_get_next(nullptr)) lv_display_delete(display);
        lv_sdl_quit();
#endif
        lv_deinit();
    }
}

int main(int argc, char ** argv) {
    bool smoke = false;
    bool sdl = false;
    for(int i = 1; i < argc; ++i) {
        if(std::string(argv[i]) == "--list") return Cookbook::run(argc, argv);
        if(std::string(argv[i]) == "--smoke") smoke = true;
        if(std::string(argv[i]) == "--sdl") sdl = true;
    }
#if !LV_USE_SDL
    if(!smoke || sdl) {
        std::cerr << "Use --smoke, or build with CPPUI_LVGL_SDL=ON for an interactive window.\n";
        return 1;
    }
#endif
    lv_init();
    if(smoke && !sdl) {
        auto display = lv_display_create(800, 600);
        static std::uint32_t buffer[800 * 60];
        lv_display_set_buffers(display, buffer, nullptr, sizeof(buffer), LV_DISPLAY_RENDER_MODE_PARTIAL);
        lv_display_set_flush_cb(display, [](lv_display_t * display, const lv_area_t *, std::uint8_t *) {
            lv_display_flush_ready(display);
        });
    }
#if LV_USE_SDL
    else {
        auto display = lv_sdl_window_create(800, 600);
        if(!display || !lv_sdl_window_get_renderer(display)
           || !lv_sdl_mouse_create() || !lv_sdl_keyboard_create()) {
            std::cerr << "Failed to initialize the SDL display/input devices\n";
            shutdown();
            return 1;
        }
    }
#endif
    int result = Cookbook::run(argc, argv);
    // The application must have released declarations and borrowed screen content.
    if(lv_obj_get_child_count(lv_screen_active()) != 0) {
        std::cerr << "Cookbook teardown left native screen children\n";
        result = 1;
    }
    shutdown();
    return result;
}

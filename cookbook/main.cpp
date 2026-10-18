#include "backend.hpp"
#include <iostream>
#if LV_USE_X11
#include <X11/Xlib.h>
#include <chrono>
#include <thread>
#endif

#if defined(CPPUI_COOKBOOK_LVGL)
namespace {
#if LV_USE_SDL
    lv_group_t * inputGroup = nullptr;
#endif
    void shutdown() {
        // Dispose native handles while their driver runtime is still live.
        while(auto input = lv_indev_get_next(nullptr)) lv_indev_delete(input);
        while(auto display = lv_display_get_next(nullptr)) lv_display_delete(display);
#if LV_USE_SDL
        if(inputGroup) lv_group_delete(inputGroup);
        inputGroup = nullptr;
        lv_sdl_quit();
#endif
        lv_deinit();
    }
}

#endif

int main(int argc, char ** argv) {
#if defined(CPPUI_COOKBOOK_LVGL)
    for(int i = 1; i < argc; ++i) {
        if(std::string(argv[i]) == "--list") return Cookbook::run(argc, argv);
    }
#if LV_USE_X11
    // LVGL v9.6.0 assumes XOpenDisplay succeeds inside its X11 driver.
    auto connection = XOpenDisplay(nullptr);
    if(!connection) {
        std::cerr << "Cannot open the X11 display; set DISPLAY to an available X server\n";
        return 1;
    }
    XCloseDisplay(connection);
#endif
    lv_init();
#if LV_USE_X11
    // Account for actual elapsed time, including scheduler delays. The driver's
    // tick thread assumes each sleep lasts exactly 5 ms and drifts behind.
    lv_tick_set_cb([]() -> uint32_t {
        static const auto start = std::chrono::steady_clock::now();
        return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count());
    });
#endif
#if LV_USE_SDL
    {
        auto display = lv_sdl_window_create(800, 600);
        auto pointer = lv_sdl_mouse_create();
        auto keyboard = lv_sdl_keyboard_create();
        if(!display || !lv_sdl_window_get_renderer(display)
           || !pointer || !keyboard) {
            std::cerr << "Failed to initialize the SDL display/input devices\n";
            shutdown();
            return 1;
        }
        lv_sdl_window_set_title(display, Cookbook::Backend::title.c_str());
        inputGroup = lv_group_create();
        lv_group_set_default(inputGroup);
        lv_indev_set_group(pointer, inputGroup);
        lv_indev_set_group(keyboard, inputGroup);
    }
#elif LV_USE_X11
    auto display = lv_x11_window_create(Cookbook::Backend::title.c_str(), 800, 600);
    if(!display) {
        std::cerr << "Failed to initialize the X11 display\n";
        shutdown();
        return 1;
    }
    // The patched driver supplies a native X11 cursor, independent of rendering.
    lv_x11_inputs_create(display, nullptr);
    lv_delay_set_cb([](uint32_t milliseconds) {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    });
#endif
    int result = Cookbook::run(argc, argv);
    // Native close may already have deleted the display and its screen.
    if(lv_display_get_default()) {
        if(lv_obj_get_child_count(lv_screen_active()) != 0) {
            std::cerr << "Cookbook teardown left native screen children\n";
            result = 1;
        }
    }
    shutdown();
    return result;
#else
    return Cookbook::run(argc, argv);
#endif
}

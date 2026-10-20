#ifndef CPPUI_LVGL_RUNTIME
#define CPPUI_LVGL_RUNTIME

#include <lvgl.h>
#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#if LV_USE_X11
#include <X11/Xlib.h>
// Supplied by cmake/CPPUILVGLX11.cmake alongside the LVGL 9.6 driver fixes.
extern "C" unsigned long cppui_lvgl_x11_window_id(lv_display_t * display);
#endif

namespace CPPUI { namespace LVGL {
    // Window declarations only address a host created by their Application.
    // A borrowed display may be an embedded framebuffer with no desktop window.
    class DisplayHost {
        public:
        virtual ~DisplayHost() = default;
        virtual void displayTitle(const std::string & title) = 0;
        virtual void displaySize(int width, int height) = 0;
    };

    class Runtime {
        bool ready = false, ownsRuntime = false, ownsDisplay = false;
        lv_display_t * display = nullptr;
        lv_group_t * group = nullptr;
        lv_group_t * previousGroup = nullptr;
        std::vector<lv_indev_t *> inputs;

        static void displayDeleted(lv_event_t * event) {
            auto self = static_cast<Runtime *>(lv_event_get_user_data(event));
            self->display = nullptr;
        }
        static void inputDeleted(lv_event_t * event) {
            auto self = static_cast<Runtime *>(lv_event_get_user_data(event));
            auto input = static_cast<lv_indev_t *>(lv_event_get_target(event));
            self->inputs.erase(std::remove(self->inputs.begin(), self->inputs.end(), input), self->inputs.end());
        }
        void observeInputs(const std::vector<lv_indev_t *> & previous) {
            for(auto input = lv_indev_get_next(nullptr); input; input = lv_indev_get_next(input)) {
                if(std::find(previous.begin(), previous.end(), input) != previous.end()) continue;
                inputs.push_back(input);
                lv_indev_add_event_cb(input, inputDeleted, LV_EVENT_DELETE, this);
            }
        }
        void reset() {
            if(lv_is_initialized()) {
                // Input callbacks can refer to display driver data until deletion.
                while(!inputs.empty()) lv_indev_delete(inputs.back());
                if(display) {
                    lv_display_remove_event_cb_with_user_data(display, displayDeleted, this);
                    if(ownsDisplay) lv_display_delete(display);
                }
                if(group) {
                    lv_group_delete(group);
                    lv_group_set_default(previousGroup);
                }
#if LV_USE_SDL
                if(ownsDisplay && !lv_display_get_next(nullptr)) lv_sdl_quit();
#endif
                if(ownsRuntime) lv_deinit();
            }
            inputs.clear();
            display = nullptr; group = nullptr; previousGroup = nullptr;
            ready = ownsRuntime = ownsDisplay = false;
        }

        public:
        Runtime() = default;
        Runtime(const Runtime &) = delete;
        Runtime & operator=(const Runtime &) = delete;
        ~Runtime() { reset(); }

        lv_display_t * ensure(const std::string & title) {
            if(ready) return display;
            try {
                ownsRuntime = !lv_is_initialized();
                if(ownsRuntime) lv_init();
                display = lv_display_get_default();
                if(!display) {
                    previousGroup = lv_group_get_default();
                    std::vector<lv_indev_t *> previousInputs;
                    for(auto input = lv_indev_get_next(nullptr); input; input = lv_indev_get_next(input))
                        previousInputs.push_back(input);
#if LV_USE_SDL
                    display = lv_sdl_window_create(800, 600);
                    ownsDisplay = true;
                    if(!display || !lv_sdl_window_get_renderer(display))
                        throw std::runtime_error("Failed to initialize the LVGL SDL display");
                    auto pointer = lv_sdl_mouse_create();
                    auto keyboard = lv_sdl_keyboard_create();
                    observeInputs(previousInputs);
                    if(!pointer || !keyboard)
                        throw std::runtime_error("Failed to initialize LVGL SDL input devices");
                    group = lv_group_create();
                    lv_group_set_default(group);
                    lv_indev_set_group(pointer, group);
                    lv_indev_set_group(keyboard, group);
#elif LV_USE_X11
                    // LVGL 9.6's X11 driver assumes XOpenDisplay succeeds.
                    auto connection = XOpenDisplay(nullptr);
                    if(!connection) throw std::runtime_error("Cannot open the X11 display; set DISPLAY to an available X server");
                    XCloseDisplay(connection);
                    lv_tick_set_cb([]() -> uint32_t {
                        static const auto start = std::chrono::steady_clock::now();
                        return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - start).count());
                    });
                    display = lv_x11_window_create(title.c_str(), 800, 600);
                    ownsDisplay = true;
                    if(!display) throw std::runtime_error("Failed to initialize the LVGL X11 display");
                    lv_x11_inputs_create(display, nullptr);
                    group = lv_group_get_default();
                    observeInputs(previousInputs);
                    lv_delay_set_cb([](uint32_t milliseconds) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
                    });
#else
                    throw std::runtime_error("Provide an LVGL display or enable the SDL2/X11 desktop driver");
#endif
                }
                lv_display_add_event_cb(display, displayDeleted, LV_EVENT_DELETE, this);
                ready = true;
                setTitle(title);
                return display;
            } catch(...) {
                reset();
                throw;
            }
        }

        void setSize(int width, int height) {
            if(!display || !ownsDisplay) return;
#if LV_USE_X11
            auto native = static_cast<_x11_user_hdr_t *>(lv_display_get_driver_data(display));
            XResizeWindow(native->display, cppui_lvgl_x11_window_id(display), width, height);
            XFlush(native->display);
#endif
            // SDL resizes its window on LV_EVENT_RESOLUTION_CHANGED; X11 only
            // reallocates its draw buffers, so its host must be resized above.
            lv_display_set_resolution(display, width, height);
        }

        void setTitle(const std::string & title) {
            if(!display || !ownsDisplay) return;
#if LV_USE_SDL
            lv_sdl_window_set_title(display, title.c_str());
#elif LV_USE_X11
            auto native = static_cast<_x11_user_hdr_t *>(lv_display_get_driver_data(display));
            XStoreName(native->display, cppui_lvgl_x11_window_id(display), title.c_str());
            XFlush(native->display);
#endif
        }
    };
}}
#endif

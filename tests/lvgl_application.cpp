#include "cppui/lvgl/application.hpp"
#include "cppui/lvgl/window.hpp"
#include "cppui/lvgl/label.hpp"
#include <stdexcept>
#include <string>
#include <iostream>
#if LV_USE_SDL
#include <SDL.h>
#endif

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    class App: public CPPUI::Application {
        public:
        using Application::Application;
        bool explicitTitle = false;
        CPPUI::Optional<CPPUI::Size> dimensions;
        void content() override {
            auto & window = CPPUI::Window();
            if(explicitTitle) window.title("Window title");
            if(dimensions) window.size(dimensions->width, dimensions->height);
            for(auto _: window) CPPUI::Label("Runtime-owned window");
        }
    };
    void settle() {
        for(int i=0;i<30;++i) { lv_timer_handler();lv_delay_ms(2); }
    }
    CPPUI::Size nativeSize(lv_display_t * display) {
#if LV_USE_SDL
        int width,height;SDL_GetWindowSize(lv_sdl_window_get_window(display),&width,&height);
        return {width,height};
#else
        auto native=static_cast<_x11_user_hdr_t *>(lv_display_get_driver_data(display));
        XWindowAttributes attributes{};
        XGetWindowAttributes(native->display,cppui_lvgl_x11_window_id(display),&attributes);
        return {attributes.width,attributes.height};
#endif
    }
    void resizeNative(lv_display_t * display,int width,int height) {
#if LV_USE_SDL
        auto window=lv_sdl_window_get_window(display);
        SDL_SetWindowSize(window,width,height);
        // The dummy driver reports programmatic SIZE_CHANGED; LVGL consumes
        // the RESIZED event produced by a user's native window resize.
        SDL_Event event{};
        event.type=SDL_WINDOWEVENT;
        event.window.windowID=SDL_GetWindowID(window);
        event.window.event=SDL_WINDOWEVENT_RESIZED;
        event.window.data1=width;event.window.data2=height;
        SDL_PushEvent(&event);
#else
        auto native=static_cast<_x11_user_hdr_t *>(lv_display_get_driver_data(display));
        XResizeWindow(native->display,cppui_lvgl_x11_window_id(display),width,height);
        XFlush(native->display);
#endif
        settle();
    }
    void checkSize(lv_display_t * display,int width,int height) {
        settle();
        auto native=nativeSize(display);
        const int logicalWidth=lv_display_get_horizontal_resolution(display);
        const int logicalHeight=lv_display_get_vertical_resolution(display);
        if(!(native==CPPUI::Size(width,height)) || logicalWidth!=width || logicalHeight!=height)
            throw std::runtime_error("expected "+std::to_string(width)+"x"+std::to_string(height)+
                ", native "+std::to_string(native.width)+"x"+std::to_string(native.height)+
                ", LVGL "+std::to_string(logicalWidth)+"x"+std::to_string(logicalHeight));
    }
    std::string title(lv_display_t * display) {
#if LV_USE_SDL
        return SDL_GetWindowTitle(lv_sdl_window_get_window(display));
#else
        auto native = static_cast<_x11_user_hdr_t *>(lv_display_get_driver_data(display));
        char * text = nullptr;
        XFetchName(native->display, cppui_lvgl_x11_window_id(display), &text);
        std::string result = text ? text : "";
        if(text) XFree(text);
        return result;
#endif
    }
    void closeNative(lv_display_t * display) {
#if LV_USE_SDL
        SDL_Event event{};
        event.type = SDL_WINDOWEVENT;
        event.window.windowID = SDL_GetWindowID(lv_sdl_window_get_window(display));
        event.window.event = SDL_WINDOWEVENT_CLOSE;
        SDL_PushEvent(&event);
#else
        auto native = static_cast<_x11_user_hdr_t *>(lv_display_get_driver_data(display));
        XEvent event{};
        event.xclient.type = ClientMessage;
        event.xclient.window = cppui_lvgl_x11_window_id(display);
        event.xclient.message_type = XInternAtom(native->display, "WM_PROTOCOLS", False);
        event.xclient.format = 32;
        event.xclient.data.l[0] = XInternAtom(native->display, "WM_DELETE_WINDOW", False);
        XSendEvent(native->display, cppui_lvgl_x11_window_id(display), False, NoEventMask, &event);
        XFlush(native->display);
#endif
    }
}
int main(int argc, char ** argv) {
    try {
        require(!lv_is_initialized(), "construction must start without a runtime");
        { App app(argc, argv); app.title("Unused"); }
        require(!lv_is_initialized(), "constructing an application must not initialize LVGL");
        if(argc > 1 && std::string(argv[1]) == "--missing-display") {
            bool failed = false;
            try { App app(argc, argv); app.sync(); }
            catch(const std::runtime_error &) { failed = true; }
            require(failed && !lv_is_initialized(), "failed startup must release its runtime");
            return 0;
        }
        for(int cycle = 0; cycle < 3; ++cycle) {
            {
                App app(argc, argv);
                app.title("Application title");
                app.dimensions=CPPUI::Size(1024,768);
                app.sync();
                auto display = lv_display_get_default();
                require(display && title(display) == "Application title", "application did not create/title its host");
                checkSize(display,1024,768);
                app.dimensions=CPPUI::Size(900,650);app.sync();checkSize(display,900,650);
                resizeNative(display,700,500);
                app.sync();checkSize(display,700,500);
                app.dimensions.reset();app.sync();checkSize(display,700,500);
                app.dimensions=CPPUI::Size(1024,768);app.sync();checkSize(display,1024,768);
                require(lv_display_get_default()==display,"resizing replaced the display");
                require(lv_group_get_default(), "owned input focus group missing");
                bool pointer = false, keyboard = false;
                for(auto input = lv_indev_get_next(nullptr); input; input = lv_indev_get_next(input)) {
                    pointer |= lv_indev_get_type(input) == LV_INDEV_TYPE_POINTER;
                    keyboard |= lv_indev_get_type(input) == LV_INDEV_TYPE_KEYPAD;
                    if(lv_indev_get_type(input) != LV_INDEV_TYPE_POINTER)
                    require(lv_indev_get_group(input) == lv_group_get_default(), "input group missing");
                }
                require(pointer && keyboard, "owned native input missing");
                app.explicitTitle = true;
                app.sync();
                require(lv_display_get_default() == display && title(display) == "Window title", "redraw replaced host or lost title");
                if(cycle == 1) {
                    closeNative(display);
                    app.start();
                    require(!lv_display_get_next(nullptr), "native close did not exit application loop");
                    app.sync(); // A closed host must not be silently recreated.
                    require(!lv_display_get_next(nullptr), "redraw recreated a closed host");
                }
            }
            require(!lv_is_initialized(), "owned runtime survives application teardown");
        }
        lv_init();
        auto borrowed = lv_display_create(640, 480);
        auto group = lv_group_create(); lv_group_set_default(group);
        auto input = lv_indev_create();
        lv_indev_set_display(input, borrowed);
        for(int cycle = 0; cycle < 2; ++cycle) {
            App app(argc, argv); app.title("Borrowed framebuffer");
            app.dimensions=CPPUI::Size(1024,768);app.sync();
            require(lv_display_get_default() == borrowed && lv_indev_get_next(nullptr) == input,
                    "borrowed display/input replaced");
            require(lv_display_get_horizontal_resolution(borrowed)==640 &&
                    lv_display_get_vertical_resolution(borrowed)==480,"window changed a borrowed framebuffer's resolution");
        }
        require(lv_is_initialized() && lv_display_get_default() == borrowed
                && lv_group_get_default() == group && lv_indev_get_next(nullptr) == input
                && lv_obj_get_child_count(lv_screen_active()) == 0,
                "application deleted borrowed runtime resources or left content");
        {
            App app(argc, argv); app.sync();
            lv_display_delete(borrowed);
            app.start();
        }
        require(lv_is_initialized() && lv_group_get_default() == group,
                "borrowed close transferred runtime ownership");
        lv_indev_delete(input); lv_group_delete(group); lv_deinit();
        std::cout << "LVGL application runtime passed\n";
        return 0;
    } catch(const std::exception & error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

#ifndef CPPUI_LVGL_APPLICATION
#define CPPUI_LVGL_APPLICATION

#include "base.hpp"
#include "runtime.hpp"
#include "window.hpp"
#include "../options.hpp"
#include "../landlock.hpp"
#include "../application_title.hpp"

namespace CPPUI {
    class Application: public LVGL::View, public ApplicationTitle, public LVGL::DisplayHost {
        LVGL::Runtime runtime;
        std::string caption;

        public:
        Application(int, char *[]) {}
        Application & title(const std::string & text) { caption = text; return *this; }
        const std::string & applicationTitle() const override { return caption; }
        void displayTitle(const std::string & text) override { runtime.setTitle(text); }
        void displaySize(int width, int height) override { runtime.setSize(width, height); }
        ~Application() override {
            // Declarations, timers and input observers must die before the host.
            destroyed = true;
            for(auto child: *children) recur_delete(child, true);
            children->clear();
            // Posted callbacks and editor commits must not outlive the host.
            LVGL::dispatch::resetCallbacks();
            LVGL::dispatch::focusedEditor() = LVGL::dispatch::FocusedEditor();
        }

        static LVGL::Window * firstWindow(::CPPUI::Node * node) {
            for(auto child: *node->children) {
                if(auto window = dynamic_cast<LVGL::Window *>(child)) return window;
                if(auto window = firstWindow(child)) return window;
            }
            return nullptr;
        }

        void update(Node * prev) override {
            if(runtime.isReady()) {
                if(!runtime.ensure(caption)) return; // The native host was closed.
                View::update(prev);
                return;
            }
            // Declare first so the display starts at the first Window's size
            // and title (PUI 09911c6) instead of being resized after creation.
            runtime.initialize();
            View::update(prev);
            int width = 800, height = 600;
            std::string title = caption;
            if(auto window = firstWindow(this)) {
                if(window->declaredSize()) { width = window->declaredSize()->width; height = window->declaredSize()->height; }
                if(window->declaredTitle()) title = *window->declaredTitle();
            }
            runtime.ensure(title, width, height);
        }
        void addChild(int, Node *) override {}
        void removeChild(int, Node *) override {}
        void sync() { redraw(); }

        using LVGL::View::run;
        void run(const LandlockPolicy & policy) { sync(); start(policy); }
        void start(const LandlockPolicy & policy) { applyLandlock(policy); start(); }
        void start() override {
            while(lv_display_get_next(nullptr)) {
                uint32_t ms_delay = lv_timer_handler();
                if(!lv_display_get_next(nullptr)) break;
                lv_delay_ms(ms_delay == LV_NO_TIMER_READY ? LV_DEF_REFR_PERIOD : ms_delay);
            }
        }
    };
}
#endif

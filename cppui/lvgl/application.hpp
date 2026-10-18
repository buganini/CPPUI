#ifndef CPPUI_LVGL_APPLICATION
#define CPPUI_LVGL_APPLICATION

#include "base.hpp"
#include "runtime.hpp"
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
        ~Application() override {
            // Declarations, timers and input observers must die before the host.
            destroyed = true;
            for(auto child: *children) recur_delete(child, true);
            children->clear();
        }

        void update(Node * prev) override {
            if(!runtime.ensure(caption)) return; // The native host was closed.
            View::update(prev);
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

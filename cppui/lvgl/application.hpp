#ifndef CPPUI_LVGL_APPLICATION
#define CPPUI_LVGL_APPLICATION

#include "base.hpp"
#include "../options.hpp"

#include <iostream>
#include "lvgl.h"

namespace CPPUI {
    class Application: public LVGL::View {
        int argc;
        char **argv;

        public:
        Application(int argc, char *argv[]): LVGL::View() {
            this->argc = argc;
            this->argv = argv;
        }

        void update(Node * prev) {
            View::update(prev);
        }

        void addChild(int idx, Node *child) {
        }

        void removeChild(int idx, Node *child) {
        }


        void start(void) override {
            std::cout << "start\n";
            while(lv_display_get_next(nullptr)) {
                uint32_t ms_delay = lv_timer_handler();
                // A native close can delete the last display inside the handler.
                // Return before sleeping so declarations and the runtime unwind.
                if(!lv_display_get_next(nullptr)) break;
                lv_delay_ms(ms_delay == LV_NO_TIMER_READY ? LV_DEF_REFR_PERIOD : ms_delay);
            }
        }
    };
}

#endif

#ifndef CPPUI_LVGL_APPLICATION
#define CPPUI_LVGL_APPLICATION

#include "base.hpp"

#include <iostream>
#include "unistd.h"
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

        ~Application() override {
            destroyed = true;
            for(::CPPUI::Node * child: *children) recur_delete(child, true);
            children->clear();
        }

        void update(Node * prev) {
            View::update(prev);
        }

        void addChild(int idx, Node *child) {
        }

        void removeChild(int idx, Node *child) {
        }


        void start(void) {
            std::cout << "start\n";
            while (true) {
                uint32_t ms_delay = lv_timer_handler();
                usleep(ms_delay * 1000);
            }
        }
    };
}

#endif

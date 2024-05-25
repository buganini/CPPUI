#include "base.hpp"
#include "lvgl.h"

#include <iostream>

namespace CPPUI {
    namespace LVGL {
        class Window: public Node {
            lv_display_t *window = nullptr;
            lv_indev_t *mouse = nullptr;
            lv_indev_t *keyboard = nullptr;

            public:
            void update(Node * prev) {
                if(prev && prev->ui) {
                    ui = prev->ui;
                    window = ((Window *)prev)->window;
                    mouse = ((Window *)prev)->mouse;
                    keyboard = ((Window *)prev)->keyboard;
                } else {
                    window = lv_sdl_window_create(800, 600);
                    mouse = lv_sdl_mouse_create();
                    keyboard = lv_sdl_keyboard_create();
                    ui = lv_screen_active();
                }
            }

            void addChild(int idx, Node * child) {
                Node::addChild(idx, child);
                lv_obj_set_size((lv_obj_t *)child->outer(), lv_pct(100), lv_pct(100));
            }

            void removeChild(int idx, Node * child) {
                Node::removeChild(idx, child);
            }
       };
    }
    LVGL::Window & Window() {
        return *(new LVGL::Window());
    }
}

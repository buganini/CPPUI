#include "base.hpp"

#include "lvgl.h"

namespace CPPUI {
    namespace LVGL {
        class Window: public Node {
            lv_indev_t *mouse = nullptr;
            lv_indev_t *keyboard = nullptr;

            public:
            void update(Node * prev) {
                if(prev && prev->ui) {
                    ui = prev->ui;
                    mouse = ((Window *)prev)->mouse;
                    keyboard = ((Window *)prev)->keyboard;
                } else {
                    ui = lv_sdl_window_create(800, 600);
                    mouse = lv_sdl_mouse_create();
                    keyboard = lv_sdl_keyboard_create();
                }
            }

            void addChild(int idx, Node *child) {
            }

            void removeChild(int idx, Node *child) {
            }
       };
    }
    LVGL::Window & Window() {
        return *(new LVGL::Window());
    }
}

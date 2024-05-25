#include "base.hpp"

#include "lvgl.h"

namespace CPPUI {
    namespace LVGL {
        class HBox: public Node {
            lv_display_t *display = nullptr;
            lv_indev_t *mouse = nullptr;
            lv_indev_t *keyboard = nullptr;

            public:
            void update(Node * prev) {
                if(prev && ((Window *)prev)->display) {
                    display = ((Window *)prev)->display;
                    mouse = ((Window *)prev)->mouse;
                    keyboard = ((Window *)prev)->keyboard;
                } else {
                    display = lv_sdl_window_create(800, 600);
                    mouse = lv_sdl_mouse_create();
                    keyboard = lv_sdl_keyboard_create();
                }
            }

            void addChild(int idx, Node *child) {
                // ((QMainWindow *)ui)->setCentralWidget((QWidget *) child->outer());
            }

            void removeChild(int idx, Node *child) {
                // ((QWidget *)child->outer())->setParent(nullptr);
            }
       };
    }
    LVGL::Window & Window() {
        return *(new LVGL::Window());
    }
}

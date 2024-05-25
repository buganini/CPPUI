#include "base.hpp"
#include "lvgl.h"

#include <iostream>

namespace CPPUI {
    namespace LVGL {
        class Window: public Node {
            public:
            void update(Node * prev) {
                if(prev && prev->ui) {
                    ui = prev->ui;
                } else {
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

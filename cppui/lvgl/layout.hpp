#include "base.hpp"

#include "lvgl.h"

namespace CPPUI {
    namespace LVGL {
        class HBox: public Node {
            public:
            void update(Node * prev) {
                if(prev && prev->ui) {
                    ui = prev->ui;
                } else {
                    ui = lv_obj_create((lv_obj_t *)parent->inner());
                    lv_obj_set_flex_flow((lv_obj_t *)ui, LV_FLEX_FLOW_ROW);
                }
            }
       };

        class VBox: public Node {
            public:
            void update(Node * prev) {
                if(prev && prev->ui) {
                    ui = prev->ui;
                } else {
                    ui = lv_obj_create((lv_obj_t *)parent->inner());
                    lv_obj_set_flex_flow((lv_obj_t *)ui, LV_FLEX_FLOW_COLUMN);
                }
            }
       };

    }
    LVGL::HBox & HBox() {
        return *(new LVGL::HBox());
    }
    LVGL::VBox & VBox() {
        return *(new LVGL::VBox());
    }
}

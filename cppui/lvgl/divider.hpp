#ifndef CPPUI_LVGL_DIVIDER
#define CPPUI_LVGL_DIVIDER

#include "base.hpp"

namespace CPPUI {
    namespace LVGL {
        class Divider: public Node {
            public:
            Divider() { is_terminal = true; is_movable = false; }
            void update(Node * prev) override {
                auto owner = static_cast<lv_obj_t *>(parent->inner());
                ui = prev && prev->ui ? prev->ui : lv_obj_create(owner);
                auto object = static_cast<lv_obj_t *>(ui);
                const bool flex = lv_obj_get_style_layout(owner, LV_PART_MAIN) == LV_LAYOUT_FLEX;
                const bool column = lv_obj_get_style_flex_flow(owner, LV_PART_MAIN) & LV_FLEX_COLUMN;
                expandX=column ? 1 : 0;expandY=column ? 0 : 1;
                lv_obj_remove_style_all(object);
                lv_obj_set_size(object, column ? lv_pct(100) : 1, column ? 1 : lv_pct(100));
                lv_obj_set_style_bg_color(object, lv_color_hex(0x808080), 0);
                lv_obj_set_style_bg_opa(object, flex ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
                lv_obj_set_clickable(object, false);
                lv_obj_set_scrollable(object, false);
            }
        };
    }
    inline LVGL::Divider & Divider() { return *new LVGL::Divider(); }
}
#endif

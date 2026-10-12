#ifndef CPPUI_LVGL_SCROLL
#define CPPUI_LVGL_SCROLL

#include "base.hpp"

namespace CPPUI { namespace LVGL {
    class Scroll: public Node {
        Optional<bool> vertical, horizontal;
        public:
        Scroll(Optional<bool> vertical = {}, Optional<bool> horizontal = false): vertical(vertical), horizontal(horizontal) {
            if((vertical && *vertical && !horizontal) || (horizontal && *horizontal && !vertical))
                throw std::invalid_argument("LVGL cannot force one scrollbar while the other is automatic");
        }
        void update(Node * previous) override {
            ui = previous && previous->ui ? previous->ui : lv_obj_create(static_cast<lv_obj_t *>(parent->inner()));
            auto object = static_cast<lv_obj_t *>(ui);
            lv_obj_set_size(object, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_pad_all(object, 0, 0);
            lv_obj_set_style_radius(object, 0, 0);
            const bool v = !vertical || *vertical, h = !horizontal || *horizontal;
            lv_obj_set_scroll_dir(object, v ? (h ? LV_DIR_ALL : LV_DIR_VER) : (h ? LV_DIR_HOR : LV_DIR_NONE));
            lv_obj_set_scrollbar_mode(object, !v && !h ? LV_SCROLLBAR_MODE_OFF
                : ((vertical && *vertical) || (horizontal && *horizontal)) ? LV_SCROLLBAR_MODE_ON : LV_SCROLLBAR_MODE_AUTO);
        }
        void layoutChild(Node * child) override {
            if(!child->ui || child->is_outoforder) return;
            auto object = static_cast<lv_obj_t *>(child->outer());
            // A disabled axis fits content to the viewport; enabled axes retain
            // the child's content size. Explicit dimensions take precedence.
            if(horizontal && !*horizontal && !child->layout_width) lv_obj_set_width(object, lv_pct(100));
            if(vertical && !*vertical && !child->layout_height) lv_obj_set_height(object, lv_pct(100));
        }
    };
}}
namespace CPPUI {
    inline LVGL::Scroll & Scroll(Optional<bool> vertical = {}, Optional<bool> horizontal = false) { return *new LVGL::Scroll(vertical, horizontal); }
}
#endif

#ifndef CPPUI_LVGL_SCROLL
#define CPPUI_LVGL_SCROLL

#include "base.hpp"

namespace CPPUI { namespace LVGL {
    class Scroll: public Node {
        Optional<bool> vertical, horizontal;
        void nativeEvent(lv_event_t * event) override {
            const auto code=lv_event_get_code(event);
            if(code==LV_EVENT_SIZE_CHANGED || code==LV_EVENT_CHILD_CHANGED || code==LV_EVENT_CHILD_DELETED) {
                // Focus scrolling can start before responsive content reflows.
                // Its old destination must not scroll past the new content bounds.
                auto object=static_cast<lv_obj_t *>(ui);
                lv_obj_stop_scroll_anim(object);
                lv_obj_readjust_scroll(object,LV_ANIM_OFF);
            }
            Node::nativeEvent(event);
        }
        public:
        Scroll(Optional<bool> vertical = {}, Optional<bool> horizontal = false): vertical(vertical), horizontal(horizontal) {
            // LVGL hit-testing skips non-clickable objects, so a drag that starts
            // on non-clickable content (e.g. a Label) must find the viewport itself.
            clickable = true;
            // LVGL has one scrollbar mode per object: forcing either axis shows
            // both scrollbars where scrolling is enabled (as PUI does).
        }
        int baseExpansion(bool x) const override { return expansionAxis(x) ? 4 : 1; }
        bool expansionAxis(bool x) const override { const auto & value=x ? horizontal : vertical;return !value || *value; }
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
            // A disabled axis fits content to the viewport: at least the
            // viewport size, but the scroll area still follows the content
            // (as PUI does). An exact 100% would make a content-sized Scroll
            // and its child depend on each other and collapse to zero.
            // Explicit dimensions take precedence; the next update restores
            // the child's own minimum.
            child->restoreBoxStyles();
            if(horizontal && !*horizontal && !child->layout_width) {
                child->saveBoxStyle(LV_STYLE_MIN_WIDTH);
                lv_obj_set_style_min_width(object, lv_pct(100), 0);
            }
            if(vertical && !*vertical && !child->layout_height) {
                child->saveBoxStyle(LV_STYLE_MIN_HEIGHT);
                lv_obj_set_style_min_height(object, lv_pct(100), 0);
            }
        }
    };
}}
namespace CPPUI {
    inline LVGL::Scroll & Scroll(Optional<bool> vertical = {}, Optional<bool> horizontal = false) { return *new LVGL::Scroll(vertical, horizontal); }
}
#endif

#ifndef CPPUI_LVGL_SPACER
#define CPPUI_LVGL_SPACER

#include "base.hpp"

namespace CPPUI {
    namespace LVGL {
        class Spacer: public Node {
            static void nativeDeleted(lv_event_t * event) {
                static_cast<Spacer *>(lv_event_get_user_data(event))->ui = nullptr;
            }
            public:
            Spacer() { is_terminal = true; is_movable = false; }
            void update(Node * prev) override {
                ui = prev && prev->ui ? prev->ui : lv_obj_create(static_cast<lv_obj_t *>(parent->inner()));
                auto object = static_cast<lv_obj_t *>(ui);
                if(prev) lv_obj_remove_event_cb_with_user_data(object, nativeDeleted, static_cast<Spacer *>(prev));
                lv_obj_add_event_cb(object, nativeDeleted, LV_EVENT_DELETE, this);
                // The physical parent's flex flow selects the expanding axis.
                lv_obj_remove_style_all(object);
                lv_obj_set_size(object, 0, 0);
                lv_obj_set_flex_grow(object, 1);
                lv_obj_set_clickable(object, false);
                lv_obj_set_click_focusable(object, false);
                lv_obj_set_scrollable(object, false);
            }
            void destroy(bool direct) override {
                if(ui) lv_obj_remove_event_cb_with_user_data(static_cast<lv_obj_t *>(ui), nativeDeleted, this);
                Node::destroy(direct);
            }
        };
    }
    inline LVGL::Spacer & Spacer() { return *new LVGL::Spacer(); }
}
#endif

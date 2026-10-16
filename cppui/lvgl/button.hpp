#ifndef CPPUI_LVGL_BUTTON
#define CPPUI_LVGL_BUTTON

#include "base.hpp"

namespace CPPUI {
    namespace LVGL {
        class Button: public Node {
            bool isDisabled = false;
            static void labelDeleted(lv_event_t * event) {
                static_cast<Button *>(lv_event_get_user_data(event))->label = nullptr;
            }
            void unobserveLabel() {
                if(label) lv_obj_remove_event_cb_with_user_data(label, labelDeleted, this);
            }
            public:
            lv_obj_t * label = nullptr;
            std::string text;
            explicit Button(const std::string & text): text(text) {
                is_terminal = true;
                clickable = true;
            }
            ~Button() override { unobserveLabel(); }
            Button & disabled(bool value = true) { isDisabled = value; return *this; }
            void destroy(bool direct) override {
                unobserveLabel();
                label = nullptr;
                Node::destroy(direct);
            }
            void update(Node * prev) override {
                auto previous = static_cast<Button *>(prev);
                if(previous && previous->ui) {
                    ui = previous->ui;
                    label = previous->label;
                    previous->unobserveLabel();
                    previous->label = nullptr;
                } else {
                    ui = lv_button_create(static_cast<lv_obj_t *>(parent->inner()));
                }
                lv_obj_set_state(static_cast<lv_obj_t *>(ui), LV_STATE_DISABLED, isDisabled);
                if(!label) label = lv_label_create(static_cast<lv_obj_t *>(ui));
                lv_obj_add_event_cb(label, labelDeleted, LV_EVENT_DELETE, this);
                lv_label_set_text(label, text.c_str());
                lv_obj_center(label);
            }
        };
    }
    inline LVGL::Button & Button(const std::string & text) { return *new LVGL::Button(text); }
}

#endif

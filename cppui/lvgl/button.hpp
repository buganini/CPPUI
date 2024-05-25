#include "base.hpp"

#include "lvgl.h"

namespace CPPUI {
    namespace LVGL {
        static void event_handler(lv_event_t * e);

        class Button: public Node {
            public:
            lv_obj_t * label;

            std::string text;
            Button(const char * text) {
                this->text = std::string(text);
            }
            Button(const std::string text) {
                this->text = text;
            }

            void update(Node * prev) {
                if(prev && prev->ui) {
                    ui = prev->ui;
                    label = ((Button *)prev)->label;
                    lv_obj_remove_event_cb((lv_obj_t *)ui, event_handler);
                } else {
                    // ui = lv_button_create((lv_obj_t *)parent->inner());
                    ui = lv_button_create(lv_screen_active());
                    label = lv_label_create((lv_obj_t *)ui);
                }
                lv_obj_add_event_cb((lv_obj_t *)ui, event_handler, LV_EVENT_ALL, this);
                lv_label_set_text(label, text.c_str());
                lv_obj_center(label);
            }
        };

        static void event_handler(lv_event_t * e)
        {
            lv_event_code_t code = lv_event_get_code(e);
            Button * button = (Button *)lv_event_get_user_data(e);

            if(code == LV_EVENT_CLICKED) {
                button->onClicked();
            }
            else if(code == LV_EVENT_VALUE_CHANGED) {
                LV_LOG_USER("Toggled");
            }
        }
    }
    LVGL::Button & Button(const char * text) {
        return *(new LVGL::Button(text));
    }
    LVGL::Button & Button(const std::string text) {
        return *(new LVGL::Button(text));
    }
}

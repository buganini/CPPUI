#ifndef CPPUI_LVGL_CHECKBOX
#define CPPUI_LVGL_CHECKBOX

#include "base.hpp"
#include "../binding.hpp"

namespace CPPUI {
    namespace LVGL {
        class Checkbox: public Node {
            Binding<bool> model;
            bool checked, isDisabled = false;
            protected:
            void nativeEvent(lv_event_t * event) override {
                if(lv_event_get_code(event) == LV_EVENT_VALUE_CHANGED) {
                    auto binding = model;
                    const bool value = lv_obj_has_state(static_cast<lv_obj_t *>(lv_event_get_current_target(event)), LV_STATE_CHECKED);
                    binding.set(value); // May redraw or remove this declaration.
                } else Node::nativeEvent(event);
            }
            public:
            std::string text;
            Checkbox(const std::string & text, Binding<bool> model): model(model), checked(model.get()), text(text) {
                is_terminal = true;
                clickable = true;
            }
            Checkbox & disabled(bool value = true) { isDisabled = value; return *this; }
            void update(Node * prev) override {
                ui = prev && prev->ui ? prev->ui : lv_checkbox_create(static_cast<lv_obj_t *>(parent->inner()));
                auto object = static_cast<lv_obj_t *>(ui);
                lv_checkbox_set_text(object, text.c_str());
                lv_obj_set_checked(object, checked);
                lv_obj_set_state(object, LV_STATE_DISABLED, isDisabled);
            }
        };
    }
    inline LVGL::Checkbox & Checkbox(const std::string & text, Binding<bool> model) { return *new LVGL::Checkbox(text, model); }
}
#endif

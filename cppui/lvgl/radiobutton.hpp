#ifndef CPPUI_LVGL_RADIOBUTTON
#define CPPUI_LVGL_RADIOBUTTON

#include "base.hpp"
#include "../binding.hpp"

namespace CPPUI { namespace LVGL {
    template<class T>
    class RadioButton: public Node {
        std::string text;
        T value;
        Binding<T> model;
        bool checked, isDisabled = false;
        protected:
        void nativeEvent(lv_event_t * event) override {
            const auto code = lv_event_get_code(event);
            const bool clicked = code == LV_EVENT_CLICKED;
            const bool selectedByKey = code == LV_EVENT_KEY
                && (lv_event_get_key(event) == LV_KEY_RIGHT || lv_event_get_key(event) == LV_KEY_UP);
            if(clicked || selectedByKey) {
                // A setter can redraw or remove this declaration and its native object.
                auto binding = model;
                auto selected = value;
                auto invoke = onClicked;
                binding.set(selected);
                if(clicked && invoke) invoke();
            } else Node::nativeEvent(event);
        }
        public:
        using Value = T;
        RadioButton(const std::string & text, const T & value, Binding<T> model)
            : text(text), value(value), model(model), checked(model.get() == value) {
            is_terminal = true;
            clickable = true;
        }
        RadioButton & disabled(bool value = true) { isDisabled = value; return *this; }
        void update(Node * prev) override {
            ui = prev && prev->ui ? prev->ui : lv_checkbox_create(static_cast<lv_obj_t *>(parent->inner()));
            auto object = static_cast<lv_obj_t *>(ui);
            lv_obj_set_radio_button(object, true);
            lv_obj_set_style_radius(object, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
            lv_obj_set_style_bg_image_src(object, nullptr, LV_PART_INDICATOR | LV_STATE_CHECKED);
            lv_checkbox_set_text(object, text.c_str());
            lv_obj_set_checked(object, checked);
            lv_obj_set_state(object, LV_STATE_DISABLED, isDisabled);
        }
    };
}}
namespace CPPUI {
    template<class T>
    inline LVGL::RadioButton<T> & RadioButton(const std::string & text, const typename LVGL::RadioButton<T>::Value & value, Binding<T> model) {
        return *new LVGL::RadioButton<T>(text, value, model);
    }
    template<class T>
    inline LVGL::RadioButton<T> & RadioButton(const std::string & text, const typename LVGL::RadioButton<T>::Value & value, State<T> & model) {
        return *new LVGL::RadioButton<T>(text, value, Binding<T>(model));
    }
}
#endif

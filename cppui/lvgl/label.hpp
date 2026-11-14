#ifndef CPPUI_LVGL_LABEL
#define CPPUI_LVGL_LABEL

#include "base.hpp"

#include "lvgl.h"

namespace CPPUI {
    namespace LVGL {
        class Label: public Node {
            public:
            std::string text;
            // Accepted for API parity with PUI; LVGL labels have no selection.
            bool selectable;
            Label(const char * text, bool selectable = false): text(text), selectable(selectable) {
                is_terminal = true;
            }
            Label(const std::string & text, bool selectable = false): text(text), selectable(selectable) {
                is_terminal = true;
            }

            void update(Node * prev) override {
                ui = prev && prev->ui ? prev->ui : lv_label_create(static_cast<lv_obj_t *>(parent->inner()));
                auto object = static_cast<lv_obj_t *>(ui);
                // Constrained widths wrap; otherwise the label keeps its natural size.
                lv_label_set_long_mode(object, layout_width ? LV_LABEL_LONG_MODE_WRAP : LV_LABEL_LONG_MODE_CLIP);
                lv_label_set_text(object, text.c_str());
            }
       };
    }
    inline LVGL::Label & Label(const char * text, bool selectable = false) {
        return *(new LVGL::Label(text, selectable));
    }
    inline LVGL::Label & Label(const std::string & text, bool selectable = false) {
        return *(new LVGL::Label(text, selectable));
    }
}

#endif

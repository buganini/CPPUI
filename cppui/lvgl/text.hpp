#ifndef CPPUI_LVGL_TEXT
#define CPPUI_LVGL_TEXT

#include "base.hpp"
#include "../diagnostics.hpp"

namespace CPPUI {
    namespace LVGL {
        class Text: public Node {
            public:
            std::string text;
            // Accepted for API parity with PUI; LVGL labels have no selection.
            bool selectable;
            explicit Text(const std::string & text, bool selectable = false): text(text), selectable(selectable) {
                is_terminal = true;
            }
            void update(Node * prev) override {
                ui = prev && prev->ui ? prev->ui : lv_label_create(static_cast<lv_obj_t *>(parent->inner()));
                auto object = static_cast<lv_obj_t *>(ui);
                lv_label_set_long_mode(object, LV_LABEL_LONG_MODE_WRAP);
                lv_label_set_text(object, text.c_str());
                lv_obj_set_width(object, lv_pct(100));
                lv_obj_set_height(object, LV_SIZE_CONTENT);
            }
        };
    }
    inline LVGL::Text & Text(const std::string & text, bool selectable = false) { return *new LVGL::Text(text, selectable); }
    CPPUI_UNSUPPORTED("CPPUI/LVGL Html has no HTML renderer; displaying raw text")
    inline LVGL::Text & Html(const std::string & text, bool selectable = false) { return *new LVGL::Text(text, selectable); }
    CPPUI_UNSUPPORTED("CPPUI/LVGL MarkDown has no Markdown renderer; displaying raw text")
    inline LVGL::Text & MarkDown(const std::string & text, bool selectable = false) { return *new LVGL::Text(text, selectable); }
}
#endif

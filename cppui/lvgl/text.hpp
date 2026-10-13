#ifndef CPPUI_LVGL_TEXT
#define CPPUI_LVGL_TEXT

#include "base.hpp"
#include "../diagnostics.hpp"
#include <stdexcept>

namespace CPPUI {
    namespace LVGL {
        class Text: public Node {
            public:
            std::string text;
            explicit Text(const std::string & text, bool selectable = false): text(text) {
                is_terminal = true;
                if(selectable) throw std::invalid_argument("LVGL Text does not support interactive text selection");
            }
            void update(Node * prev) override {
                ui = prev && prev->ui ? prev->ui : lv_label_create(static_cast<lv_obj_t *>(parent->inner()));
                auto object = static_cast<lv_obj_t *>(ui);
                lv_label_set_long_mode(object, LV_LABEL_LONG_MODE_WRAP);
                lv_label_set_text(object, text.c_str());
                lv_obj_set_width(object, lv_pct(100));
                lv_obj_set_height(object, LV_SIZE_CONTENT);
                lv_obj_set_clickable(object, false);
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

#include "base.hpp"

#include "lvgl.h"

namespace CPPUI {
    namespace LVGL {
        class Label: public Node {
            public:
            std::string text;
            Label(const char * text) {
                this->text = std::string(text);
            }
            Label(const std::string text) {
                this->text = text;
            }

            void update(Node * prev) {
                if(prev && prev->ui) {
                    ui = prev->ui;
                } else {
                    ui = lv_label_create((lv_obj_t *)parent->inner());
                }
                lv_label_set_text((lv_obj_t *)ui, text.c_str());
            }
       };
    }
    LVGL::Label & Label(const char * text) {
        return *(new LVGL::Label(text));
    }
    LVGL::Label & Label(const std::string text) {
        return *(new LVGL::Label(text));
    }
}

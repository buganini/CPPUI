#ifndef CPPUI_LVGL_WINDOW
#define CPPUI_LVGL_WINDOW

#include "base.hpp"
#include "runtime.hpp"
#include "../application_title.hpp"
#include "lvgl.h"

#include <iostream>

namespace CPPUI {
    namespace LVGL {
        class Window: public Node {
            OptionalText caption;
            public:
            Window & title(const std::string & text) { caption = text; return *this; }
            bool supportsVisibility() const override { return false; }
            void update(Node * prev) {
                if(prev && prev->ui) {
                    ui = prev->ui;
                } else {
                    ui = lv_screen_active();
                }
                if(auto title = windowTitle(this, caption)) {
                    auto root = static_cast<::CPPUI::Node *>(this);
                    while(root->parent && root->parent != root) root = root->parent;
                    if(auto host = dynamic_cast<DisplayHost *>(root)) host->displayTitle(*title);
                }
            }

            void destroy(bool direct) override {
                // The display owns its active screen; only clear our content.
                if(direct && ui) lv_obj_clean(static_cast<lv_obj_t *>(ui));
                ui = nullptr;
            }

            void addChild(int idx, Node * child) {
                Node::addChild(idx, child);
                layoutChild(child);
            }

            void layoutChild(Node * child) override {
                if(!child->ui) return;
                if(!child->layout_width) lv_obj_set_width(static_cast<lv_obj_t *>(child->outer()), lv_pct(100));
                if(!child->layout_height) lv_obj_set_height(static_cast<lv_obj_t *>(child->outer()), lv_pct(100));
            }

            void removeChild(int idx, Node * child) {
                Node::removeChild(idx, child);
            }
       };
    }
    inline LVGL::Window & Window() {
        return *(new LVGL::Window());
    }
}

#endif

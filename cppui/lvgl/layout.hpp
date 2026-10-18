#ifndef CPPUI_LVGL_LAYOUT
#define CPPUI_LVGL_LAYOUT

#include "base.hpp"
#include "spacer.hpp"
#include "divider.hpp"

namespace CPPUI { namespace LVGL {
    class Box: public Layout {
        bool horizontal;
        public:
        explicit Box(bool horizontal): horizontal(horizontal) {}
        void update(Node * previous) override {
            ui = previous && previous->ui ? previous->ui : lv_obj_create(static_cast<lv_obj_t *>(parent->inner()));
            auto object = static_cast<lv_obj_t *>(ui);
            lv_obj_set_flex_flow(object, horizontal ? LV_FLEX_FLOW_ROW : LV_FLEX_FLOW_COLUMN);
            lv_obj_set_flex_align(object, LV_FLEX_ALIGN_START,
                horizontal ? LV_FLEX_ALIGN_CENTER : LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
            lv_obj_set_style_bg_opa(object, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(object, 0, 0);
            lv_obj_set_style_radius(object, 0, 0);
            lv_obj_set_style_pad_all(object, dynamic_cast<Layout *>(dom_parent) ? 0 : 11, 0);
            lv_obj_set_style_pad_row(object, 6, 0); lv_obj_set_style_pad_column(object, 6, 0);
            lv_obj_set_size(object, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_scrollable(object, false);
        }
        void layoutChild(Node * child) override {
            if(!child->ui || child->is_outoforder || dynamic_cast<Divider *>(child)) return;
            auto object = static_cast<lv_obj_t *>(child->outer());
            if(horizontal) {
                if(!child->layout_height && !dynamic_cast<Spacer *>(child))
                    lv_obj_set_height(object, dynamic_cast<Layout *>(child) ? lv_pct(100) : LV_SIZE_CONTENT);
            } else if(!child->layout_width) lv_obj_set_width(object, lv_pct(100));
        }
    };
    class HBox: public Box { public: HBox(): Box(true) {} };
    class VBox: public Box { public: VBox(): Box(false) {} };
}}
namespace CPPUI {
    inline LVGL::HBox & HBox() { return *new LVGL::HBox(); }
    inline LVGL::VBox & VBox() { return *new LVGL::VBox(); }
}
#include "stack.hpp"
#include "grid.hpp"
#endif

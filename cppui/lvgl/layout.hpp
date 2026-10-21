#ifndef CPPUI_LVGL_LAYOUT
#define CPPUI_LVGL_LAYOUT

#include "base.hpp"
#include "spacer.hpp"
#include "divider.hpp"

namespace CPPUI { namespace LVGL {
    class Box: public Layout {
        protected:
        bool horizontal, inverse;
        public:
        explicit Box(bool horizontal,bool inverse=false): horizontal(horizontal),inverse(inverse) {}
        void update(Node * previous) override {
            ui = previous && previous->ui ? previous->ui : lv_obj_create(static_cast<lv_obj_t *>(parent->inner()));
            auto object = static_cast<lv_obj_t *>(ui);
            lv_obj_set_flex_flow(object, horizontal ? (inverse ? LV_FLEX_FLOW_ROW_REVERSE : LV_FLEX_FLOW_ROW)
                : (inverse ? LV_FLEX_FLOW_COLUMN_REVERSE : LV_FLEX_FLOW_COLUMN));
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
                if(!child->layout_height && !dynamic_cast<Spacer *>(child)) {
                    child->saveStyle(LV_STYLE_HEIGHT);
                    lv_obj_set_height(object, dynamic_cast<Layout *>(child) ? lv_pct(100) : LV_SIZE_CONTENT);
                }
            } else if(!child->layout_width) {
                // Restore the child's intrinsic size before the next update:
                // an adaptive column may become a row without remounting it.
                child->saveStyle(LV_STYLE_WIDTH);
                lv_obj_set_width(object, lv_pct(100));
            }
        }
    };
    class HBox: public Box { public: explicit HBox(bool inverse=false): Box(true,inverse) {} };
    class VBox: public Box { public: explicit VBox(bool inverse=false): Box(false,inverse) {} };
    class HVBox: public Box {
        public:
        explicit HVBox(int minWidth,bool inverseH=false,bool inverseV=false):Box(false) { horizontal=::CPPUI::detail::adaptiveHorizontal(*this,minWidth,true);inverse=horizontal ? inverseH : inverseV; }
    };
    class VHBox: public Box {
        public:
        explicit VHBox(int minHeight,bool inverseH=false,bool inverseV=false):Box(true) { horizontal=::CPPUI::detail::adaptiveHorizontal(*this,minHeight,false);inverse=horizontal ? inverseH : inverseV; }
    };
}}
namespace CPPUI {
    inline LVGL::HBox & HBox(bool inverse=false) { return *new LVGL::HBox(inverse); }
    inline LVGL::VBox & VBox(bool inverse=false) { return *new LVGL::VBox(inverse); }
    inline LVGL::HVBox & HVBox(int minWidth,bool inverseH=false,bool inverseV=false) { return *new LVGL::HVBox(minWidth,inverseH,inverseV); }
    inline LVGL::VHBox & VHBox(int minHeight,bool inverseH=false,bool inverseV=false) { return *new LVGL::VHBox(minHeight,inverseH,inverseV); }
}
#include "stack.hpp"
#include "grid.hpp"
#endif

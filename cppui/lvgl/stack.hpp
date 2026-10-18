#ifndef CPPUI_LVGL_STACK
#define CPPUI_LVGL_STACK

#include "base.hpp"
#include "spacer.hpp"

namespace CPPUI { namespace LVGL {
    // One native grid cell is shared by every layer. Child order is z-order.
    class Stack: public Layout {
        bool configured=false, fillX=false, fillY=false, configuring=false, remeasure=false;
        static const int32_t * track(bool fill) {
            static const int32_t content[]={LV_GRID_CONTENT,LV_GRID_TEMPLATE_LAST};
            static const int32_t stretch[]={LV_GRID_FR(1),LV_GRID_TEMPLATE_LAST};
            return fill ? stretch : content;
        }
        bool fills(lv_obj_t * object,bool horizontal) const {
            const auto size=horizontal ? lv_obj_get_style_width(object,LV_PART_MAIN) : lv_obj_get_style_height(object,LV_PART_MAIN);
            if(size!=LV_SIZE_CONTENT) return true;
            auto owner=lv_obj_get_parent(object);
            if(!owner) return false;
            const auto layout=lv_obj_get_style_layout(owner,LV_PART_MAIN);
            if(layout==LV_LAYOUT_FLEX && lv_obj_get_style_flex_grow(object,LV_PART_MAIN)>0) {
                const bool row=(lv_obj_get_style_flex_flow(owner,LV_PART_MAIN)&LV_FLEX_COLUMN)==0;
                if(row==horizontal) return true;
            }
            if(layout==LV_LAYOUT_GRID) {
                const auto align=horizontal ? lv_obj_get_style_grid_cell_x_align(object,LV_PART_MAIN) : lv_obj_get_style_grid_cell_y_align(object,LV_PART_MAIN);
                if(align==LV_GRID_ALIGN_STRETCH) return true;
            }
            return false;
        }
        void place(lv_obj_t * child) {
            lv_obj_set_grid_cell(child,fillX ? LV_GRID_ALIGN_STRETCH : LV_GRID_ALIGN_START,0,1,
                                      fillY ? LV_GRID_ALIGN_STRETCH : LV_GRID_ALIGN_START,0,1);
        }
        void configure() {
            if(!ui || configuring) return;
            auto object=static_cast<lv_obj_t *>(ui);
            const bool x=fills(object,true), y=fills(object,false);
            if(configured && x==fillX && y==fillY) return;
            configuring=true;
            remeasure=remeasure || (configured && ((fillX && !x) || (fillY && !y)));
            fillX=x;fillY=y;configured=true;
            lv_obj_set_grid_dsc_array(object,track(fillX),track(fillY));
            for(uint32_t i=0;i<lv_obj_get_child_count(object);++i) {
                auto child=lv_obj_get_child(object,i);
                if(!lv_obj_is_ignore_layout(child)) place(child);
            }
            configuring=false;
        }
        void nativeEvent(lv_event_t * event) override {
            const auto code=lv_event_get_code(event);
            if(code==LV_EVENT_SIZE_CHANGED || code==LV_EVENT_STYLE_CHANGED) configure();
            if(code==LV_EVENT_LAYOUT_CHANGED && remeasure) {
                remeasure=false;
                // Grid has released its stretch sizing flags. Resolve the original
                // child size styles again so a formerly large Stack can shrink.
                auto object=static_cast<lv_obj_t *>(ui);
                for(uint32_t i=0;i<lv_obj_get_child_count(object);++i)
                    lv_obj_mark_layout_as_dirty(lv_obj_get_child(object,i));
            }
            Node::nativeEvent(event);
        }
    public:
        void update(Node * previous) override {
            auto prev=static_cast<Stack *>(previous);
            ui=prev && prev->ui ? prev->ui : lv_obj_create(static_cast<lv_obj_t *>(parent->inner()));
            if(prev && prev->ui) {
                configured=prev->configured;fillX=prev->fillX;fillY=prev->fillY;remeasure=prev->remeasure;
            }
            auto object=static_cast<lv_obj_t *>(ui);
            lv_obj_set_style_bg_opa(object,LV_OPA_TRANSP,0);
            lv_obj_set_style_border_width(object,0,0);
            lv_obj_set_style_radius(object,0,0);
            lv_obj_set_style_pad_all(object,dynamic_cast<Layout *>(dom_parent) ? 0 : 11,0);
            lv_obj_set_style_pad_row(object,0,0);lv_obj_set_style_pad_column(object,0,0);
            lv_obj_set_size(object,LV_SIZE_CONTENT,LV_SIZE_CONTENT);
            lv_obj_set_scrollable(object,false);
            lv_obj_set_grid_dsc_array(object,track(fillX),track(fillY));
        }
        void layoutChild(Node * child) override {
            if(!child->ui || child->is_outoforder || !ui) return;
            configure();
            auto object=static_cast<lv_obj_t *>(child->outer());
            if(dynamic_cast<Spacer *>(child)) {
                lv_obj_set_ignore_layout(object,true);
                lv_obj_set_size(object,0,0);
                return;
            }
            place(object);
            const auto margin=child->layout_margin.valueOr(Insets(0));
            lv_obj_set_style_margin_top(object,margin.top,0);lv_obj_set_style_margin_right(object,margin.right,0);
            lv_obj_set_style_margin_bottom(object,margin.bottom,0);lv_obj_set_style_margin_left(object,margin.left,0);
        }
        void addChild(int index,Node * child) override {
            if(child->is_outoforder) return;
            Node::addChild(index,child);layoutChild(child);
        }
        void postSync() override { configure(); }
        void postUpdate() override { Node::postUpdate();configure(); }
    };
}}
namespace CPPUI { inline LVGL::Stack & Stack() { return *new LVGL::Stack(); } }
#endif

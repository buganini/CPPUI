#ifndef CPPUI_LVGL_LAYOUT
#define CPPUI_LVGL_LAYOUT

#include "base.hpp"
#include "spacer.hpp"
#include "divider.hpp"

namespace CPPUI { namespace LVGL {
    class Box: public Layout {
        protected:
        bool horizontal, inverse;
        bool configuring=false, configured=false, filledMain=false, filledCross=false;
        bool preciseWeights=false;
        int lastBudget=-1;
        int budget(const std::vector<Node *> & mounted,int highest) const {
            auto object=static_cast<lv_obj_t *>(ui);
            int available=horizontal ? lv_obj_get_content_width(object) : lv_obj_get_content_height(object);
            int count=0;
            for(auto child:mounted) if(child->layoutVisible()) {
                ++count;
                auto native=static_cast<lv_obj_t *>(child->outer());
                available-=horizontal ? lv_obj_get_style_margin_left(native,LV_PART_MAIN)+lv_obj_get_style_margin_right(native,LV_PART_MAIN)
                    : lv_obj_get_style_margin_top(native,LV_PART_MAIN)+lv_obj_get_style_margin_bottom(native,LV_PART_MAIN);
                if(child->expansionPriority(horizontal,this)!=highest)
                    available-=horizontal ? lv_obj_get_width(native) : lv_obj_get_height(native);
            }
            available-=std::max(0,count-1)*(horizontal ? lv_obj_get_style_pad_column(object,LV_PART_MAIN) : lv_obj_get_style_pad_row(object,LV_PART_MAIN));
            return std::max(0,available);
        }
        static void physicalChildren(::CPPUI::Node * parent,std::vector<Node *> & result) {
            for(auto declaration:*parent->children) {
                auto child=declaration->getNode();
                if(child->is_outoforder) continue;
                if(child->is_virtual) physicalChildren(child,result);
                else if(auto native=dynamic_cast<Node *>(child)) if(native->ui) result.push_back(native);
            }
        }
        void configure(bool force=false) {
            if(!ui || configuring) return;
            auto object=static_cast<lv_obj_t *>(ui);
            const bool main=fillsAxis(object,horizontal),cross=fillsAxis(object,!horizontal);
            if(!force && configured && main==filledMain && cross==filledCross) return;
            configuring=true;configured=true;filledMain=main;filledCross=cross;
            std::vector<Node *> mounted;physicalChildren(this,mounted);
            int highest=0;
            for(auto child:mounted) if(child->layoutVisible()) highest=std::max(highest,child->expansionPriority(horizontal,this));
            preciseWeights=false;
            int64_t totalWeight=0;
            for(auto child:mounted) if(main && highest>0 && child->layoutVisible() && child->expansionPriority(horizontal,this)==highest) {
                const int weight=std::max(1,child->layout_weight.valueOr(1));
                preciseWeights=preciseWeights || weight>255;totalWeight+=weight;
            }
            lastBudget=budget(mounted,highest);
            int64_t remainingWeight=totalWeight;
            int remainingSpace=lastBudget;
            for(auto child:mounted) {
                auto native=static_cast<lv_obj_t *>(child->outer());
                child->restoreBoxStyles();
                child->saveBoxStyle(LV_STYLE_FLEX_GROW);
                const bool grows=main && child->layoutVisible() && highest>0 && child->expansionPriority(horizontal,this)==highest;
                const int weight=std::max(1,child->layout_weight.valueOr(1));
                lv_obj_set_flex_grow(native,grows ? (preciseWeights ? 1 : weight) : 0);
                // LVGL grow factors are uint8_t. For larger public weights,
                // constrain each grow item to its full-precision pixel share.
                if(grows && preciseWeights) {
                    const int allocated=static_cast<int>((int64_t(remainingSpace)*weight+remainingWeight/2)/remainingWeight);
                    remainingSpace-=allocated;remainingWeight-=weight;
                    child->saveBoxStyle(horizontal ? LV_STYLE_MIN_WIDTH : LV_STYLE_MIN_HEIGHT);
                    child->saveBoxStyle(horizontal ? LV_STYLE_MAX_WIDTH : LV_STYLE_MAX_HEIGHT);
                    if(horizontal) {
                        lv_obj_set_style_min_width(native,allocated,0);lv_obj_set_style_max_width(native,allocated,0);
                    } else {
                        lv_obj_set_style_min_height(native,allocated,0);lv_obj_set_style_max_height(native,allocated,0);
                    }
                }
                const auto mainProperty=horizontal ? LV_STYLE_WIDTH : LV_STYLE_HEIGHT;
                const auto mainSize=horizontal ? lv_obj_get_style_width(native,LV_PART_MAIN) : lv_obj_get_style_height(native,LV_PART_MAIN);
                if(!grows && LV_COORD_IS_PCT(mainSize)) {
                    child->saveBoxStyle(mainProperty);
                    if(horizontal) lv_obj_set_width(native,LV_SIZE_CONTENT);else lv_obj_set_height(native,LV_SIZE_CONTENT);
                }
                if(cross && child->expansionPriority(!horizontal,this)>0
                    && !(horizontal ? bool(child->layout_height) : bool(child->layout_width))) {
                    child->saveBoxStyle(horizontal ? LV_STYLE_HEIGHT : LV_STYLE_WIDTH);
                    if(horizontal) lv_obj_set_height(native,lv_pct(100));
                    else lv_obj_set_width(native,lv_pct(100));
                }
                lv_obj_mark_layout_as_dirty(native);
            }
            configuring=false;
        }
        void nativeEvent(lv_event_t * event) override {
            const auto code=lv_event_get_code(event);
            if(code==LV_EVENT_SIZE_CHANGED || code==LV_EVENT_STYLE_CHANGED) configure(preciseWeights && code==LV_EVENT_SIZE_CHANGED);
            if(code==LV_EVENT_LAYOUT_CHANGED && preciseWeights && !configuring) {
                std::vector<Node *> mounted;physicalChildren(this,mounted);
                int highest=0;
                for(auto child:mounted) if(child->layoutVisible()) highest=std::max(highest,child->expansionPriority(horizontal,this));
                if(budget(mounted,highest)!=lastBudget) configure(true);
            }
            Node::nativeEvent(event);
        }
        public:
        int baseExpansion(bool x) const override { return x==horizontal ? 3 : 1; }
        bool expansionAxis(bool x) const override { return x==horizontal; }
        bool reflowChildren() const override { return true; }
        void postSync() override { configure(true); }
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
        void layoutChild(Node *) override { configure(true); }

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

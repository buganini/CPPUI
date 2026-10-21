#ifndef CPPUI_LVGL_GRID
#define CPPUI_LVGL_GRID

#include "base.hpp"
#include "spacer.hpp"
#include "../grid.hpp"
#include <limits>

namespace CPPUI { namespace LVGL {
    class Grid: public ::CPPUI::detail::GridLayout<Layout> {
        // LVGL borrows the descriptor arrays. Retained declarations share their
        // storage, including while native callbacks run during reconciliation.
        struct Tracks {
            std::vector<int32_t> columns{0,LV_GRID_TEMPLATE_LAST},rows{0,LV_GRID_TEMPLATE_LAST};
            bool updating=false,configured=false,fillX=false,fillY=false,remeasure=false;
        };
        std::shared_ptr<Tracks> tracks{new Tracks()};
        static bool participates(lv_obj_t * object) {
            return !lv_obj_is_ignore_layout(object) && !lv_obj_is_hidden(object) && !lv_obj_is_floating(object);
        }
        static int span(int value) {return std::max(1,value);}
        static void validate(int position,int count) {
            if(position<0 || position>std::numeric_limits<int>::max()-count)
                throw std::out_of_range("LVGL Grid cell position/span exceeds the supported integer range");
        }
        std::vector<int32_t> measure(bool horizontal,bool fill) const {
            struct Item {int start,count,size;};
            std::vector<Item> items;
            int count=horizontal ? std::max(1,grid_columns) : 1;
            auto object=static_cast<lv_obj_t *>(ui);
            const int gap=horizontal ? lv_obj_get_style_pad_column(object,LV_PART_MAIN) : lv_obj_get_style_pad_row(object,LV_PART_MAIN);
            for(uint32_t i=0;i<lv_obj_get_child_count(object);++i) {
                auto child=lv_obj_get_child(object,i);
                if(!participates(child)) continue;
                const int start=horizontal ? lv_obj_get_style_grid_cell_column_pos(child,LV_PART_MAIN) : lv_obj_get_style_grid_cell_row_pos(child,LV_PART_MAIN);
                const int n=span(horizontal ? lv_obj_get_style_grid_cell_column_span(child,LV_PART_MAIN) : lv_obj_get_style_grid_cell_row_span(child,LV_PART_MAIN));
                validate(start,n);
                count=std::max(count,start+n);
                const int size=horizontal ? lv_obj_get_width(child)+lv_obj_get_style_margin_left(child,LV_PART_MAIN)+lv_obj_get_style_margin_right(child,LV_PART_MAIN)
                                          : lv_obj_get_height(child)+lv_obj_get_style_margin_top(child,LV_PART_MAIN)+lv_obj_get_style_margin_bottom(child,LV_PART_MAIN);
                items.push_back({start,n,size});
            }
            std::vector<int32_t> result(count,fill ? LV_GRID_FR(1) : 0);
            if(!fill) {
                // Native LV_GRID_CONTENT ignores spanning items. Measure single
                // cells first, then share each span's unmet extent across its tracks.
                std::stable_sort(items.begin(),items.end(),[](const Item & a,const Item & b){return a.count<b.count;});
                for(const auto & item:items) {
                    int64_t available=int64_t(gap)*(item.count-1);
                    for(int i=0;i<item.count;++i) available+=result[item.start+i];
                    int64_t extra=std::max<int64_t>(0,int64_t(item.size)-available);
                    for(int i=0;i<item.count;++i) {
                        const auto share=extra/(item.count-i);
                        result[item.start+i]+=static_cast<int32_t>(share);extra-=share;
                    }
                }
            }
            result.push_back(LV_GRID_TEMPLATE_LAST);
            return result;
        }
        void configure() {
            if(!ui || tracks->updating) return;
            auto object=static_cast<lv_obj_t *>(ui);
            const bool x=fillsAxis(object,true),y=fillsAxis(object,false);
            auto columns=measure(true,x),rows=measure(false,y);
            if(tracks->configured && x==tracks->fillX && y==tracks->fillY && columns==tracks->columns && rows==tracks->rows) return;
            tracks->updating=true;
            tracks->remeasure=tracks->remeasure || (tracks->configured && ((tracks->fillX && !x) || (tracks->fillY && !y)));
            tracks->fillX=x;tracks->fillY=y;tracks->configured=true;
            // Keep old arrays alive until both native style pointers are replaced.
            tracks->columns.swap(columns);tracks->rows.swap(rows);
            lv_obj_set_grid_dsc_array(object,tracks->columns.data(),tracks->rows.data());
            for(uint32_t i=0;i<lv_obj_get_child_count(object);++i) {
                auto child=lv_obj_get_child(object,i);
                if(lv_obj_is_ignore_layout(child)) continue;
                lv_obj_set_style_grid_cell_x_align(child,x ? LV_GRID_ALIGN_STRETCH : LV_GRID_ALIGN_START,0);
                lv_obj_set_style_grid_cell_y_align(child,y ? LV_GRID_ALIGN_STRETCH : LV_GRID_ALIGN_START,0);
            }
            tracks->updating=false;
        }
        void nativeEvent(lv_event_t * event) override {
            const auto code=lv_event_get_code(event);
            if(code==LV_EVENT_LAYOUT_CHANGED && tracks->remeasure) {
                tracks->remeasure=false;
                // Native grid has released its stretch sizing flags.
                auto object=static_cast<lv_obj_t *>(ui);
                for(uint32_t i=0;i<lv_obj_get_child_count(object);++i)
                    lv_obj_mark_layout_as_dirty(lv_obj_get_child(object,i));
            }
            if(code==LV_EVENT_SIZE_CHANGED || code==LV_EVENT_STYLE_CHANGED || code==LV_EVENT_CHILD_CHANGED
               || code==LV_EVENT_CHILD_DELETED || code==LV_EVENT_LAYOUT_CHANGED) configure();
            Node::nativeEvent(event);
        }
    public:
        Grid() {is_grid_layout=true;}
        void destroy(bool direct) override {
            if(ui && !direct) {
                // C++ descendants retire before their parent's native subtree.
                // Detach borrowed arrays before their storage can be reclaimed;
                // native delete listeners may still inspect this widget's styles.
                static const int32_t empty[]={0,LV_GRID_TEMPLATE_LAST};
                tracks->updating=true;
                lv_obj_set_grid_dsc_array(static_cast<lv_obj_t *>(ui),empty,empty);
                tracks->updating=false;
            }
            Node::destroy(direct);
        }
        void update(Node * previous) override {
            auto prev=static_cast<Grid *>(previous);
            retainGrid(prev && prev->ui ? prev : nullptr);
            if(prev) tracks=prev->tracks;
            ui=prev && prev->ui ? prev->ui : lv_obj_create(static_cast<lv_obj_t *>(parent->inner()));
            auto object=static_cast<lv_obj_t *>(ui);
            lv_obj_set_style_bg_opa(object,LV_OPA_TRANSP,0);
            lv_obj_set_style_border_width(object,0,0);lv_obj_set_style_radius(object,0,0);
            lv_obj_set_style_pad_all(object,dynamic_cast<Layout *>(dom_parent) ? 0 : 11,0);
            lv_obj_set_style_pad_row(object,6,0);lv_obj_set_style_pad_column(object,6,0);
            lv_obj_set_size(object,LV_SIZE_CONTENT,LV_SIZE_CONTENT);
            lv_obj_set_scrollable(object,false);
            lv_obj_set_grid_dsc_array(object,tracks->columns.data(),tracks->rows.data());
        }
        void layoutChild(Node * child) override {
            // Automatic cells are assigned together after child reconciliation.
            if(grid_columns) return;
            placeChild(child,GridCell(child->grid_row,child->grid_column,
                                     span(child->grid_rowspan),span(child->grid_columnspan)));
        }
        void placeChild(Node * child,const GridCell & cell) {
            if(!ui || !child->ui || child->is_outoforder) return;
            auto object=static_cast<lv_obj_t *>(child->outer());
            if(dynamic_cast<Spacer *>(child)) {
                lv_obj_set_ignore_layout(object,true);lv_obj_set_size(object,0,0);return;
            }
            const int columns=cell.columnspan,rows=cell.rowspan;
            validate(cell.column,columns);validate(cell.row,rows);
            lv_obj_set_ignore_layout(object,false);
            lv_obj_set_grid_cell(object,tracks->fillX ? LV_GRID_ALIGN_STRETCH : LV_GRID_ALIGN_START,cell.column,columns,
                                       tracks->fillY ? LV_GRID_ALIGN_STRETCH : LV_GRID_ALIGN_START,cell.row,rows);
            const auto margin=child->layout_margin.valueOr(Insets(0));
            lv_obj_set_style_margin_top(object,margin.top,0);lv_obj_set_style_margin_right(object,margin.right,0);
            lv_obj_set_style_margin_bottom(object,margin.bottom,0);lv_obj_set_style_margin_left(object,margin.left,0);
            configure();
        }
        void addChild(int index,Node * child) override {
            if(child->is_outoforder) return;
            mountGrid(index,child);
            Node::addChild(index,child);layoutChild(child);
        }
        void removeChild(int index,Node * child) override {
            if(child->is_outoforder) return;
            unmountGrid(index,child);
            if(child->ui) lv_obj_set_ignore_layout(static_cast<lv_obj_t *>(child->outer()),true);
            Node::removeChild(index,child);
        }
        void postSync() override {
            for(const auto & item:cells()) placeChild(static_cast<Node *>(item.node),item.cell);
            configure();
        }
        void postUpdate() override {Node::postUpdate();configure();}
    };
}}
namespace CPPUI { inline LVGL::Grid & Grid() {return *new LVGL::Grid();} }
#endif

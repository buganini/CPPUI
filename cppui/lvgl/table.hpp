#ifndef CPPUI_LVGL_TABLE
#define CPPUI_LVGL_TABLE
#include "base.hpp"
#include "../binding.hpp"
#include "../table_adapter.hpp"
// LVGL 9.6 has no public cell rectangle API. The overlay uses the same
// measured rows/columns as lv_table; no second copy of text layout is needed.
#include <src/widgets/table/lv_table_private.h>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <limits>
#include <type_traits>
#include <cstdint>
namespace CPPUI { namespace LVGL {
namespace detail {
    template<class T> struct TableValue {
        static_assert(std::is_arithmetic<T>::value,"Table cells support arithmetic values and std::string");
        static std::string encode(T value){std::ostringstream out;out<<std::setprecision(std::numeric_limits<T>::max_digits10)<<+value;return out.str();}
        static T decode(const std::string & value){std::istringstream in(value);long double parsed;if(!(in>>parsed))throw std::invalid_argument("Invalid table value");in>>std::ws;
            if(!in.eof()||!std::isfinite(parsed)||parsed<std::numeric_limits<T>::lowest()||parsed>std::numeric_limits<T>::max()||(std::is_integral<T>::value&&std::floor(parsed)!=parsed))throw std::invalid_argument("Invalid table value");return static_cast<T>(parsed);}
    };
    template<> struct TableValue<std::string> {static std::string encode(const std::string & v){return v;}static std::string decode(const std::string & v){return v;}};
    template<> struct TableValue<bool> {static std::string encode(bool v){return v?"True":"False";}static bool decode(const std::string & v){if(v=="True"||v=="true"||v=="1")return true;if(v=="False"||v=="false"||v=="0")return false;throw std::invalid_argument("Invalid boolean table value");}};
    struct TableCell {
        std::string value;
        OptionalText display;
        std::string prefix, suffix, placeholder;
        Optional<std::vector<std::pair<std::string,std::string>>> choices;
        std::function<std::string()> read,displayRead;
        std::function<std::string(const std::string &)> validate;
        std::function<void(const std::string &)> write;
        std::function<void()> clicked,doubleClicked;
        std::string current() const {return read?read():value;}
    };
}
    class TableNodeBase: public ::CPPUI::Node {
        public:
        detail::TableCell cell;
        void destroy(bool) override {}
    };
    template<class T> class TableNode: public TableNodeBase {
        bool bound = false;
        public:
        explicit TableNode(const T & value) { cell.value = detail::TableValue<T>::encode(value); cell.validate = [](const std::string & input) { return detail::TableValue<T>::encode(detail::TableValue<T>::decode(input)); }; if(std::is_same<T,bool>::value) cell.choices = std::vector<std::pair<std::string,std::string>>{{"False","False"},{"True","True"}}; }
        explicit TableNode(Binding<T> model): TableNode(model.get()) {
            bound = true;
            cell.read = [model]() { return detail::TableValue<T>::encode(model.get()); };
            cell.write = [model](const std::string & value) { model.set(detail::TableValue<T>::decode(value)); };
        }
        TableNode & prefix(const std::string & value) { cell.prefix = value; return *this; }
        TableNode & suffix(const std::string & value) { cell.suffix = value; return *this; }
        TableNode & placeholder(const std::string & value) { cell.placeholder = value; return *this; }
        TableNode & display(const std::string & value) { cell.display = value; return *this; }
        TableNode & display(Binding<std::string> value) { return display(value.get()); }
        TableNode & set(std::function<void(const T &)> callback) {
            if(!bound) cell.write = callback ? std::function<void(const std::string &)>([callback](const std::string & value) {
                callback(detail::TableValue<T>::decode(value));
            }) : std::function<void(const std::string &)>();
            return *this;
        }
        TableNode & choices(const std::vector<T> & values) {
            std::vector<std::pair<std::string, std::string>> items;
            for(const auto & value: values) {
                auto encoded = detail::TableValue<T>::encode(value);
                items.emplace_back(encoded, encoded);
            }
            cell.choices = std::move(items); return *this;
        }
        TableNode & choices(const std::vector<std::pair<T, std::string>> & values) {
            std::vector<std::pair<std::string, std::string>> items;
            for(const auto & value: values) items.emplace_back(detail::TableValue<T>::encode(value.first), value.second);
            cell.choices = std::move(items); return *this;
        }
        TableNode & dblclick(std::function<void()> callback) { cell.doubleClicked = std::move(callback); return *this; }
    };


namespace detail {
    inline std::string tableText(const TableCell & cell) {
        std::string text;
        if(cell.display) text=*cell.display;
        else if(cell.displayRead) text=cell.displayRead();
        else {
            text=cell.current();
            if(cell.choices) for(const auto & item:*cell.choices) if(item.first==text) {text=item.second;break;}
        }
        return cell.prefix+text+cell.suffix;
    }
    struct TableNative: std::enable_shared_from_this<TableNative> {
        lv_obj_t * object=nullptr,* editor=nullptr,* field=nullptr;
        std::vector<std::vector<TableCell>> cells;
        int rows=0,columns=0,rowOffset=0,columnOffset=0;
        int selectedRow=-1,selectedColumn=-1,editRow=-1,editColumn=-1,choice=-1;
        std::uint64_t generation=0;
        bool active=true,choiceEditor=false,keyboardEdit=false;
        std::string original;
        std::vector<std::pair<std::string,std::string>> items;
        const TableCell * cell(int row,int column) const {
            return active && row>=0 && column>=0 && row<rows && column<columns
                && row<int(cells.size()) && column<int(cells[row].size()) ? &cells[row][column] : nullptr;
        }
        static void report() {
            try {throw;} catch(const std::exception & error) {std::fprintf(stderr,"CPPUI LVGL Table: %s\n",error.what());}
            catch(...) {std::fprintf(stderr,"CPPUI LVGL Table callback failed\n");}
        }
        void closeEditor(bool focus=false) {
            auto panel=editor;
            editor=nullptr;field=nullptr;editRow=editColumn=-1;items.clear();
            // Clear the editor before changing focus or deleting its children:
            // both operations can synchronously emit DEFOCUSED.
            if(focus && object && active) lv_group_focus_obj(object);
            if(panel) lv_obj_delete(panel);
        }
        void deactivate() {active=false;++generation;closeEditor();cells.clear();}
        void renderCell(int row,int column) {
            if(auto entry=cell(row,column)) {
                auto text=tableText(*entry);
                lv_table_set_cell_value(object,row+rowOffset,column+columnOffset,text.c_str());
            }
        }
        bool edit(int row,int column,const std::string & text) {
            auto found=cell(row,column);if(!found || !found->write) return false;
            const auto entry=*found;const auto version=generation;
            try {
                const auto value=entry.validate ? entry.validate(text) : text;
                if(value!=entry.current()) entry.write(value);
                if(active && object && version==generation) renderCell(row,column);
                return true;
            } catch(...) {report();return false;}
        }
        void commit(bool focus,bool acceptChoice=false) {
            if(!field) return;
            const int row=editRow,column=editColumn;
            std::string value;
            bool valid=true;
            if(choiceEditor) {
                int index=acceptChoice ? int(lv_dropdown_get_selected(field)) : choice;
                valid=index>=0 && index<int(items.size());
                if(valid) value=items[index].first;
            } else value=lv_textarea_get_text(field);
            closeEditor(focus);
            if(valid) edit(row,column,value);
        }
        static void editorEvent(lv_event_t * event) {
            auto holder=static_cast<std::shared_ptr<TableNative> *>(lv_event_get_user_data(event));
            auto self=*holder;
            const auto code=lv_event_get_code(event);
            if(code==LV_EVENT_DELETE) {delete holder;return;}
            if(!self->active || lv_event_get_target_obj(event)!=self->field) return;
            try {
                if(code==LV_EVENT_KEY && lv_event_get_key(event)==LV_KEY_ESC) self->closeEditor(true);
                else if(code==LV_EVENT_DEFOCUSED) self->commit(false);
                else if(!self->choiceEditor && code==LV_EVENT_READY) self->commit(true);
                else if(self->choiceEditor && code==LV_EVENT_VALUE_CHANGED) self->commit(true,true);
                // LVGL omits VALUE_CHANGED when accepting the current option
                // (also option zero for an unknown value). Still close the editor.
                else if(self->choiceEditor && !lv_dropdown_is_open(self->field)
                    && (code==LV_EVENT_RELEASED || (code==LV_EVENT_KEY && lv_event_get_key(event)==LV_KEY_ENTER
                        && lv_indev_get_active_obj()!=self->field)))
                    self->commit(true,true);
            } catch(...) {report();}
        }
        void positionEditor() {
            if(!editor || !object) return;
            const int r=editRow+rowOffset,c=editColumn+columnOffset;
            auto table=reinterpret_cast<lv_table_t *>(object);
            int x=0,y=0;
            for(int i=0;i<c;++i) x+=table->col_w[i];
            for(int i=0;i<r;++i) y+=table->row_h[i];
            lv_obj_set_pos(editor,x,y);
            lv_obj_set_size(editor,table->col_w[c],table->row_h[r]);
        }
        bool beginEdit(int row,int column) {
            auto found=cell(row,column);if(!object || !found || !found->write) return false;
            const auto entry=*found;
            const auto value=entry.current();
            closeEditor();
            selectedRow=row;selectedColumn=column;
            lv_table_set_selected_cell(object,row+rowOffset,column+columnOffset);
            editRow=row;editColumn=column;original=value;
            editor=lv_obj_create(object);
            lv_obj_set_ignore_layout(editor,true);
            lv_obj_set_scrollable(editor,false);
            lv_obj_set_style_pad_all(editor,0,0);
            lv_obj_set_style_border_width(editor,0,0);
            lv_obj_set_style_radius(editor,0,0);
            lv_obj_set_style_pad_column(editor,2,0);
            lv_obj_set_flex_flow(editor,LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(editor,LV_FLEX_ALIGN_START,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);
            if(!entry.prefix.empty()) lv_label_set_text(lv_label_create(editor),entry.prefix.c_str());
            choiceEditor=bool(entry.choices);choice=-1;
            field=choiceEditor ? lv_dropdown_create(editor) : lv_textarea_create(editor);
            lv_obj_set_flex_grow(field,1);lv_obj_set_width(field,0);lv_obj_set_height(field,lv_pct(100));
            lv_obj_set_style_pad_all(field,2,0);lv_obj_set_style_radius(field,0,0);
            if(choiceEditor) {
                items=*entry.choices;
                std::string options;
                for(std::size_t i=0;i<items.size();++i) {
                    if(i) options+='\n';
                    auto label=items[i].second;std::replace(label.begin(),label.end(),'\n',' ');options+=label;
                    if(choice<0 && items[i].first==value) choice=int(i);
                }
                lv_dropdown_set_options(field,options.c_str());
                if(choice>=0) lv_dropdown_set_selected(field,choice);
                lv_dropdown_set_selected_highlight(field,choice>=0);
                lv_dropdown_set_text(field,choice>=0 ? nullptr : original.c_str());
            } else {
                lv_textarea_set_one_line(field,true);
                lv_textarea_set_text(field,value.c_str());
                lv_textarea_set_placeholder_text(field,entry.placeholder.c_str());
                lv_textarea_set_cursor_pos(field,LV_TEXTAREA_CURSOR_LAST);
            }
            if(!entry.suffix.empty()) lv_label_set_text(lv_label_create(editor),entry.suffix.c_str());
            positionEditor();
            lv_obj_add_event_cb(field,editorEvent,LV_EVENT_ALL,new std::shared_ptr<TableNative>(shared_from_this()));
            lv_group_focus_obj(field);
            return true;
        }
        void dispatch(bool twice) {
            auto entry=cell(selectedRow,selectedColumn);if(!entry) return;
            const int row=selectedRow,column=selectedColumn;
            auto callback=twice ? entry->doubleClicked : entry->clicked;
            const auto version=generation;
            if(callback) callback();
            if(twice && active && object && version==generation) beginEdit(row,column);
        }
        static void event(lv_event_t * event) {
            auto holder=static_cast<std::shared_ptr<TableNative> *>(lv_event_get_user_data(event));
            auto self=*holder;
            const auto code=lv_event_get_code(event);
            if(code==LV_EVENT_DELETE) {
                self->object=nullptr;self->editor=nullptr;self->field=nullptr;
                self->active=false;++self->generation;self->cells.clear();delete holder;return;
            }
            if(!self->active || lv_event_get_target_obj(event)!=self->object) return;
            try {
                if(code==LV_EVENT_VALUE_CHANGED) {
                    uint32_t row,column;lv_table_get_selected_cell(self->object,&row,&column);
                    self->selectedRow=int(row)-self->rowOffset;self->selectedColumn=int(column)-self->columnOffset;
                } else if(code==LV_EVENT_RELEASED && self->selectedRow>=0 && self->selectedColumn>=0) {
                    // Native pointer release clears its selection after VALUE_CHANGED.
                    lv_table_set_selected_cell(self->object,self->selectedRow+self->rowOffset,self->selectedColumn+self->columnOffset);
                    if(self->keyboardEdit) {
                        self->keyboardEdit=false;
                        self->beginEdit(self->selectedRow,self->selectedColumn);
                    }
                } else if(code==LV_EVENT_SHORT_CLICKED) self->dispatch(false);
                else if(code==LV_EVENT_DOUBLE_CLICKED) self->dispatch(true);
                else if(code==LV_EVENT_KEY && lv_event_get_key(event)==LV_KEY_ENTER) {
                    uint32_t row,column;lv_table_get_selected_cell(self->object,&row,&column);
                    // A real keypad release must still target the table. Moving
                    // focus on press would deliver that release to the dropdown.
                    if(lv_indev_get_active_obj()==self->object) self->keyboardEdit=true;
                    else self->beginEdit(int(row)-self->rowOffset,int(column)-self->columnOffset);
                } else if(code==LV_EVENT_SIZE_CHANGED) self->positionEditor();
                else if(code==LV_EVENT_DRAW_TASK_ADDED) {
                    auto task=lv_event_get_draw_task(event);
                    auto descriptor=static_cast<lv_draw_dsc_base_t *>(lv_draw_task_get_draw_dsc(task));
                    if(descriptor->part==LV_PART_ITEMS && ((self->rowOffset && descriptor->id1==0)
                        || (self->columnOffset && descriptor->id2==0))) {
                        if(auto fill=lv_draw_task_get_fill_dsc(task)) {
                            fill->color=lv_color_mix(lv_obj_get_style_text_color(self->object,LV_PART_ITEMS),
                                lv_obj_get_style_bg_color(self->object,LV_PART_MAIN),24);
                            fill->opa=LV_OPA_COVER;
                        }
                    }
                }
            } catch(...) {report();}
        }
    };
}
class Table: public Node {
    std::shared_ptr<detail::TableNative> native;
    std::vector<std::vector<detail::TableCell>> prepared;
    std::vector<std::vector<std::string>> text;
    Optional<std::vector<std::string>> colLabels,rowLabels;
    int nr=0,nc=0;
        bool autofit;
        Optional<std::vector<std::string>> columns, headings;
        Optional<int> height, width;
        std::shared_ptr<void> adapterOwner;
        std::vector<std::vector<detail::TableCell>> adapterRows;
        std::vector<OptionalText> adapterColumns, adapterHeadings;
        int adapterHeight = 0, adapterWidth = 0;
        static Optional<std::vector<std::string>> headers(const std::vector<OptionalText> & source,
                                                         const Optional<std::vector<std::string>> & fallback) {
            auto result = fallback.valueOr({});
            bool visible = bool(fallback);
            result.resize(std::max(result.size(), source.size()));
            for(std::size_t i = 0; i < source.size(); ++i) if(source[i]) { result[i] = *source[i]; visible = true; }
            return visible ? Optional<std::vector<std::string>>(std::move(result)) : Optional<std::vector<std::string>>();
        }
        public:
        explicit Table(bool autofit = true): autofit(autofit) { is_terminal = true; expandX=expandY=4; }
        template<class Adapter> explicit Table(std::shared_ptr<Adapter> source, bool autofit = true): Table(autofit) {
            using T = typename Adapter::Value;
            std::shared_ptr<BaseTableAdapter<T>> adapter = std::move(source);
            if(!adapter) throw std::invalid_argument("Table adapter must not be null");
            adapterOwner = adapter;
            adapterHeight = adapter->rowCount(); adapterWidth = adapter->columnCount();
            if(adapterHeight < 0 || adapterWidth < 0 || adapterHeight > LV_TABLE_CELL_NONE || adapterWidth > LV_TABLE_CELL_NONE) throw std::invalid_argument("LVGL Table adapter dimensions must be between 0 and 65535");
            for(int column = 0; column < adapterWidth; ++column) adapterColumns.push_back(adapter->columnHeader(column));
            for(int row = 0; row < adapterHeight; ++row) {
                adapterHeadings.push_back(adapter->rowHeader(row));
                adapterRows.emplace_back();
                for(int column = 0; column < adapterWidth; ++column) {
                    detail::TableCell cell;
                    cell.validate = [](const std::string & input) { return detail::TableValue<T>::encode(detail::TableValue<T>::decode(input)); };
                    if(std::is_same<T,bool>::value) cell.choices = std::vector<std::pair<std::string,std::string>>{{"False","False"},{"True","True"}};
                    cell.read = [adapter, row, column]() { return detail::TableValue<T>::encode(adapter->editData(row, column)); };
                    cell.displayRead = [adapter, row, column]() { return detail::TableValue<T>::encode(adapter->data(row, column)); };
                    // Read while building, so State reads inside the adapter
                    // subscribe this view and failures leave the old UI intact.
                    cell.read(); cell.displayRead();
                    if(adapter->editable(row, column)) cell.write = [adapter, row, column](const std::string & value) {
                        adapter->setData(row, column, detail::TableValue<T>::decode(value));
                    };
                    cell.clicked = [adapter, row, column]() { adapter->clicked(row, column); };
                    cell.doubleClicked = [adapter, row, column]() { adapter->dblclicked(row, column); };
                    adapterRows.back().push_back(std::move(cell));
                }
            }
        }
        Table & columnHeader(const std::vector<std::string> & labels) { columns = labels; return *this; }
        Table & rowHeader(const std::vector<std::string> & labels) { headings = labels; return *this; }
        Table & rowCount(int value) { if(value < 0) throw std::invalid_argument("Table row count must be non-negative"); height = value; return *this; }
        Table & columnCount(int value) { if(value < 0) throw std::invalid_argument("Table column count must be non-negative"); width = value; return *this; }

        void prepare() override {
            prepared.clear();
            if(adapterOwner) prepared=adapterRows;
            else for(auto row:*children) {
                if(!dynamic_cast<TableNodeBase *>(row)) throw std::logic_error("Table requires TableNode rows");
                prepared.emplace_back();
                for(auto node:*row->children) {
                    auto entry=dynamic_cast<TableNodeBase *>(node);
                    if(!entry) throw std::logic_error("Table rows require TableNode cells");
                    prepared.back().push_back(entry->cell);prepared.back().back().clicked=entry->onClicked;
                }
            }
            colLabels=adapterOwner ? headers(adapterColumns,columns) : columns;
            rowLabels=adapterOwner ? headers(adapterHeadings,headings) : headings;
            nr=adapterOwner ? adapterHeight : height.valueOr(std::max(int(prepared.size()),rowLabels ? int(rowLabels->size()) : 0));
            nc=colLabels ? int(colLabels->size()) : 0;
            for(const auto & row:prepared) nc=std::max(nc,int(row.size()));
            nc=adapterOwner ? adapterWidth : width.valueOr(nc);
            if(nr>LV_TABLE_CELL_NONE-int(bool(colLabels)) || nc>LV_TABLE_CELL_NONE-int(bool(rowLabels)))
                throw std::invalid_argument("LVGL Table dimensions including headers must not exceed 65535");
            text.assign(nr+int(bool(colLabels)),std::vector<std::string>(nc+int(bool(rowLabels))));
            if(colLabels) for(int c=0;c<nc && c<int(colLabels->size());++c) text[0][c+int(bool(rowLabels))]=(*colLabels)[c];
            if(rowLabels) for(int r=0;r<nr && r<int(rowLabels->size());++r) text[r+int(bool(colLabels))][0]=(*rowLabels)[r];
            for(int r=0;r<nr && r<int(prepared.size());++r) for(int c=0;c<nc && c<int(prepared[r].size());++c)
                text[r+int(bool(colLabels))][c+int(bool(rowLabels))]=detail::tableText(prepared[r][c]);
        }
        void update(Node * previous) override {
            auto prev=static_cast<Table *>(previous);
            if(prev && prev->native && prev->native->object) native=prev->native;
            else {
                native=std::make_shared<detail::TableNative>();
                native->object=lv_table_create(static_cast<lv_obj_t *>(parent->inner()));
                lv_obj_set_send_draw_task_events(native->object,true);
                lv_obj_add_event_cb(native->object,detail::TableNative::event,LV_EVENT_ALL,
                    new std::shared_ptr<detail::TableNative>(native));
            }
            auto state=native;state->closeEditor();++state->generation;
            ui=state->object;state->active=true;state->cells=prepared;
            state->rows=nr;state->columns=nc;state->rowOffset=bool(colLabels);state->columnOffset=bool(rowLabels);
            state->selectedRow=state->selectedColumn=-1;state->keyboardEdit=false;
            auto table=reinterpret_cast<lv_table_t *>(ui);table->row_act=table->col_act=LV_TABLE_CELL_NONE;
            lv_table_set_row_count(state->object,nr+state->rowOffset);
            lv_table_set_column_count(state->object,nc+state->columnOffset);
            for(std::size_t r=0;r<text.size();++r) for(std::size_t c=0;c<text[r].size();++c)
                lv_table_set_cell_value(state->object,r,c,text[r][c].c_str());
            lv_obj_set_size(state->object,LV_SIZE_CONTENT,layout_weight && *layout_weight>0 ? 0 : LV_SIZE_CONTENT);
        }
        void postUpdate() override {
            if(ui && autofit) {
                auto object=static_cast<lv_obj_t *>(ui);
                auto font=lv_obj_get_style_text_font(object,LV_PART_ITEMS);
                const int padding=lv_obj_get_style_pad_left(object,LV_PART_ITEMS)+lv_obj_get_style_pad_right(object,LV_PART_ITEMS);
                for(int c=0;c<nc+int(bool(rowLabels));++c) {
                    int width=40;
                    for(const auto & row:text) {
                        lv_point_t size;
                        lv_text_get_size(&size,row[c].c_str(),font,lv_obj_get_style_text_letter_space(object,LV_PART_ITEMS),
                            lv_obj_get_style_text_line_space(object,LV_PART_ITEMS),LV_COORD_MAX,LV_TEXT_FLAG_NONE);
                        width=std::max(width,int(size.x)+padding+2);
                    }
                    lv_table_set_column_width(object,c,width);
                }
            }
            Node::postUpdate();
        }
        bool beginEdit(int row,int column) {auto state=native;return state && state->beginEdit(row,column);}
        bool edit(int row,int column,const std::string & value) {
            auto state=native;
            if(!state || !state->object) return false;
            state->closeEditor();return state->edit(row,column,value);
        }
        lv_obj_t * editor() const {return native ? native->field : nullptr;}
        std::string cellText(int row,int column) const {
            if(!native || !native->object || row<0 || column<0 || row>=nr || column>=nc) return {};
            return lv_table_get_cell_value(native->object,row+native->rowOffset,column+native->columnOffset);
        }
        void destroy(bool direct) override {
            if(native) native->deactivate();
            Node::destroy(direct);native.reset();
        }
};
}}
namespace CPPUI {
    inline LVGL::Table & Table(bool autofit = true) { return *new LVGL::Table(autofit); }
    template<class Adapter> inline LVGL::Table & Table(std::shared_ptr<Adapter> adapter, bool autofit = true) {
        return *new LVGL::Table(std::move(adapter), autofit);
    }
    inline LVGL::TableNode<std::string> & TableNode(const std::string & value = {}) { return *new LVGL::TableNode<std::string>(value); }
    template<class T> inline LVGL::TableNode<T> & TableNode(Binding<T> model) { return *new LVGL::TableNode<T>(model); }
    template<class T> inline LVGL::TableNode<T> & TableNode(State<T> & model) { return TableNode(Binding<T>(model)); }
    template<class T> inline typename std::enable_if<std::is_arithmetic<T>::value, LVGL::TableNode<T> &>::type TableNode(T value) { return *new LVGL::TableNode<T>(value); }
}
#endif

#ifndef CPPUI_WX_TABLE
#define CPPUI_WX_TABLE
#include "base.hpp"
#include "../binding.hpp"
#include "../table_adapter.hpp"
#include <wx/grid.h>
#include <wx/choice.h>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <limits>
#include <type_traits>
#include <cstdint>
namespace CPPUI { namespace Wx {
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
    class TableNodeBase: public Node {
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

class TableModel:public wxGridTableBase {
public:
    std::vector<std::vector<detail::TableCell>> rows;
    Optional<std::vector<std::string>> columns,headings;
    int height=0,width=0;
    bool active=true;
    std::uint64_t generation=0;
    std::shared_ptr<void> adapterOwner;
    const detail::TableCell * cell(int r,int c) const {return active&&r>=0&&c>=0&&r<height&&c<width&&r<int(rows.size())&&c<int(rows[r].size())?&rows[r][c]:nullptr;}
    int GetNumberRows() override{return height;}
    int GetNumberCols() override{return width;}
    bool IsEmptyCell(int r,int c) override{return GetValue(r,c).empty();}
    wxString GetValue(int r,int c) override {
        auto found=cell(r,c);if(!found)return {};auto entry=*found;
        try {
            std::string value;
            if(entry.display) value=*entry.display;
            else if(entry.displayRead) value=entry.displayRead();
            else { value=entry.current(); if(entry.choices)for(auto & choice:*entry.choices)if(choice.first==value){value=choice.second;break;} }
            return wxString::FromUTF8(entry.prefix+value+entry.suffix);
        } catch(...){reportException("Table read");return {};}

    }
    bool edit(int r,int c,const std::string & value){auto found=cell(r,c);if(!found||!found->write)return false;auto entry=*found;try{auto converted=entry.validate?entry.validate(value):value;entry.write(converted);return true;}catch(...){reportException("Table edit");return false;}}
    void SetValue(int r,int c,const wxString & value) override {edit(r,c,value.ToStdString(wxConvUTF8));}
    wxString GetColLabelValue(int c) override{return columns&&c>=0&&c<int(columns->size())?wxString::FromUTF8((*columns)[c]):wxString();}
    wxString GetRowLabelValue(int r) override{return headings&&r>=0&&r<int(headings->size())?wxString::FromUTF8((*headings)[r]):wxString();}
    void dispatch(int r,int c,bool twice){auto found=cell(r,c);if(!found)return;auto callback=twice?found->doubleClicked:found->clicked;try{if(callback)callback();}catch(...){reportException("Table callback");}}
    void deactivate(){active=false;++generation;rows.clear();adapterOwner.reset();}
};
// Editors retain a generation, not a declaration. A redraw discards their draft.
class TableEditor:public wxGridCellEditor {
    bool choice;
    TableModel * model=nullptr;
    std::uint64_t generation=0;
    int row=0,column=0;
    std::string original,pending;
    std::vector<std::pair<std::string,std::string>> items;
    bool valid() const{return model&&model->active&&generation==model->generation&&model->cell(row,column);}
public:
    wxWindow * field = nullptr;
    std::string prefix, suffix, hint;
    explicit TableEditor(bool choice, const std::string & prefix={}, const std::string & suffix={}, const std::string & hint={})
        :choice(choice),prefix(prefix),suffix(suffix),hint(hint){}
    wxGridCellEditor * Clone() const override{return new TableEditor(choice,prefix,suffix,hint);}
    void Create(wxWindow * parent,wxWindowID id,wxEvtHandler * handler) override {
        const bool decorated=!prefix.empty() || !suffix.empty();
        auto panel=decorated ? new wxControl(parent,id) : nullptr;
        auto owner=panel ? panel : parent;
        field=choice?static_cast<wxWindow *>(new wxChoice(owner,panel?wxID_ANY:id))
            :static_cast<wxWindow *>(new wxTextCtrl(owner,panel?wxID_ANY:id,"",wxDefaultPosition,wxDefaultSize,wxTE_PROCESS_ENTER));
        if(!choice)static_cast<wxTextCtrl *>(field)->SetHint(wxString::FromUTF8(hint));
        m_control=panel ? static_cast<wxWindow *>(panel) : field;
        if(panel){
            auto box=new wxBoxSizer(wxHORIZONTAL);
            if(!prefix.empty())box->Add(new wxStaticText(panel,wxID_ANY,wxString::FromUTF8(prefix)),0,wxALIGN_CENTER_VERTICAL);
            box->Add(field,1,wxEXPAND);
            if(!suffix.empty())box->Add(new wxStaticText(panel,wxID_ANY,wxString::FromUTF8(suffix)),0,wxALIGN_CENTER_VERTICAL);
            panel->SetSizer(box);
            field->Bind(wxEVT_KEY_DOWN,[panel](wxKeyEvent & event){if(!panel->GetEventHandler()->ProcessEvent(event)) event.Skip();});
            field->Bind(wxEVT_KILL_FOCUS,[panel](wxFocusEvent & event){if(!panel->GetEventHandler()->ProcessEvent(event)) event.Skip();});
        }
        if(choice)field->Bind(wxEVT_CHOICE,[this](wxCommandEvent &){if(valid()){auto grid=model->GetView();grid->SaveEditControlValue();grid->DisableCellEditControl();}});
        wxGridCellEditor::Create(parent,id,handler);
    }
    void SetSize(const wxRect & rect) override {m_control->SetSize(rect);}
    void BeginEdit(int r,int c,wxGrid * grid) override {
        model=static_cast<TableModel *>(grid->GetTable());generation=model->generation;row=r;column=c;auto found=model->cell(r,c);if(!found)return;auto cell=*found;
        original=cell.current();pending=original;
        if(choice){items=cell.choices.valueOr({});auto control=static_cast<wxChoice *>(field);control->Clear();for(auto & item:items)control->Append(wxString::FromUTF8(item.second));}
        Reset();field->SetFocus();if(!choice)static_cast<wxTextCtrl *>(field)->SelectAll();
    }
    bool EndEdit(int,int,const wxGrid *,const wxString &,wxString * value) override {
        if(!valid())return false;
        if(choice){int index=static_cast<wxChoice *>(field)->GetSelection();if(index<0||index>=int(items.size()))return false;pending=items[index].first;}
        else pending=static_cast<wxTextCtrl *>(field)->GetValue().ToStdString(wxConvUTF8);
        auto cell=*model->cell(row,column);try{if(cell.validate)pending=cell.validate(pending);}catch(...){return false;}
        if(original==pending)return false;*value=wxString::FromUTF8(pending);return true;
    }
    void ApplyEdit(int,int,wxGrid *) override {if(valid()){auto value=pending;original=value;model->edit(row,column,value);}}
    void Reset() override {if(choice){auto control=static_cast<wxChoice *>(field);int selected=wxNOT_FOUND;for(std::size_t i=0;i<items.size();++i)if(items[i].first==original)selected=i;control->SetSelection(selected);}else static_cast<wxTextCtrl *>(field)->ChangeValue(wxString::FromUTF8(original));}
    wxString GetValue() const override {return choice?static_cast<wxChoice *>(field)->GetStringSelection():static_cast<wxTextCtrl *>(field)->GetValue();}
    void StartingKey(wxKeyEvent & event) override {if(!choice&&event.GetUnicodeKey()>=32){auto control=static_cast<wxTextCtrl *>(field);control->ChangeValue(wxString(wxUniChar(event.GetUnicodeKey())));control->SetInsertionPointEnd();}else event.Skip();}
};
class Table:public Widget {
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
        TableModel * model = nullptr;
        explicit Table(bool autofit = true): autofit(autofit) { is_terminal = true; layout_weight = 1; expandX=expandY=3; }
        template<class Adapter> explicit Table(std::shared_ptr<Adapter> source, bool autofit = true): Table(autofit) {
            using T = typename Adapter::Value;
            std::shared_ptr<BaseTableAdapter<T>> adapter = std::move(source);
            if(!adapter) throw std::invalid_argument("Table adapter must not be null");
            adapterOwner = adapter;
            adapterHeight = adapter->rowCount(); adapterWidth = adapter->columnCount();
            if(adapterHeight < 0 || adapterWidth < 0) throw std::invalid_argument("Table adapter dimensions must be non-negative");
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
        void invalidateNative() override {model=nullptr;Widget::invalidateNative();}
        void updateWidget(Widget * previous) override {
            auto prev=static_cast<Table *>(previous);
            if(prev){ui=prev->ui;model=prev->model;++model->generation;static_cast<wxGrid *>(ui)->DisableCellEditControl();}
            else {auto grid=new wxGrid(parentWindow(),wxID_ANY);ui=grid;model=new TableModel();grid->SetTable(model,true);auto owned=model;
                grid->Bind(wxEVT_GRID_CELL_LEFT_CLICK,[owned](wxGridEvent & e){e.Skip();owned->dispatch(e.GetRow(),e.GetCol(),false);});
                grid->Bind(wxEVT_GRID_CELL_LEFT_DCLICK,[owned](wxGridEvent & e){e.Skip();owned->dispatch(e.GetRow(),e.GetCol(),true);});}
            std::vector<std::vector<detail::TableCell>> rows;
            if(adapterOwner)rows=adapterRows;
            else for(auto row:*children){rows.emplace_back();for(auto node:*row->children){auto entry=dynamic_cast<TableNodeBase *>(node);if(!entry)throw std::logic_error("Table rows require TableNode cells");rows.back().push_back(entry->cell);rows.back().back().clicked=entry->onClicked;}}
            auto colLabels=adapterOwner?headers(adapterColumns,columns):columns;
            auto rowLabels=adapterOwner?headers(adapterHeadings,headings):headings;
            int nr=adapterOwner?adapterHeight:height.valueOr(std::max(int(rows.size()),rowLabels?int(rowLabels->size()):0));
            int nc=colLabels?colLabels->size():0;for(auto & row:rows)nc=std::max(nc,int(row.size()));nc=adapterOwner?adapterWidth:width.valueOr(nc);
            auto grid=static_cast<wxGrid *>(ui);grid->BeginBatch();int oldr=model->height,oldc=model->width;
            model->rows=std::move(rows);model->columns=colLabels;model->headings=rowLabels;model->adapterOwner=adapterOwner;model->height=nr;model->width=nc;
            if(nr>oldr){wxGridTableMessage message(model,wxGRIDTABLE_NOTIFY_ROWS_APPENDED,nr-oldr);grid->ProcessTableMessage(message);}
            if(nr<oldr){wxGridTableMessage message(model,wxGRIDTABLE_NOTIFY_ROWS_DELETED,nr,oldr-nr);grid->ProcessTableMessage(message);}
            if(nc>oldc){wxGridTableMessage message(model,wxGRIDTABLE_NOTIFY_COLS_APPENDED,nc-oldc);grid->ProcessTableMessage(message);}
            if(nc<oldc){wxGridTableMessage message(model,wxGRIDTABLE_NOTIFY_COLS_DELETED,nc,oldc-nc);grid->ProcessTableMessage(message);}
            for(int r=0;r<nr;++r)for(int c=0;c<nc;++c){auto cell=model->cell(r,c);grid->SetReadOnly(r,c,!cell||!cell->write);if(cell&&cell->write)grid->SetCellEditor(r,c,new TableEditor(bool(cell->choices),cell->prefix,cell->suffix,cell->placeholder));}
            grid->SetColLabelSize(colLabels?grid->GetDefaultColLabelSize():0);grid->SetRowLabelSize(rowLabels?grid->GetDefaultRowLabelSize():0);
            if(autofit&&nr&&nc)grid->AutoSizeColumns(false);grid->EndBatch();grid->ForceRefresh();
        }
        void postSync() override {if(ui){auto grid=static_cast<wxGrid *>(ui);grid->SetDefaultCellFont(ui->GetFont());grid->SetDefaultCellTextColour(ui->GetForegroundColour());grid->SetDefaultCellBackgroundColour(ui->GetBackgroundColour());}}
        void destroy(bool direct) override {if(model){model->deactivate();static_cast<wxGrid *>(ui)->DisableCellEditControl();}Widget::destroy(direct);model=nullptr;}
};
}}
namespace CPPUI {
    inline Wx::Table & Table(bool autofit = true) { return *new Wx::Table(autofit); }
    template<class Adapter> inline Wx::Table & Table(std::shared_ptr<Adapter> adapter, bool autofit = true) {
        return *new Wx::Table(std::move(adapter), autofit);
    }
    inline Wx::TableNode<std::string> & TableNode(const std::string & value = {}) { return *new Wx::TableNode<std::string>(value); }
    template<class T> inline Wx::TableNode<T> & TableNode(Binding<T> model) { return *new Wx::TableNode<T>(model); }
    template<class T> inline Wx::TableNode<T> & TableNode(State<T> & model) { return TableNode(Binding<T>(model)); }
    template<class T> inline typename std::enable_if<std::is_arithmetic<T>::value, Wx::TableNode<T> &>::type TableNode(T value) { return *new Wx::TableNode<T>(value); }
}
#endif

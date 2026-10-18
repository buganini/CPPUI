#include "cppui/lvgl/table.hpp"
#include "cppui/lvgl/layout.hpp"
#include "cppui/lvgl/window.hpp"
#include "cppui/lvgl/label.hpp"
#include <stdexcept>

namespace {
    void require(bool ok,const char * message) {if(!ok) throw std::runtime_error(message);}
    using Matrix=std::vector<std::vector<int>>;
    struct Adapter: CPPUI::BaseTableAdapter<int> {
        mutable CPPUI::State<Matrix> values{Matrix{{4,9},{3,2}}};
        bool failRead=false,failWrite=false,failCount=false,headings=false;
        int clicks=0,doubles=0,writes=0;
        std::function<void()> afterWrite,afterClick,afterDouble;
        int data(int r,int c) const override {if(failRead) throw std::runtime_error("read failure");return editData(r,c)*10;}
        int editData(int r,int c) const override {return static_cast<Matrix>(values).at(r).at(c);}
        void setData(int r,int c,const int & value) override {
            if(failWrite) throw std::runtime_error("write failure");
            ++writes;auto next=static_cast<Matrix>(values);next.at(r).at(c)=value;values=next;
            auto invoke=afterWrite;if(invoke) invoke();
        }
        int rowCount() const override {return failCount ? -1 : int(static_cast<Matrix>(values).size());}
        int columnCount() const override {return 2;}
        bool editable(int r,int) const override {return r==0;}
        CPPUI::OptionalText rowHeader(int r) const override {return headings && r==0 ? CPPUI::OptionalText("R0") : CPPUI::OptionalText();}
        void clicked(int,int) override {++clicks;auto invoke=afterClick;if(invoke)invoke();}
        void dblclicked(int,int) override {++doubles;auto invoke=afterDouble;if(invoke)invoke();}
    };
    struct Root: CPPUI::LVGL::View {
        std::shared_ptr<Adapter> source=std::make_shared<Adapter>();
        bool show=true,fallback=false,declarative=false,invalid=false;
        void content() override {
            for(auto window:CPPUI::Window()) for(auto column:CPPUI::VBox()) if(show) {
                if(declarative) {
                    for(auto table:CPPUI::Table().id("first")) for(auto row:CPPUI::TableNode()) CPPUI::TableNode(7);
                } else {
                    auto & first=CPPUI::Table(source);first.id("first");
                    if(fallback) first.rowHeader({"Fallback","Fallback2"});
                    // Adapter counts take precedence over declaration counts.
                    first.rowCount(42).columnCount(42);
                    CPPUI::Table(source).id("second");
                }
                if(invalid) for(auto table:CPPUI::Table()) CPPUI::Label("bad cell");
            }
        }
        CPPUI::LVGL::Table * table(int index=0) {return static_cast<CPPUI::LVGL::Table *>(children->front()->children->front()->children->at(index));}
        lv_obj_t * native(int index=0) {return static_cast<lv_obj_t *>(table(index)->ui);}
        void sync() {redraw();lv_obj_update_layout(lv_screen_active());}
        void edit(const char * value) {
            require(table()->beginEdit(0,0),"adapter editable cell rejected");
            auto field=table()->editor();lv_textarea_set_text(field,value);
            uint32_t enter=LV_KEY_ENTER;lv_obj_send_event(field,LV_EVENT_KEY,&enter);
        }
    };
    void adapters() {
        Root root;root.sync();auto object=root.native();
        require(lv_table_get_row_count(object)==3 && lv_table_get_column_count(object)==2,"adapter count/header override");
        require(root.table()->cellText(0,0)=="40" && !root.table()->beginEdit(1,0),"display or readonly adapter");
        require(root.table()->beginEdit(0,0) && std::string(lv_textarea_get_text(root.table()->editor()))=="4","editData not used");
        root.edit("8");require(root.native()==object && root.table()->cellText(0,0)=="80" && root.table(1)->cellText(0,0)=="80"
            && root.source->writes==1 && !root.table()->editor(),"shared State did not update both tables exactly once");
        require(lv_obj_get_height(object)>200 && lv_obj_get_height(root.native(1))>200,"tables did not share flex height");
        root.fallback=true;root.sync();
        require(std::string(lv_table_get_cell_value(object,1,0))=="Fallback","fallback header missing");
        root.source->headings=true;root.sync();
        require(std::string(lv_table_get_cell_value(object,1,0))=="R0" && std::string(lv_table_get_cell_value(object,2,0))=="Fallback2","partial header merge");
        root.source->values=Matrix{{1,2},{3,4},{5,6}};
        require(lv_table_get_row_count(object)==4,"State shape update lost");
        for(bool count:{true,false}) {
            root.source->failCount=count;root.source->failRead=!count;bool failed=false;
            try {root.sync();} catch(const std::exception &) {failed=true;}
            root.source->failCount=false;root.source->failRead=false;
            require(failed && root.native()==object && root.table()->cellText(0,0)=="10","bad snapshot changed mounted table");
        }
        root.source->failWrite=true;root.edit("9");root.source->failWrite=false;
        require(root.table()->cellText(0,0)=="10" && root.source->writes==1,"throwing setter changed model");
        root.invalid=true;bool failed=false;try {root.sync();} catch(const std::logic_error &) {failed=true;}
        root.invalid=false;require(failed && root.native()==object,"invalid declaration replaced native tree");
        auto weak=std::weak_ptr<Adapter>(root.source);root.source=std::make_shared<Adapter>();root.sync();
        require(weak.expired() && root.native()==object,"old adapter retained after replacement");
        root.source->afterWrite=[&root](){root.show=false;root.redraw();};root.edit("7");
        require(!root.show && root.children->front()->children->front()->children->empty(),"setter could not remove table");
        root.source->afterWrite={};root.show=true;root.sync();
        root.source->afterDouble=[&root](){root.declarative=true;root.redraw();};
        lv_table_set_selected_cell(root.native(),1,1);lv_obj_send_event(root.native(),LV_EVENT_VALUE_CHANGED,nullptr);
        lv_obj_send_event(root.native(),LV_EVENT_DOUBLE_CLICKED,nullptr);
        require(root.table()->cellText(0,0)=="7" && !root.table()->editor(),"double click edited replacement model");
        root.declarative=false;root.sync();
        weak=root.source;root.source.reset();root.show=false;root.sync();require(weak.expired(),"removed adapter leaked");
    }
    struct Plain: CPPUI::BaseTableAdapter<std::string> {
        std::vector<std::string> values{"one","two"};
        std::string data(int r,int) const override {return values[r];}
        void setData(int r,int,const std::string & value) override {values[r]=value;}
        bool editable(int,int) const override {return true;}
        int rowCount() const override {return values.size();}
        int columnCount() const override {return 1;}
        CPPUI::OptionalText rowHeader(int) const override {return {};}
        CPPUI::OptionalText columnHeader(int) const override {return {};}
    };
    struct PlainRoot: CPPUI::LVGL::View {
        std::shared_ptr<Plain> source=std::make_shared<Plain>();
        void content() override {for(auto window:CPPUI::Window()) CPPUI::Table(source);}
        CPPUI::LVGL::Table * table() {return static_cast<CPPUI::LVGL::Table *>(children->front()->children->front());}
    };
    void plain() {
        PlainRoot root;root.redraw();auto object=static_cast<lv_obj_t *>(root.table()->ui);
        root.table()->beginEdit(0,0);auto field=root.table()->editor();lv_textarea_set_text(field,"changed");lv_obj_send_event(field,LV_EVENT_READY,nullptr);
        require(root.source->values[0]=="changed" && root.table()->cellText(0,0)=="changed","plain adapter edit not rendered");
        root.source->values={"three"};root.redraw();require(lv_table_get_row_count(object)==1 && root.table()->cellText(0,0)=="three","plain shape refresh failed");
        root.source->values.clear();root.redraw();require(lv_table_get_row_count(object)==0,"empty adapter retained rows");
    }
}
int main() {
    lv_init();lv_display_create(800,600);auto group=lv_group_create();lv_group_set_default(group);int result=0;
    try {adapters();plain();} catch(const std::exception & error) {std::fprintf(stderr,"%s\n",error.what());result=1;}
    lv_group_set_default(nullptr);lv_group_delete(group);lv_deinit();return result;
}

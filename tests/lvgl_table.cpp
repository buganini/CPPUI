#include "cppui/lvgl/table.hpp"
#include "cppui/lvgl/layout.hpp"
#include "cppui/lvgl/window.hpp"
#include <stdexcept>

namespace {
    void require(bool ok,const char * message) {if(!ok) throw std::runtime_error(message);}
    void settle() {lv_obj_update_layout(lv_screen_active());}
    CPPUI::Node * find(CPPUI::Node & node,const std::string & id) {
        if(node._id==id) return &node;
        for(auto child:*node.children) if(auto result=find(*child,id)) return result;
        return nullptr;
    }
    void key(lv_obj_t * object,uint32_t value) {lv_obj_send_event(object,LV_EVENT_KEY,&value);}
    class Root: public CPPUI::LVGL::View {
    public:
        std::string value="initial",price="",other="other";
        int number=3,writes=0,clicks=0,doubles=0;
        bool flag=false,show=true,headers=true,fit=true,rebind=false,remove=false,redrawOnWrite=false;
        CPPUI::Optional<int> rows,columns;
        std::function<void()> action;
        std::shared_ptr<int> token=std::make_shared<int>(1);
        void content() override {
            for(auto window:CPPUI::Window()) for(auto box:CPPUI::VBox()) if(show) {
                auto & table=CPPUI::Table(fit);
                if(headers) table.columnHeader({"Text","Number","Boolean","Choice"}).rowHeader({"First","Second"});
                if(rows) table.rowCount(*rows);
                if(columns) table.columnCount(*columns);
                table.layout({},180,0);
                for(auto scope:table.id("table")) {
                    for(auto row:CPPUI::TableNode()) {
                        auto saved=token;auto target=rebind ? &other : &value;
                        CPPUI::TableNode(CPPUI::Binding<std::string>([target](){return *target;},[this,target,saved](const std::string & text){
                            ++writes;*target=text;if(remove) show=false;
                            if(redrawOnWrite || remove) redraw();
                        })).display("shown").click([this,saved](){++clicks;auto invoke=action;if(invoke)invoke();});
                        CPPUI::TableNode(CPPUI::Binding<int>([this](){return number;},[this](int value){++writes;number=value;}));
                        CPPUI::TableNode(CPPUI::Binding<bool>([this](){return flag;},[this](bool value){++writes;flag=value;}));
                        CPPUI::TableNode(CPPUI::Binding<int>([this](){return number;},[this](int value){++writes;number=value;}))
                            .choices(std::vector<std::pair<int,std::string>>{{3,"Duplicate"},{8,"Duplicate"}});
                    }
                    for(auto row:CPPUI::TableNode()) {
                        CPPUI::TableNode(CPPUI::Binding<std::string>([this](){return price;},[this](const std::string & text){++writes;price=text;}))
                            .prefix("$").suffix(" USD").placeholder("Enter price");
                        CPPUI::TableNode("readonly").dblclick([this](){++doubles;auto invoke=action;if(invoke)invoke();});
                    }
                }
            }
        }
        CPPUI::LVGL::Table * table() {return dynamic_cast<CPPUI::LVGL::Table *>(find(*this,"table"));}
        lv_obj_t * native() {return static_cast<lv_obj_t *>(table()->ui);}
        void sync() {redraw();settle();}
        lv_obj_t * begin(int row,int column) {require(table()->beginEdit(row,column),"editable cell rejected");settle();return table()->editor();}
        void text(int row,int column,const char * value) {auto field=begin(row,column);lv_textarea_set_text(field,value);key(field,LV_KEY_ENTER);}
    };
    struct Pointer {
        lv_indev_data_t packet{};
        lv_indev_t * device=lv_indev_create();
        Pointer() {
            lv_indev_set_type(device,LV_INDEV_TYPE_POINTER);lv_indev_set_user_data(device,this);
            lv_indev_set_read_cb(device,[](lv_indev_t * input,lv_indev_data_t * data){*data=static_cast<Pointer *>(lv_indev_get_user_data(input))->packet;});
        }
        ~Pointer() {lv_indev_delete(device);}
        void click(lv_obj_t * object,int row,int column) {
            settle();lv_area_t area;lv_obj_get_content_coords(object,&area);
            auto table=reinterpret_cast<lv_table_t *>(object);
            for(int r=0;r<row;++r) area.y1+=table->row_h[r];
            for(int c=0;c<column;++c) area.x1+=table->col_w[c];
            packet.point={area.x1+table->col_w[column]/2-lv_obj_get_scroll_x(object),area.y1+table->row_h[row]/2-lv_obj_get_scroll_y(object)};
            packet.state=LV_INDEV_STATE_PRESSED;lv_indev_read(device);lv_tick_inc(30);
            packet.state=LV_INDEV_STATE_RELEASED;lv_indev_read(device);lv_tick_inc(30);
        }
    };
    void declarative() {
        Root root;root.sync();auto object=root.native();
        require(lv_table_get_row_count(object)==3 && lv_table_get_column_count(object)==5,"header dimensions");
        require(root.table()->cellText(0,0)=="shown" && root.table()->cellText(0,3)=="Duplicate","display or choice label");
        require(root.table()->cellText(1,3).empty() && !root.table()->beginEdit(1,3) && !root.table()->beginEdit(1,1),"ragged/readonly cell editable");
        auto field=root.begin(0,0);require(std::string(lv_textarea_get_text(field))=="initial","editor used display instead of raw data");
        lv_textarea_set_text(field,"cancelled");key(field,LV_KEY_ESC);
        require(root.value=="initial" && root.writes==0 && !root.table()->editor(),"escape committed");
        root.text(0,0,"new");require(root.value=="new" && root.writes==1,"text not committed exactly once");
        root.text(0,1,"4.5");root.text(0,1,"999999999999999999999999");root.text(0,1,"nan");
        require(root.number==3 && root.writes==1,"invalid number reached model");
        root.text(0,1,"8");require(root.number==8 && root.table()->cellText(0,1)=="8","typed numeric commit");
        field=root.begin(0,2);lv_dropdown_set_selected(field,1);lv_obj_send_event(field,LV_EVENT_VALUE_CHANGED,nullptr);
        require(root.flag && root.table()->cellText(0,2)=="True","boolean dropdown failed");
        field=root.begin(0,3);lv_dropdown_set_selected(field,0);lv_obj_send_event(field,LV_EVENT_VALUE_CHANGED,nullptr);
        require(root.number==3,"duplicate choice label lost raw value");
        root.number=99;root.sync();field=root.begin(0,3);lv_obj_send_event(field,LV_EVENT_DEFOCUSED,nullptr);
        require(root.number==99,"unknown choice changed on focus loss");
        field=root.begin(0,3);key(field,LV_KEY_ENTER);key(field,LV_KEY_ENTER);
        require(root.number==3 && !root.table()->editor(),"initial choice could not be accepted");
        field=root.begin(1,0);require(std::string(lv_textarea_get_placeholder_text(field))=="Enter price","placeholder missing");
        require(lv_obj_get_child_count(lv_obj_get_parent(field))==3,"prefix/suffix not separate labels");
        lv_textarea_set_text(field,"12");lv_obj_send_event(field,LV_EVENT_DEFOCUSED,nullptr);
        require(root.price=="12" && root.table()->cellText(1,0)=="$12 USD","decorated edit leaked decorations");
        field=root.begin(0,0);lv_textarea_set_text(field,"stale");root.rebind=true;root.sync();
        require(root.native()==object && !root.table()->editor() && root.value=="new","redraw committed stale draft or replaced native");
        root.text(0,0,"rebound");require(root.other=="rebound" && root.value=="new","stale binding");
        auto weak=std::weak_ptr<int>(root.token);root.token=std::make_shared<int>(2);root.sync();require(weak.expired(),"retired callback retained");
        root.headers=false;root.sync();require(lv_table_get_column_count(object)==4 && lv_table_get_row_count(object)==2,"omitted headers retained");
        root.rows=0;root.columns=0;root.sync();require(lv_table_get_row_count(object)==0 && lv_table_get_column_count(object)==0,"explicit zero count ignored");
        root.rows.reset();root.columns.reset();root.sync();
        root.redrawOnWrite=true;root.text(0,0,"redraw");require(root.other=="redraw" && root.native()==object,"synchronous redraw failed");
        root.remove=true;root.text(0,0,"remove");require(!root.table() && root.other=="remove","setter failed to remove table");
    }
    void input() {
        Root root;root.sync();Pointer pointer;lv_tick_inc(500);
        pointer.click(root.native(),1,1);require(root.clicks==1 && !root.table()->editor(),"single click not dispatched");
        pointer.click(root.native(),1,1);require(root.table()->editor(),"double click did not open editor");
        auto field=root.table()->editor();lv_textarea_set_text(field,"pointer");key(field,LV_KEY_ENTER);require(root.value=="pointer","double click editor commit");
        lv_table_set_selected_cell(root.native(),1,2);key(root.native(),LV_KEY_ENTER);require(root.table()->editor(),"Enter did not open selected cell");
        key(root.table()->editor(),LV_KEY_ESC);
        lv_tick_inc(500);pointer.click(root.native(),2,2);pointer.click(root.native(),2,2);require(root.doubles==1 && !root.table()->editor(),"readonly double click");
        root.action=[&root](){root.show=false;root.redraw();};root.sync();lv_tick_inc(500);pointer.click(root.native(),1,1);
        require(!root.table(),"click could not remove table");
    }
    struct Keyboard {
        lv_indev_data_t packet{};
        lv_indev_t * device=lv_indev_create();
        Keyboard() {
            lv_indev_set_type(device,LV_INDEV_TYPE_KEYPAD);lv_indev_set_group(device,lv_group_get_default());
            lv_indev_set_user_data(device,this);
            lv_indev_set_read_cb(device,[](lv_indev_t * input,lv_indev_data_t * data){*data=static_cast<Keyboard *>(lv_indev_get_user_data(input))->packet;});
        }
        ~Keyboard() {lv_indev_delete(device);}
        void press(uint32_t key) {
            packet.key=key;packet.state=LV_INDEV_STATE_PRESSED;lv_indev_read(device);
            packet.state=LV_INDEV_STATE_RELEASED;lv_indev_read(device);
        }
    };
    void keyboard() {
        Root root;root.sync();Keyboard input;lv_group_focus_obj(root.native());
        lv_table_set_selected_cell(root.native(),1,1);input.press(LV_KEY_ENTER);
        require(root.table()->editor(),"keypad Enter failed to leave editor open");
        input.press('!');input.press(LV_KEY_ENTER);
        require(root.value=="initial!" && root.writes==1 && !root.table()->editor(),"keypad commit duplicated or dropped input");
        lv_table_set_selected_cell(root.native(),1,4);input.press(LV_KEY_ENTER);
        require(root.table()->editor(),"keypad did not open choice editor");
        input.press(LV_KEY_ENTER);input.press(LV_KEY_DOWN);input.press(LV_KEY_ENTER);
        require(root.number==8 && !root.table()->editor(),"keypad choice commit failed");
        root.begin(0,0);lv_textarea_set_text(root.table()->editor(),"focus loss");lv_group_focus_obj(root.native());
        require(root.value=="focus loss" && !root.table()->editor(),"real focus change failed to commit");
    }
    void geometry() {
        Root root;root.sync();auto object=root.native();
        const int small=lv_table_get_column_width(object,1);
        root.price=std::string(50,'M');root.sync();require(lv_table_get_column_width(object,1)>small,"autofit did not grow");
        root.price="";root.sync();require(lv_table_get_column_width(object,1)==small,"autofit did not shrink");
        root.fit=false;root.sync();lv_table_set_column_width(object,1,300);root.sync();
        require(lv_table_get_column_width(object,1)==300,"autofit=false overwrote native width");
        lv_obj_set_width(object,250);root.begin(0,3);settle();
        auto native=reinterpret_cast<lv_table_t *>(object);auto panel=lv_obj_get_parent(root.table()->editor());
        lv_area_t area,panelArea;lv_obj_get_content_coords(object,&area);lv_obj_get_coords(panel,&panelArea);
        int x=area.x1-lv_obj_get_scroll_x(object),y=area.y1-lv_obj_get_scroll_y(object);
        for(int c=0;c<4;++c)x+=native->col_w[c];y+=native->row_h[0];
        require(panelArea.x1==x && panelArea.y1==y && lv_obj_get_width(panel)==native->col_w[4],"editor did not follow scrolled cell geometry");
        root.sync();root.rows=std::numeric_limits<int>::max();bool rejected=false;
        try {root.sync();} catch(const std::invalid_argument &) {rejected=true;}
        require(rejected && root.native()==object,"oversized table changed existing native grid");
        root.rows.reset();
    }
    void nativeDeletion() {
        Root root;root.sync();root.begin(0,0);lv_obj_delete(root.native());
        require(root.table()->ui==nullptr && !root.table()->editor(),"external native deletion retained observer/editor");
        root.show=false;root.sync();
    }
}
int main() {
    lv_init();lv_display_create(800,600);auto group=lv_group_create();lv_group_set_default(group);
    int result=0;
    try {declarative();input();keyboard();geometry();nativeDeletion();}
    catch(const std::exception & error) {std::fprintf(stderr,"%s\n",error.what());result=1;}
    lv_group_set_default(nullptr);lv_group_delete(group);lv_deinit();return result;
}

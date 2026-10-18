#include "cppui/lvgl/grid.hpp"
#include "cppui/lvgl/layout.hpp"
#include "cppui/lvgl/window.hpp"
#include "cppui/lvgl/label.hpp"
#include "cppui/lvgl/textfield.hpp"
#include <stdexcept>

namespace {
    void require(bool ok,const char * message) {if(!ok) throw std::runtime_error(message);}
    void settle() {lv_obj_update_layout(lv_screen_active());}
    CPPUI::Node * find(CPPUI::Node & node,const std::string & id) {
        if(node._id==id) return &node;
        for(auto child:*node.children) if(auto result=find(*child,id)) return result;
        return nullptr;
    }
    CPPUI::Node & cell(CPPUI::Node & node,int row,int col,int rows=1,int cols=1) {
        node.grid_row=row;node.grid_column=col;node.grid_rowspan=rows;node.grid_columnspan=cols;return node;
    }
    void deleted(lv_event_t * event) {++*static_cast<int *>(lv_event_get_user_data(event));}
    struct Pointer {
        lv_indev_data_t packet{};
        lv_indev_t * device=lv_indev_create();
        Pointer() {
            lv_indev_set_type(device,LV_INDEV_TYPE_POINTER);
            lv_indev_set_user_data(device,this);
            lv_indev_set_read_cb(device,[](lv_indev_t * input,lv_indev_data_t * data) {
                *data=static_cast<Pointer *>(lv_indev_get_user_data(input))->packet;
            });
        }
        ~Pointer() {lv_indev_delete(device);}
        void click(lv_obj_t * object) {
            lv_area_t area;lv_obj_get_coords(object,&area);
            packet.point={area.x1+5,area.y1+5};
            packet.state=LV_INDEV_STATE_PRESSED;lv_indev_read(device);
            packet.state=LV_INDEV_STATE_RELEASED;lv_indev_read(device);
        }
    };
    class Root: public CPPUI::LVGL::View {
    public:
        CPPUI::Optional<int> width,height;
        bool row=false,hide=false,margin=false,show=true,grow=false,remove=false;
        std::string clicked;
        std::vector<std::string> keys{"a","b","c","d"};
        int lastRow=2;
        void content() override {
            for(auto window:CPPUI::Window()) for(auto box:row ? static_cast<CPPUI::Node &>(CPPUI::HBox()) : CPPUI::VBox()) if(show) {
                for(auto grid:CPPUI::Grid().id("grid").layout(width,height,grow ? CPPUI::Optional<int>(1) : CPPUI::Optional<int>(),CPPUI::Insets(3,5,7,11))) {
                    for(const auto & key:keys) {
                        auto & item=CPPUI::Label(key);item.id(key);
                        if(key=="a") cell(item,0,0,0,0).layout(30,20);
                        if(key=="b") cell(item,1,0).layout(60,30);
                        if(key=="c") cell(item,0,1,2).layout(70,80).visible(!hide);
                        if(key=="d") cell(item,lastRow,0,1,2).layout(180,40);
                        if(margin && key=="c") item.layout({}, {}, {}, {}, CPPUI::Insets(2,4,6,8));
                        item.click([this,key]() {clicked=key;if(remove) {show=false;redraw();}});
                    }
                    cell(CPPUI::Spacer().id("gap"),10,10);
                    CPPUI::Label("unplaced").id("unplaced");
                }
            }
        }
        lv_obj_t * native(const char * id) {auto node=find(*this,id);require(node && node->outer(),"missing native grid item");return static_cast<lv_obj_t *>(node->outer());}
        void sync() {redraw();settle();}
    };
    void geometry() {
        Root root;root.width=300;root.sync();
        auto grid=root.native("grid"),a=root.native("a"),b=root.native("b"),c=root.native("c"),d=root.native("d");
        require(!find(root,"unplaced") && lv_obj_get_child_count(grid)==5,"unplaced declarations were mounted");
        require(lv_obj_get_width(a)==139 && lv_obj_get_x(c)==145 && lv_obj_get_width(d)==284,"columns or column span did not use allocated width");
        require(lv_obj_get_y(b)==38 && lv_obj_get_y(d)==86 && lv_obj_get_height(grid)==136,"automatic row span did not reserve its full extent");
        require(lv_obj_get_style_grid_cell_row_span(a,LV_PART_MAIN)==1 && lv_obj_get_style_grid_cell_column_span(a,LV_PART_MAIN)==1,"zero span did not default to one cell");
        require(lv_obj_is_ignore_layout(root.native("gap")) && !lv_obj_is_clickable(root.native("gap")),"placed Spacer participated in grid layout or input");
        root.height=200;root.sync();
        require(root.native("a")==a && lv_obj_get_height(grid)==200 && lv_obj_get_height(c)==lv_obj_get_y(b)+lv_obj_get_height(b),"fixed rows did not stretch span or retained items");
        lv_obj_set_size(grid,420,260);settle();
        require(lv_obj_get_width(d)==404 && lv_obj_get_y(d)+lv_obj_get_height(d)==250,"native resize failed to fill allocated grid area");
        root.height.reset();root.sync();
        require(lv_obj_get_height(grid)==136,"fixed-to-auto rows did not shrink");
        root.margin=true;root.sync();
        require(lv_obj_get_height(grid)==144 && lv_obj_get_width(c)==127 && lv_obj_get_x(c)==153 && lv_obj_get_y(c)==2,"span margins were not included in placement and sizing");
        root.margin=false;root.hide=true;root.sync();
        require(root.native("c")==c && lv_obj_is_hidden(c) && lv_obj_get_height(grid)==112,"hidden spanning item retained its old row height");
        root.hide=false;root.keys={"d","c","b","a"};root.sync();
        require(root.native("a")==a && root.native("c")==c && lv_obj_get_height(grid)==136,"declaration reorder changed coordinate identity");
        int removed=0;lv_obj_add_event_cb(d,deleted,LV_EVENT_DELETE,&removed);
        root.lastRow=3;root.sync();
        require(removed==1 && lv_obj_get_y(root.native("d"))==92,"coordinate change did not replace and reposition the cell");
        root.lastRow=2;root.keys={"a","b"};root.sync();
        require(lv_obj_get_height(grid)==66,"removed rows retained their former extent");
        root.grow=true;root.sync();
        require(lv_obj_get_height(grid)>400 && lv_obj_get_y(b)+lv_obj_get_height(b)==lv_obj_get_content_height(grid),"flex allocation did not stretch grid rows");
        root.grow=false;root.sync();require(lv_obj_get_height(grid)==66,"omitted flex allocation did not restore content height");
        root.keys.clear();root.sync();require(lv_obj_get_height(grid)==10,"empty grid retained old height");
        root.keys={"a","b","c","d"};root.row=true;root.width.reset();root.sync();grid=root.native("grid");
        require(lv_obj_get_width(grid)==196 && lv_obj_get_x(root.native("c"))==88,"automatic columns did not include a spanning item's width");
        root.width=400;root.sync();root.width.reset();root.sync();
        require(lv_obj_get_width(grid)==196,"fixed-to-auto columns did not shrink");
        root.keys={"c"};root.sync();require(lv_obj_get_width(grid)==92,"span-only grid lost its intrinsic width or leading empty column");
        root.keys={"d"};root.sync();require(lv_obj_get_width(grid)==196,"span-only grid did not reserve spanning width");
    }
    void input() {
        Pointer pointer;Root root;root.sync();
        for(const auto & key:root.keys) {
            pointer.click(root.native(key.c_str()));require(root.clicked==key,"pointer hit the wrong grid cell");
        }
        root.remove=true;root.sync();pointer.click(root.native("c"));
        require(!root.show && !find(root,"grid"),"cell callback failed to remove its grid");
    }
    class TextRoot: public CPPUI::LVGL::View {
    public:
        std::string text="one\ntwo\nthree";
        CPPUI::Optional<int> height;
        void content() override {
            for(auto window:CPPUI::Window()) for(auto box:CPPUI::VBox()) for(auto grid:CPPUI::Grid().layout({},height).id("grid")) {
                cell(CPPUI::Label(text).id("text"),0,0,2);
                cell(CPPUI::Label("after").id("after"),2,0);
            }
        }
        int size() {redraw();settle();return lv_obj_get_height(static_cast<lv_obj_t *>(find(*this,"grid")->outer()));}
    };
    void naturalText() {
        TextRoot root;const int tall=root.size();
        root.text="one";const int shortHeight=root.size();
        require(shortHeight<tall,"changing spanning label text did not shrink automatic rows");
        root.height=300;root.size();root.height.reset();
        require(root.size()==shortHeight,"stretched label did not restore natural height");
        root.text="one\ntwo\nthree";require(root.size()==tall,"spanning text did not grow again");
    }
    class Cells: public CPPUI::LVGL::View {
        CPPUI::State<int> & count;
    public:
        explicit Cells(CPPUI::State<int> & count):count(count) {}
        void content() override {
            for(int i=0;i<count;++i) cell(CPPUI::Label("row").id("row"+std::to_string(i)).layout(80,20),i+1,0,1,2);
        }
    };
    class NestedRoot: public CPPUI::LVGL::View {
    public:
        CPPUI::State<int> count{1};
        CPPUI::State<std::string> text{std::string("initial")};
        bool hidden=false,show=true;
        void content() override {
            for(auto window:CPPUI::Window()) if(show) for(auto stack:CPPUI::Stack()) {
                for(auto outer:CPPUI::Grid().id("outer")) {
                    for(auto grid:cell(CPPUI::Grid().id("inner").visible(!hidden),0,0)) {
                        cell(CPPUI::TextField(text).id("editor"),0,0);
                        cell((new Cells(count))->id("cells"),1,0);
                    }
                }
            }
        }
        lv_obj_t * native(const char * id) {return static_cast<lv_obj_t *>(find(*this,id)->outer());}
    };
    void lifetime() {
        NestedRoot root;root.redraw();settle();
        auto outer=root.native("outer"),inner=root.native("inner"),editor=root.native("editor");
        require(lv_obj_get_style_pad_left(inner,LV_PART_MAIN)==0 && lv_obj_get_width(inner)==lv_obj_get_content_width(outer),"nested grid padding or stretch failed");
        lv_textarea_set_text(editor,"draft");
        for(int i=1;i<=12;++i) {
            root.count=i;settle();
            require(lv_obj_get_child_count(inner)==static_cast<uint32_t>(i+1),"independent view redraw did not resize native grid tracks");
            root.hidden=!root.hidden;root.redraw();settle();
            require(root.native("inner")==inner && root.native("editor")==editor && std::string(lv_textarea_get_text(editor))=="draft","hidden/redrawn grid lost retained native state");
        }
        root.hidden=false;root.redraw();root.count=0;settle();
        require(lv_obj_get_child_count(inner)==1 && lv_obj_get_height(editor)==lv_obj_get_content_height(inner),"independent removal retained stale row descriptors");
        auto node=find(root,"inner");lv_obj_delete(inner);require(!node->ui,"external grid deletion left a stale observer");
        root.show=false;root.redraw();require(lv_obj_get_child_count(lv_screen_active())==0,"grid removal left native children");
        root.show=true;root.redraw();settle();
    }
}
int main() {
    lv_init();lv_display_create(640,480);int result=0;
    try {geometry();input();naturalText();lifetime();require(lv_obj_get_child_count(lv_screen_active())==0,"grid teardown leaked native content");}
    catch(const std::exception & error) {std::cerr<<error.what()<<'\n';result=1;}
    lv_deinit();return result;
}

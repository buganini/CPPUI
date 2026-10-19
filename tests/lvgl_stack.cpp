#include "cppui/lvgl/stack.hpp"
#include "cppui/lvgl/layout.hpp"
#include "cppui/lvgl/window.hpp"
#include "cppui/lvgl/label.hpp"
#include "cppui/lvgl/textfield.hpp"
#include <stdexcept>

namespace {
    void require(bool ok,const char * message) { if(!ok) throw std::runtime_error(message); }
    void settle() { lv_obj_update_layout(lv_screen_active()); }
    CPPUI::Node * find(CPPUI::Node & node,const std::string & id) {
        if(node._id==id) return &node;
        for(auto child:*node.children) if(auto result=find(*child,id)) return result;
        return nullptr;
    }
    void deleted(lv_event_t * event) { ++*static_cast<int *>(lv_event_get_user_data(event)); }
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
        std::vector<std::string> keys{"bottom","top"};
        CPPUI::Optional<int> width,height;
        bool show=true,hideTop=false,spacer=true,margin=false,row=false,grow=false,remove=false;
        int clicked=0;
        void content() override {
            for(auto window:CPPUI::Window()) for(auto box:row ? static_cast<CPPUI::Node &>(CPPUI::HBox()) : CPPUI::VBox()) if(show) {
                for(auto stack:CPPUI::Stack().id("stack").layout(width,height,grow ? CPPUI::Optional<int>(1) : CPPUI::Optional<int>(),CPPUI::Insets(3,5,7,11))) {
                    for(const auto & key:keys) {
                        auto & label=CPPUI::Label(key);label.id(key).layout(key=="top" ? 70 : 90,key=="top" ? 60 : 40);
                        label.visible(key!="top" || !hideTop);
                        if(margin && key=="top") label.layout({}, {}, {}, {}, CPPUI::Insets(2,4,6,8));
                        label.click([this,key]() {clicked=key=="top" ? 2 : 1;if(remove) {show=false;redraw();}});
                    }
                    if(spacer) CPPUI::Spacer().id("gap");
                }
            }
        }
        lv_obj_t * native(const char * id) { return static_cast<lv_obj_t *>(find(*this,id)->outer()); }
        void sync() {redraw();settle();}
    };
    void geometry() {
        Root root;root.sync();
        auto stack=root.native("stack"),bottom=root.native("bottom"),top=root.native("top");
        require(lv_obj_get_height(stack)==70,"automatic Stack height did not fit largest layer plus padding");
        require(lv_obj_get_x(bottom)==0 && lv_obj_get_y(bottom)==0 && lv_obj_get_x(top)==0 && lv_obj_get_y(top)==0,"layers do not overlap at the padded origin");
        require(lv_obj_get_width(top)==lv_obj_get_content_width(stack),"allocated Stack width did not stretch layers");
        require(!lv_obj_is_clickable(root.native("gap")) && lv_obj_get_width(root.native("gap"))==0 && lv_obj_get_height(root.native("gap"))==0,"Spacer covered Stack content");
        root.height=200;root.width=300;root.sync();
        require(root.native("stack")==stack && root.native("top")==top && lv_obj_get_width(top)==284 && lv_obj_get_height(top)==190 && lv_obj_get_height(bottom)==190,"fixed Stack size did not stretch retained layers");
        root.margin=true;root.sync();
        require(lv_obj_get_x(top)==8 && lv_obj_get_y(top)==2 && lv_obj_get_width(top)==272 && lv_obj_get_height(top)==182,"layer margin was not applied inside Stack");
        root.margin=false;root.sync();
        require(lv_obj_get_x(top)==0 && lv_obj_get_width(top)==284,"omitted layer margin was retained");
        lv_obj_set_size(stack,420,240);settle();
        require(lv_obj_get_width(top)==404 && lv_obj_get_height(top)==230,"native resize did not resize layers");
        root.width.reset();root.height.reset();root.sync();
        require(lv_obj_get_height(stack)==70,"fixed-to-auto Stack did not restore intrinsic height");
        root.grow=true;root.sync();
        require(lv_obj_get_height(stack)>400 && lv_obj_get_height(top)==lv_obj_get_content_height(stack),"flex allocation did not stretch Stack layers");
        root.grow=false;root.sync();
        require(lv_obj_get_height(stack)==70,"omitted flex allocation retained stretched height");
        root.margin=true;root.sync();
        require(lv_obj_get_height(stack)==78,"automatic Stack height ignored layer margins");
        root.margin=false;root.sync();
        root.hideTop=true;root.sync();
        require(lv_obj_get_height(stack)==50 && lv_obj_is_hidden(top),"hidden large layer retained automatic height");
        root.keys={"top","bottom"};root.sync();
        require(root.native("top")==top && lv_obj_get_child(stack,0)==top && lv_obj_is_hidden(top),"moving hidden layer lost native identity or visibility");
        root.hideTop=false;root.sync();
        require(lv_obj_get_height(stack)==70 && !lv_obj_is_hidden(top),"restored layer did not restore intrinsic height");
        root.keys={"bottom"};root.sync();
        require(lv_obj_get_height(stack)==50,"removing large layer retained old size");
        root.keys.clear();root.sync();
        require(lv_obj_get_height(stack)==10,"empty Stack retained stale size");
        root.keys={"bottom","top"};root.sync();
        root.row=true;root.sync();stack=root.native("stack");
        require(lv_obj_get_width(stack)==106,"automatic Stack width did not fit largest layer plus padding");
        root.width=300;root.sync();root.width.reset();root.sync();
        require(lv_obj_get_width(stack)==106,"fixed-to-auto Stack width did not shrink");
        root.keys={"top"};root.sync();
        require(lv_obj_get_width(stack)==86,"automatic Stack width did not shrink after removal");
    }
    void input() {
        Pointer pointer;Root root;root.sync();
        pointer.click(root.native("bottom"));require(root.clicked==2,"last layer did not receive overlapping pointer click");
        root.hideTop=true;root.clicked=0;root.sync();
        pointer.click(root.native("bottom"));require(root.clicked==1,"hidden layer intercepted click");
        root.hideTop=false;root.keys={"top","bottom"};root.clicked=0;root.sync();
        pointer.click(root.native("bottom"));require(root.clicked==1,"keyed reorder did not change pointer hit order");
        root.remove=true;root.sync();pointer.click(root.native("bottom"));
        require(!root.show && lv_obj_get_child_count(lv_screen_active())==1,"click removal retained Stack");
    }
    class Layers: public CPPUI::LVGL::View {
        CPPUI::State<int> & order;
    public:
        explicit Layers(CPPUI::State<int> & order):order(order) {}
        void content() override {
            if(order==0) return;
            if(order==1) CPPUI::Label("A").layout(100,30).id("a");
            CPPUI::Label("B").layout(140,50).id("b");
            if(order==2) CPPUI::Label("A").layout(100,30).id("a");
        }
    };
    class NestedRoot: public CPPUI::LVGL::View {
    public:
        CPPUI::State<int> order{1};
        CPPUI::State<std::string> text{std::string("initial")};
        bool show=true,hidden=false;
        void content() override {
            for(auto window:CPPUI::Window()) if(show) for(auto outer:CPPUI::Stack().id("outer")) {
                for(auto inner:CPPUI::Stack().id("inner").visible(!hidden)) {
                    (new Layers(order))->id("layers");
                    CPPUI::TextField(text).id("editor");
                }
            }
        }
        lv_obj_t * native(const char * id) {return static_cast<lv_obj_t *>(find(*this,id)->outer());}
    };
    void lifetime() {
        NestedRoot root;root.redraw();settle();
        auto outer=root.native("outer"),inner=root.native("inner"),a=root.native("a"),b=root.native("b"),editor=root.native("editor");
        require(lv_obj_get_style_pad_left(inner,LV_PART_MAIN)==0,"nested Stack received duplicate default padding");
        require(lv_obj_get_width(inner)==lv_obj_get_content_width(outer) && lv_obj_get_height(a)==lv_obj_get_content_height(inner),"nested Stack did not stretch to parent");
        lv_textarea_set_text(editor,"draft");
        for(int i=0;i<12;++i) {
            auto first=lv_obj_get_child(inner,0);
            root.order=2;settle();
            if(root.hidden) require(lv_obj_get_child(inner,0)==first,"hidden Stack reconciled frozen layer order");
            else require(lv_obj_get_child(inner,0)==b && lv_obj_get_child(inner,1)==a,"independent virtual reorder lost layer order");
            root.hidden=!root.hidden;root.redraw();settle();
            if(!root.hidden) require(lv_obj_get_child(inner,0)==b && lv_obj_get_child(inner,1)==a,"shown Stack did not apply pending layer order");
            root.order=1;settle();
            require(root.native("inner")==inner && root.native("editor")==editor && std::string(lv_textarea_get_text(editor))=="draft","redraw/hide lost native state");
        }
        int removed=0;lv_obj_add_event_cb(a,deleted,LV_EVENT_DELETE,&removed);lv_obj_add_event_cb(b,deleted,LV_EVENT_DELETE,&removed);
        root.order=0;settle();require(removed==2 && lv_obj_get_child_count(inner)==1,"virtual removal leaked layers");
        root.order=1;settle();
        auto node=find(root,"inner");lv_obj_delete(inner);require(!node->ui,"external deletion left stale Stack observer");
        root.show=false;root.redraw();require(lv_obj_get_child_count(lv_screen_active())==0,"removed Stack retained native children");
        root.show=true;root.redraw();settle();
    }
}
int main() {
    lv_init();lv_display_create(640,480);
    int result=0;
    try {geometry();input();lifetime();require(lv_obj_get_child_count(lv_screen_active())==0,"Stack teardown leaked native objects");}
    catch(const std::exception & error) {std::cerr<<error.what()<<'\n';result=1;}
    lv_deinit();return result;
}

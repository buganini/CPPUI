#include "cppui/lvgl/splitter.hpp"
#include "cppui/lvgl/layout.hpp"
#include "cppui/lvgl/window.hpp"
#include "cppui/lvgl/label.hpp"
#include "cppui/lvgl/textfield.hpp"
#include <stdexcept>

namespace {
    void require(bool ok,const char * message) {if(!ok) throw std::runtime_error(message);}
    void settle() {lv_obj_update_layout(lv_screen_active());}
    CPPUI::Node * find(CPPUI::Node & root,const std::string & id) {
        if(root._id==id) return &root;
        for(auto child:*root.children) if(auto result=find(*child,id)) return result;
        return nullptr;
    }
    lv_obj_t * native(CPPUI::Node & root,const char * id) {
        auto node=find(root,id);require(node && node->outer(),"missing native node");
        return static_cast<lv_obj_t *>(node->outer());
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
        void press(lv_obj_t * handle) {
            lv_area_t area;lv_obj_get_coords(handle,&area);
            packet.point={(area.x1+area.x2)/2,(area.y1+area.y2)/2};
            packet.state=LV_INDEV_STATE_PRESSED;lv_indev_read(device);
        }
        void move(int x,int y) {packet.point.x+=x;packet.point.y+=y;lv_indev_read(device);settle();}
        void release() {packet.state=LV_INDEV_STATE_RELEASED;lv_indev_read(device);settle();}
        void drag(lv_obj_t * handle,int x,int y) {press(handle);move(x,y);release();}
    };
    class Root: public CPPUI::LVGL::View {
    public:
        std::vector<std::string> keys{"a","b","c"};
        CPPUI::State<std::string> text{std::string("initial")};
        bool vertical=false,show=true,visible=true,hidePane=false,nested=false,replace=false,extra=false;
        int padding=3,margin=0;
        void content() override {
            for(auto window:CPPUI::Window()) if(show)
                for(auto split:CPPUI::Splitter(vertical).id("split").visible(visible).layout({}, {}, {}, CPPUI::Insets(padding))) {
                    for(const auto & key:keys) {
                        if(key=="a") {
                            if(replace) CPPUI::Label("replacement").id(key);
                            else for(auto box:CPPUI::VBox().id(key).layout({}, {}, {}, CPPUI::Insets(0),CPPUI::Insets(margin))) {
                                CPPUI::TextField(text).id("editor");
                                if(extra) CPPUI::Label("extra").id("extra");
                            }
                        } else if(key=="b" && nested) {
                            for(auto inner:CPPUI::Splitter(true).id(key)) {CPPUI::Label("top").id("top");CPPUI::Label("bottom").id("bottom");}
                        } else CPPUI::Label(key).id(key).visible(key!="b" || !hidePane).click([this]() {show=false;redraw();});
                    }
                }
        }
        void sync() {redraw();settle();}
    };
    void geometry() {
        Root root;root.sync();Pointer pointer;
        auto host=native(root,"split"),a=native(root,"a"),b=native(root,"b"),c=native(root,"c"),editor=native(root,"editor");
        require(lv_obj_get_child_count(host)==5,"three panes need two handles");
        auto first=lv_obj_get_child(host,1),second=lv_obj_get_child(host,3);
        require(lv_obj_get_width(first)==12 && lv_obj_get_height(first)==lv_obj_get_content_height(host),"horizontal handle geometry is wrong");
        require(std::abs(lv_obj_get_width(a)-lv_obj_get_width(c))<=2 && lv_obj_get_x(b)==lv_obj_get_width(a)+12,
            "initial panes were not equally spaced");
        const int aw=lv_obj_get_width(a),bw=lv_obj_get_width(b),cw=lv_obj_get_width(c);
        pointer.drag(first,35,0);
        require(lv_obj_get_width(a)==aw+35 && lv_obj_get_width(b)==bw-35 && lv_obj_get_width(c)==cw,
            "drag did not resize only the adjacent panes");
        lv_textarea_set_text(editor,"draft");lv_textarea_set_cursor_pos(editor,2);
        root.extra=true;
        for(int i=0;i<20;++i) root.sync();
        require(native(root,"split")==host && native(root,"editor")==editor && lv_obj_get_child(host,1)==first
            && lv_obj_get_child(host,3)==second && lv_obj_get_width(a)==aw+35,
            "ordinary redraw reset handles, allocations or native controls");
        require(std::string(lv_textarea_get_text(editor))=="draft" && lv_textarea_get_cursor_pos(editor)==2,
            "redraw lost the editor draft/cursor");
        root.margin=5;root.sync();
        require(lv_obj_get_x(a)==5 && lv_obj_get_width(a)==aw+25 && lv_obj_get_height(a)==lv_obj_get_content_height(host)-10,
            "pane margins did not inset its allocation");
        root.margin=0;root.sync();require(lv_obj_get_x(a)==0 && lv_obj_get_width(a)==aw+35,"omitted margins did not reset");
        root.hidePane=true;root.sync();require(lv_obj_is_hidden(b) && lv_obj_get_x(c)==aw+bw+24,"hidden pane changed split positions");
        root.hidePane=false;root.visible=false;root.sync();root.visible=true;root.sync();
        require(lv_obj_is_visible(editor) && lv_obj_is_visible(b) && lv_obj_get_width(a)==aw+35,"hide/show reset allocation or visibility");
        const int before=lv_obj_get_width(a),usable=lv_obj_get_content_width(host)-24;
        lv_display_set_resolution(lv_display_get_default(),800,600);settle();
        require(std::abs(lv_obj_get_width(a)-int(int64_t(before)*(lv_obj_get_content_width(host)-24)/usable))<=1,
            "resize did not preserve pane proportions");
        pointer.drag(first,-2000,0);require(lv_obj_get_width(a)==1,"drag below minimum collapsed a pane");
        pointer.drag(first,2000,0);require(lv_obj_get_width(b)==1,"drag past maximum collapsed adjacent pane");
        root.keys={"c","a","d"};root.sync();
        require(native(root,"c")==c && native(root,"editor")==editor && lv_obj_get_child(host,0)==c
            && lv_obj_get_child(host,2)==a,"keyed pane move replaced controls or lost order");
        require(std::abs(lv_obj_get_width(c)-lv_obj_get_width(a))<=1,"changed composition did not reset allocations");
        root.vertical=true;root.sync();first=lv_obj_get_child(host,1);
        const int ch=lv_obj_get_height(c),ah=lv_obj_get_height(a);
        pointer.drag(first,0,25);
        require(lv_obj_get_height(c)==ch+25 && lv_obj_get_height(a)==ah-25 && lv_obj_get_height(first)==12,
            "vertical drag did not resize stacked panes");
        root.keys={"a"};root.sync();require(lv_obj_get_child_count(host)==1 && lv_obj_get_height(a)==lv_obj_get_content_height(host),
            "single pane did not fill Splitter");
        root.keys.clear();root.sync();require(lv_obj_get_child_count(host)==0,"empty Splitter retained handles or panes");
        root.keys={"a","b","c","d"};root.vertical=false;root.nested=true;root.sync();
        auto inner=native(root,"b"),top=native(root,"top");const int oldHeight=lv_obj_get_height(top);
        require(lv_obj_get_child_count(host)==7 && lv_obj_get_child_count(inner)==3,"nested/many-pane Splitter did not mount");
        pointer.drag(lv_obj_get_child(inner,1),0,30);
        require(lv_obj_get_height(top)==oldHeight+30,"nested handle did not receive pointer drag");
        root.sync();require(lv_obj_get_height(top)==oldHeight+30,"parent redraw reset nested split");
        int removed=0;lv_obj_add_event_cb(native(root,"editor"),deleted,LV_EVENT_DELETE,&removed);
        root.replace=true;root.sync();require(removed==1 && lv_obj_get_child(host,0)==native(root,"a"),"pane type replacement leaked editor");
        lv_display_set_resolution(lv_display_get_default(),9,9);settle();
        for(uint32_t i=0;i<lv_obj_get_child_count(host);++i) {
            auto child=lv_obj_get_child(host,i);
            require(lv_obj_get_width(child)>=0 && lv_obj_get_x(child)+lv_obj_get_width(child)<=lv_obj_get_content_width(host),
                "tiny viewport overflowed pane/handle allocation");
        }
        lv_display_set_resolution(lv_display_get_default(),640,480);settle();
        require(std::abs(lv_obj_get_width(native(root,"a"))-lv_obj_get_width(native(root,"d")))<=3,
            "restoring a tiny viewport lost the pane proportions");
        auto declaration=find(root,"split");lv_obj_delete(host);
        require(!declaration->ui,"external deletion left stale Splitter native observer");
        root.show=false;root.sync();root.show=true;root.sync();
        lv_obj_send_event(native(root,"c"),LV_EVENT_CLICKED,nullptr);
        require(!find(root,"split") && lv_obj_get_child_count(lv_screen_active())==0,"click removal retained Splitter objects");
    }
    class Panes: public CPPUI::LVGL::View {
        CPPUI::State<int> & count;
    public:
        explicit Panes(CPPUI::State<int> & count):count(count) {}
        void content() override {for(int i=0;i<count;++i) CPPUI::Label("pane").id("pane"+std::to_string(i));}
    };
    class VirtualRoot: public CPPUI::LVGL::View {
    public:
        CPPUI::State<int> count{2};
        void content() override {
            for(auto window:CPPUI::Window()) for(auto split:CPPUI::Splitter().id("split"))
                (new Panes(count))->id("panes");
        }
    };
    void independent() {
        VirtualRoot root;root.redraw();settle();auto host=native(root,"split"),pane=native(root,"pane0");
        for(int count:{4,1,3,0,2}) {
            root.count=count;settle();
            require(lv_obj_get_child_count(host)==unsigned(std::max(0,2*count-1)),"independent view did not reconcile handles/panes");
            if(count==4) require(native(root,"pane0")==pane,"independent update replaced retained pane");
        }
        // Delete one native child externally, then remove its declaration.
        lv_obj_delete(native(root,"pane1"));root.count=1;settle();
        require(lv_obj_get_child_count(host)==1,"externally deleted child left a stale pane/handle");
    }
    void activeDrag() {
        Root root;root.sync();Pointer pointer;
        auto handle=lv_obj_get_child(native(root,"split"),1);
        pointer.press(handle);root.extra=true;root.sync();pointer.move(20,0);pointer.release();
        require(lv_obj_get_child(native(root,"split"),1)==handle,"redraw during drag replaced handle");
        // Removing the host from a native handle callback must leave no callbacks
        // referring to the retired declaration or freed native state.
        lv_obj_add_event_cb(handle,[](lv_event_t * event) {
            auto root=static_cast<Root *>(lv_event_get_user_data(event));root->show=false;root->sync();
        },LV_EVENT_PRESSING,&root);
        pointer.press(handle);pointer.release();
        require(!find(root,"split"),"active handle callback did not remove Splitter");
    }
}
int main() {
    lv_init();lv_display_create(640,480);
    int result=0;
    try {geometry();independent();activeDrag();require(lv_obj_get_child_count(lv_screen_active())==0,"Splitter teardown leaked native objects");}
    catch(const std::exception & error) {std::cerr<<error.what()<<'\n';result=1;}
    lv_deinit();return result;
}

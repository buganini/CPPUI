#include "cppui/lvgl/backend.hpp"
#include <stdexcept>
static void require(bool ok,const char * message) {if(!ok) throw std::runtime_error(message);}
static CPPUI::Node * find(CPPUI::Node & n,const char * id) {
    if(n._id==id) return &n;
    for(auto child:*n.children) if(auto found=find(*child,id)) return found;
    return nullptr;
}
static void pump() {for(int i=0;i<8;++i) {lv_tick_inc(LV_DEF_REFR_PERIOD+1);lv_timer_handler();lv_obj_update_layout(lv_screen_active());}}
struct Root:CPPUI::LVGL::View {
    bool horizontal=false,inverse=false,canvas=true,spacer=false;
    int mode=0,container=0;
    CPPUI::Optional<int> weight;
    void content() override {
        for(auto w:CPPUI::Window()) {
            auto & box=horizontal ? static_cast<CPPUI::Node &>(CPPUI::HBox(inverse)) : CPPUI::VBox(inverse);
            for(auto scope:box.layout(CPPUI::layout::padding(0)).id("box")) {
                if(mode==0) {CPPUI::Label("First").id("first");CPPUI::Divider();CPPUI::Label("Last").id("last");}
                if(mode==1) {
                    auto & along=horizontal ? static_cast<CPPUI::Node &>(CPPUI::HBox()) : CPPUI::VBox();
                    for(auto inner:along.id("along").layout(CPPUI::layout::padding(0))) CPPUI::Label("Along");
                    auto & across=horizontal ? static_cast<CPPUI::Node &>(CPPUI::VBox()) : CPPUI::HBox();
                    for(auto inner:across.id("across").layout(CPPUI::layout::padding(0))) CPPUI::Label("Across");
                    CPPUI::Canvas([](CPPUI::CanvasPainter &){}).layout(40,40,weight).visible(canvas).id("canvas");
                    CPPUI::Spacer().visible(spacer).id("spacer");
                }
                if(mode==2) {CPPUI::Label("Weighted").layout({}, {},weight).id("label");CPPUI::Spacer().id("spacer");}
                if(mode==3) {
                    CPPUI::Label("Heading");
                    for(auto scroll:CPPUI::Scroll().id("scroll"))
                        for(auto rows:CPPUI::VBox().layout(CPPUI::layout::padding(0)).id("rows"))
                            for(int i=0;i<80;++i) CPPUI::Label(std::to_string(i)).id("row"+std::to_string(i));
                }
                if(mode==5) {
                    CPPUI::Node * candidate=nullptr;
                    if(container==0) candidate=&CPPUI::Stack();
                    if(container==1) candidate=&CPPUI::Tabs();
                    if(container==2) candidate=&CPPUI::Views();
                    if(container==3) candidate=&CPPUI::Splitter();
                    for(auto nested:candidate->id("candidate")) {
                        if(container==1) for(auto tab:CPPUI::Tab("Page")) CPPUI::Label("Page");
                        else CPPUI::Label("Page").id("page");
                    }
                    CPPUI::Canvas([](CPPUI::CanvasPainter &){}).layout(40,40).visible(canvas).id("canvas");
                }
                if(mode==4) {
                    for(auto fixed:CPPUI::VBox().layout(CPPUI::layout::width(120)).id("fixed"))
                        CPPUI::Canvas([](CPPUI::CanvasPainter &){}).layout(40,40);
                    for(auto plain:CPPUI::VBox().id("plain")) CPPUI::Label("Plain");
                }
            }
        }
    }
    lv_obj_t * object(const char * id) {return static_cast<lv_obj_t *>(find(*this,id)->outer());}
    int extent(const char * id) {return horizontal ? lv_obj_get_width(object(id)) : lv_obj_get_height(object(id));}
    void mount() {redraw();pump();}
};
int main() {
    lv_init();auto display=lv_display_create(800,600);int result=0;
    try {
        Root root;
        for(bool h:{false,true}) for(bool inverse:{false,true}) {
            root.horizontal=h;root.inverse=inverse;root.mount();
            for(auto id:{"first","last"}) require(lv_obj_get_width(root.object(id))<100 && lv_obj_get_height(root.object(id))<50,"static label stretched");
        }
        root.mode=1;root.inverse=false;
        for(bool h:{false,true}) for(int state=0;state<5;++state) {
            root.horizontal=h;root.canvas=state!=1;root.spacer=state==2;
            root.weight=state==3 ? CPPUI::Optional<int>(0) : CPPUI::Optional<int>();root.mount();
            const std::string winner=state==1 || state==3 ? "along" : state==2 ? "spacer" : "canvas";
            for(int size:{800,650,900}) {
                lv_display_set_resolution(display,size,size-200);pump();
                for(auto id:{"along","across","canvas","spacer"}) if(find(root,id)->is_visible) {
                    if(!(id==winner ? root.extent(id)>250 : root.extent(id)<150)) {
                        std::cerr<<"h="<<h<<" state="<<state<<" winner="<<winner<<" id="<<id<<" size="<<root.extent(id)<<'\n';
                        throw std::runtime_error("wrong expansion winner after resize");
                    }
                }
            }
        }
        root.mode=2;root.horizontal=false;
        for(int weight:{1,2,300,1000000000,0,-1,1}) {
            root.weight=weight<0 ? CPPUI::Optional<int>() : CPPUI::Optional<int>(weight);root.mount();
            for(int size:{700,950,600}) {
                lv_display_set_resolution(display,size,size-100);pump();
                if(weight>0) require(std::abs(root.extent("label")-(root.extent("label")+root.extent("spacer"))*double(weight)/(double(weight)+1))<=1,"weighted allocation ratio after resize");
                else require(root.extent("label")<50,"weight removal retained expanded height");
            }
        }
        root.mode=3;root.mount();
        require(lv_obj_get_scroll_bottom(root.object("scroll"))>500,"scroll lost content height");
        for(auto child:*find(root,"rows")->children)
            require(lv_obj_get_height(static_cast<lv_obj_t *>(child->outer()))<50,"scroll row expanded");
        root.mode=5;
        for(bool h:{false,true}) for(int kind=0;kind<4;++kind) for(bool canvas:{true,false}) {
            root.horizontal=h;root.container=kind;root.canvas=canvas;root.mount();
            if(!(canvas ? root.extent("canvas")>250 && root.extent("candidate")<180 : root.extent("candidate")>250)) {
                std::cerr<<"container="<<kind<<" h="<<h<<" canvas="<<canvas<<" candidate="<<root.extent("candidate")<<'\n';
                throw std::runtime_error("container priority allocation");
            }
        }
        root.mode=4;root.horizontal=true;root.mount();
        require(find(root,"fixed")->expansionPriority(true)==1,"fixed dimension leaked child demand");
    } catch(const std::exception & e) {std::cerr<<e.what()<<'\n';result=1;}
    lv_deinit();return result;
}

#include "cppui/backend.hpp"
#include <stdexcept>
static void require(bool ok,const char * message) {if(!ok) throw std::runtime_error(message);}
static CPPUI::Node * find(CPPUI::Node & n,const char * id) {if(n._id==id) return &n;for(auto c:*n.children) if(auto r=find(*c,id)) return r;return nullptr;}
static void pump() {for(int i=0;i<8;++i) {
#if defined(CPPUI_BACKEND_QT)
    QCoreApplication::processEvents();QCoreApplication::sendPostedEvents(nullptr,QEvent::LayoutRequest);
#elif defined(CPPUI_BACKEND_WX)
    wxTheApp->Yield(true);wxTheApp->ProcessPendingEvents();wxTheApp->ProcessIdle();
#else
    lv_tick_inc(LV_DEF_REFR_PERIOD+1);lv_timer_handler();lv_obj_update_layout(lv_screen_active());
#endif
}}
struct Root:CPPUI::Application {
    using Application::Application;
    CPPUI::State<std::string> text{std::string("value")};
    CPPUI::Optional<int> weight;
    bool automatic=false,editors=true,gap=false;
    void content() override {
        for(auto w:CPPUI::Window().size(700,400)) {
            auto & declaration=CPPUI::Grid();if(automatic) declaration.columns(3,CPPUI::grid::expandChildren(false));
            for(auto grid:declaration.layout(CPPUI::layout::padding(0)).id("grid")) {
                CPPUI::Label("Field").id("label").grid(CPPUI::grid::row(0),CPPUI::grid::column(0)).layout({}, {},weight);
                CPPUI::Label("Value").id("value").grid(CPPUI::grid::row(0),CPPUI::grid::column(1));
                CPPUI::TextField(text).id("input").grid(CPPUI::grid::row(0),CPPUI::grid::column(2)).visible(editors);
                CPPUI::Label("Body").id("body-label").grid(CPPUI::grid::row(1),CPPUI::grid::column(0),CPPUI::grid::columnspan(2));
                CPPUI::TextEdit(text).id("body").grid(CPPUI::grid::row(1),CPPUI::grid::column(2)).visible(editors);
                CPPUI::Label("Footer").id("footer").grid(CPPUI::grid::row(2),CPPUI::grid::column(0),CPPUI::grid::columnspan(3));
                if(gap) CPPUI::Spacer().id("gap").grid(CPPUI::grid::row(3),CPPUI::grid::column(2)).layout(CPPUI::layout::weight(2));
            }
        }
    }
    void mount() {
#if defined(CPPUI_BACKEND_QT) || defined(CPPUI_BACKEND_WX)
        sync();
#else
        redraw();
#endif
        pump();
    }
    int extent(const char * id,bool x) {
        auto p=find(*this,id)->outer();
#if defined(CPPUI_BACKEND_QT)
        auto w=static_cast<QWidget *>(p);return x ? w->width() : w->height();
#elif defined(CPPUI_BACKEND_WX)
        auto size=static_cast<wxWindow *>(p)->GetSize();return x ? size.x : size.y;
#else
        return x ? lv_obj_get_width(static_cast<lv_obj_t *>(p)) : lv_obj_get_height(static_cast<lv_obj_t *>(p));
#endif
    }
};
int main(int argc,char ** argv) {
#if defined(CPPUI_BACKEND_LVGL)
    lv_init();lv_display_create(700,400);
#endif
    int result=0;
    try {
        Root root(argc,argv);root.mount();auto editor=find(root,"input")->outer();
        for(bool automatic:{false,true,false}) {
            root.automatic=automatic;root.mount();
            require(root.extent("label",true)<120 && root.extent("value",true)<120,"label columns did not stay compact");
            require(root.extent("input",true)>300 && root.extent("input",false)<100,"TextField track sizing");
            require(root.extent("body",false)>150 && root.extent("footer",false)<80,"TextEdit row did not receive space");
        }
        root.weight=2;root.mount();require(root.extent("label",true)>150 && root.extent("label",false)>60,"grid weight did not apply on both axes");
        root.weight=0;root.mount();require(root.extent("label",true)<120 && find(root,"input")->outer()==editor,"zero weight or retention");
        root.editors=false;root.mount();require(root.extent("footer",true)<180 && root.extent("footer",false)<80,"hidden editors retained track demand");
        root.editors=true;root.mount();require(find(root,"input")->outer()==editor && root.extent("input",true)>300,"restored editor did not expand");
        for(int i=0;i<3;++i) {
            root.gap=true;root.mount();root.mount();
            require(find(root,"gap") && root.extent("input",true)>300,"placed Spacer lost its track or expansion");
            root.gap=false;root.mount();
            require(!find(root,"gap") && find(root,"input")->outer()==editor,"Spacer removal damaged retained cells");
        }
    } catch(const std::exception & e) {std::cerr<<e.what()<<'\n';result=1;}
#if defined(CPPUI_BACKEND_LVGL)
    lv_deinit();
#endif
    return result;
}

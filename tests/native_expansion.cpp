#if defined(CPPUI_TEST_QT)
#include "cppui/qt/backend.hpp"
#elif defined(CPPUI_TEST_WX)
#include "cppui/wx/backend.hpp"
#else
#include "cppui/lvgl/backend.hpp"
#endif
#include <stdexcept>
static void require(bool ok,const char * message) {if(!ok) throw std::runtime_error(message);}
static CPPUI::Node * find(CPPUI::Node & n,const char * id) {
    if(n._id==id) return &n;
    for(auto child:*n.children) if(auto found=find(*child,id)) return found;
    return nullptr;
}
static void pump() {
    for(int i=0;i<8;++i) {
#if defined(CPPUI_TEST_QT)
        QCoreApplication::processEvents();QCoreApplication::sendPostedEvents(nullptr,QEvent::LayoutRequest);
#elif defined(CPPUI_TEST_WX)
        wxTheApp->Yield(true);wxTheApp->ProcessPendingEvents();wxTheApp->ProcessIdle();
#else
        lv_tick_inc(LV_DEF_REFR_PERIOD+1);lv_timer_handler();lv_obj_update_layout(lv_screen_active());
#endif
    }
}
struct Fragment:CPPUI::Backend::View {
    bool shown=false;
    void content() override {
        if(shown) CPPUI::Canvas([](CPPUI::CanvasPainter &){}).layout(CPPUI::layout::width(40),CPPUI::layout::height(40)).id("canvas");
        else CPPUI::Label("Static");
    }
};
struct Root:CPPUI::Application {
    using Application::Application;
    int padding=-1;
    void content() override {
        for(auto w:CPPUI::Window().size(640,480).id("window"))
            for(auto row:CPPUI::HBox().layout(CPPUI::layout::weight(1)).id("row")) {
                for(auto box:CPPUI::VBox().id("demand")) CPPUI::SubView<Fragment>().id("fragment");
                for(auto box:CPPUI::VBox().id("static")) CPPUI::Label("Sibling");
            }
#if defined(CPPUI_TEST_QT)
        for(auto w:CPPUI::Window()) for(auto tabs:CPPUI::Tabs()) {
            auto & page=CPPUI::Tab("Page");if(padding>=0) page.layout(CPPUI::layout::padding(padding));
            for(auto scope:page.id("page")) for(auto box:CPPUI::VBox().id("content")) CPPUI::Label("Content");
        }
#endif
    }
};
static int width(CPPUI::Node * n) {
#if defined(CPPUI_TEST_QT)
    return static_cast<QWidget *>(n->outer())->width();
#elif defined(CPPUI_TEST_WX)
    return static_cast<CPPUI::Wx::Node *>(n)->sizer()->GetSize().x;
#else
    return lv_obj_get_width(static_cast<lv_obj_t *>(n->outer()));
#endif
}
static void sync(CPPUI::View & view) {
#if defined(CPPUI_TEST_QT) || defined(CPPUI_TEST_WX)
    static_cast<CPPUI::Backend::View &>(view).sync();
#else
    view.redraw();
#endif
}
int main(int argc,char ** argv) {
#if !defined(CPPUI_TEST_QT) && !defined(CPPUI_TEST_WX)
    lv_init();lv_display_create(640,480);
#endif

    int result=0;
    try {
        Root root(argc,argv);sync(root);pump();
        for(bool shown:{true,false,true}) {
            auto fragment=static_cast<Fragment *>(find(root,"fragment"));fragment->shown=shown;sync(*fragment);pump();
            auto demand=find(root,"demand");auto sibling=find(root,"static");
            require(demand->expansionPriority(true)==(shown ? 2 : 1),"descendant priority did not update");
            require(sibling->expansionPriority(true)==1,"static sibling promoted");
            if(shown) require(width(demand)>width(sibling)*2,"ancestor did not reflow after independent view update");
            else require(std::abs(width(demand)-width(sibling))<30,"withdrawn demand retained stretch");
        }
#if defined(CPPUI_TEST_QT)
        for(int padding:{-1,0,7,-1}) {
            root.padding=padding;sync(root);pump();
            auto page=static_cast<QWidget *>(find(root,"page")->outer());
            int expected=padding<0 ? 11 : padding;
            require(page->contentsMargins()==QMargins(expected,expected,expected,expected),"Tab padding default/override");
            require(static_cast<QWidget *>(find(root,"content")->outer())->contentsMargins()==QMargins(),"nested Tab padding duplicated");
        }
#endif
    } catch(const std::exception & e) {std::cerr<<e.what()<<'\n';result=1;}
#if !defined(CPPUI_TEST_QT) && !defined(CPPUI_TEST_WX)
    lv_deinit();
#endif
    return result;
}

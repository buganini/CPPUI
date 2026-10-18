#if defined(CPPUI_TEST_QT)
#include "cppui/qt/application.hpp"
#include "cppui/qt/window.hpp"
#include "cppui/qt/tab.hpp"
#include "cppui/qt/views.hpp"
#include "cppui/qt/label.hpp"
#elif defined(CPPUI_TEST_WX)
#include "cppui/wx/application.hpp"
#include "cppui/wx/window.hpp"
#include "cppui/wx/tab.hpp"
#include "cppui/wx/views.hpp"
#include "cppui/wx/label.hpp"
#else
#include "cppui/lvgl/application.hpp"
#include "cppui/lvgl/window.hpp"
#include "cppui/lvgl/tab.hpp"
#include "cppui/lvgl/views.hpp"
#include "cppui/lvgl/label.hpp"
#endif
#include <thread>
#include <chrono>
#include <limits>
static void require(bool value,const char * message) { if(!value) throw std::runtime_error(message); }
class Root:public CPPUI::Application {
public:
    using Application::Application;
    bool show=true;
    CPPUI::State<CPPUI::OptionalText> selection{CPPUI::OptionalText()};
    void content() override {
        for(auto window:CPPUI::Window().ref("window")) for(auto column:CPPUI::VBox()) if(show) {
            for(auto tabs:CPPUI::Tabs().ref("tabs")) {
                for(auto page:CPPUI::Tab("First").id("first")) CPPUI::Label("One");
                for(auto page:CPPUI::Tab("Second").id("second")) CPPUI::Label("Two");
            }
            for(auto views:CPPUI::Views(selection).ref("views")) {
                CPPUI::Label("One").id("first"); CPPUI::Label("Two").id("second");
            }
        }
    }
    void mount() {
#if defined(CPPUI_TEST_QT) || defined(CPPUI_TEST_WX)
        sync();
#else
        redraw();
#endif
    }
};
static void pump() {
    for(int i=0;i<5;++i) {
#if defined(CPPUI_TEST_QT)
        QCoreApplication::processEvents();
#elif defined(CPPUI_TEST_WX)
        wxTheApp->Yield(true); wxTheApp->ProcessPendingEvents(); wxTheApp->ProcessIdle();
#else
        lv_tick_inc(LV_DEF_REFR_PERIOD+1); lv_timer_handler();
#endif
    }
}
static int selected(const CPPUI::NodeBinder & tabs) {
#if defined(CPPUI_TEST_QT)
    return static_cast<QTabWidget *>(tabs.node()->outer())->currentIndex();
#elif defined(CPPUI_TEST_WX)
    return static_cast<wxNotebook *>(tabs.node()->outer())->GetSelection();
#else
    return lv_tabview_get_tab_active(static_cast<lv_obj_t *>(tabs.node()->outer()));
#endif
}
int main(int argc,char ** argv) {
#if !defined(CPPUI_TEST_QT) && !defined(CPPUI_TEST_WX)
    lv_init(); lv_display_create(640,480);
#endif
    int result=0;
    try {
        Root root(argc,argv); root.mount(); pump();
        auto post=CPPUI::ref("tabs").node()->getPoster();
        int callbacks=0;
        const auto uiThread=std::this_thread::get_id();
        auto callback=[&]() { require(std::this_thread::get_id()==uiThread,"callback ran outside UI thread"); ++callbacks; };
        for(double invalid:{-1.,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}) {
            bool rejected=false;try {post(callback,invalid);}catch(const std::invalid_argument &){rejected=true;}
            require(rejected,"invalid delay accepted");
        }
        bool rejected=false;try {post({});}catch(const std::invalid_argument &){rejected=true;}
        require(rejected,"empty callback accepted");
        auto node=CPPUI::ref("tabs").node();bool wrongThread=false;
        std::thread invalidCaller([&]() {try {node->getPoster();}catch(const std::logic_error &){wrongThread=true;}});invalidCaller.join();
        require(wrongThread,"getPoster accepted worker-thread access");
        std::thread immediate([&]() {post(callback);});immediate.join();
        require(callbacks==0,"post ran synchronously");root.mount();pump();require(callbacks==1,"post lost callback across reconciliation");
        const auto start=std::chrono::steady_clock::now();
        std::thread delayed([&]() {post(callback,30);});delayed.join();
        while(callbacks<2 && std::chrono::steady_clock::now()-start<std::chrono::seconds(2)) {pump();std::this_thread::sleep_for(std::chrono::milliseconds(1));}
        require(callbacks==2 && std::chrono::steady_clock::now()-start>=std::chrono::milliseconds(30),"delay fired early or failed");
        post(callback,30);std::this_thread::sleep_for(std::chrono::milliseconds(50));pump();
        require(callbacks==3,"delay restarted after UI queue wait");
        post(callback,80);pump(); // Arm the native timer, then remove its target below.
        auto tabs=CPPUI::ref("tabs"), views=CPPUI::ref("views"), window=CPPUI::ref("window");
        std::thread worker([=](){tabs.select("second"); views.select("second");}); worker.join();
        require(selected(tabs)==0 && !static_cast<CPPUI::OptionalText>(root.selection),"worker performed UI work synchronously");
        root.mount(); pump();
        require(selected(tabs)==1 && static_cast<CPPUI::OptionalText>(root.selection).value()=="second","queued command failed after reconciliation");
        tabs.select("first"); pump(); require(selected(tabs)==0,"first tab command failed");
#if defined(CPPUI_TEST_QT) || defined(CPPUI_TEST_WX)
        std::thread commands([window](){window.fullscreen(true);}); commands.join();
        pump();
#if defined(CPPUI_TEST_QT)
        require(static_cast<QWidget *>(window.node()->outer())->isFullScreen(),"fullscreen command failed");
#else
        require(static_cast<wxFrame *>(window.node()->outer())->IsFullScreen(),"fullscreen command failed");
#endif
        window.fullscreen(false); pump();
#endif
        views.select("first"); tabs.select("second"); root.show=false; root.mount(); pump();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));pump();post(callback);pump();
        require(callbacks==3,"removed target executed a pending or newly posted callback");
        require(!tabs.node() && !views.node(),"removed ref retained its node");
        bool expired=false; try { tabs.select("first"); } catch(const std::runtime_error &) { expired=true; }
        require(expired,"expired binder accepted command");
    } catch(const std::exception & error) { std::cerr<<error.what()<<'\n'; result=1; }
#if !defined(CPPUI_TEST_QT) && !defined(CPPUI_TEST_WX)
    lv_deinit();
#endif
    return result;
}

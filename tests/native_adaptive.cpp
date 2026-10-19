#if defined(CPPUI_TEST_QT)
#include "cppui/qt/backend.hpp"
#elif defined(CPPUI_TEST_WX)
#include "cppui/wx/backend.hpp"
#else
#include "cppui/lvgl/backend.hpp"
#endif
#include <thread>
static void require(bool value,const char * message) {if(!value) throw std::runtime_error(message);}
static CPPUI::Node * find(CPPUI::Node & root,const std::string & id) {
    if(root._id==id) return &root;
    for(auto child:*root.children) if(auto result=find(*child,id)) return result;
    return nullptr;
}
static void pump() {
    for(int i=0;i<20;++i) {
#if defined(CPPUI_TEST_QT)
        QCoreApplication::processEvents();QCoreApplication::sendPostedEvents(nullptr,QEvent::LayoutRequest);
#elif defined(CPPUI_TEST_WX)
        wxTheApp->Yield(true);wxTheApp->ProcessPendingEvents();wxTheApp->ProcessIdle();
#else
        lv_tick_inc(LV_DEF_REFR_PERIOD+1);lv_timer_handler();lv_obj_update_layout(lv_screen_active());
#endif
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
struct Root:CPPUI::Application {
    using Application::Application;
    bool present=true;
    int builds=0;
    CPPUI::State<std::string> text{std::string("Ada")};
    void content() override {
        ++builds;
        for(auto window:CPPUI::Window().id("window")) {
            if(!present) continue;
            auto & container=CPPUI::VBox();container.id("container").layout({}, {},1,CPPUI::Insets(10));
            auto size=container.size().get();
            for(auto scope:container) {
                CPPUI::Label(size ? std::to_string(size->width) : "waiting");
                for(auto compact:CPPUI::VBox().id("compact").visible(true,{{},600})) CPPUI::TextField(text).id("compact-input");
                for(auto wide:CPPUI::HBox().id("wide").visible(true,{600})) CPPUI::TextField(text).id("wide-input");
                CPPUI::Spacer();
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
static void resize(Root & root,int width) {
#if defined(CPPUI_TEST_QT)
    auto window=static_cast<QWidget *>(find(root,"window")->outer());window->resize(width,400);window->show();
#elif defined(CPPUI_TEST_WX)
    auto window=static_cast<wxWindow *>(find(root,"window")->outer());window->SetClientSize(width,400);window->Show();
#else
    (void)root;lv_display_set_resolution(lv_display_get_default(),width,400);
#endif
    pump();
}
int main(int argc,char ** argv) {
#if !defined(CPPUI_TEST_QT) && !defined(CPPUI_TEST_WX)
    lv_init();lv_display_create(800,400);
#endif
    int result=0;
    try {
        Root root(argc,argv);root.mount();resize(root,850);
        auto binding=find(root,"container")->size();auto state=find(root,"container")->sizeState;
        require(binding.get() && binding.get()->width>=600,"native size was not measured");
        require(find(root,"wide")->is_visible && !find(root,"compact")->is_visible,"wide branch did not show");
        auto wide=find(root,"wide-input")->outer();
        auto windowSize=find(root,"window")->size();pump();
        require(windowSize.get() && windowSize.get()->width>0,"observing an already mounted window failed");
        for(int width:{450,850,450,850}) {
            resize(root,width);
            require(find(root,"container")->sizeState==state,"native resize replaced reactive size state");
            require(find(root,"wide")->is_visible==(width==850) && find(root,"compact")->is_visible==(width==450),"resize did not switch adaptive layout");
        }
        require(find(root,"wide-input")->outer()==wide,"resizing replaced hidden native input");
        auto measured=binding.get();
#if defined(CPPUI_TEST_QT)
        auto widget=static_cast<QWidget *>(find(root,"container")->outer());
        auto content=widget->contentsRect().marginsRemoved(widget->layout()->contentsMargins());
        require(measured->width==content.width() && measured->height==content.height(),"Qt measurement includes content margins");
#elif defined(CPPUI_TEST_WX)
        auto sizer=static_cast<CPPUI::Wx::Node *>(find(root,"container"))->sizer();
        require(measured->width==sizer->GetSize().GetWidth() && measured->height==sizer->GetSize().GetHeight(),"wx sizer size differs");
#else
        auto object=static_cast<lv_obj_t *>(find(root,"container")->outer());
        require(measured->width==lv_obj_get_content_width(object) && measured->height==lv_obj_get_content_height(object),"LVGL measurement includes padding");
#endif
        int before=root.builds;pump();require(root.builds==before,"stable geometry kept rebuilding");
        resize(root,450);root.present=false;root.mount();pump();before=root.builds;
        resize(root,850);require(root.builds==before,"removed size observer kept rebuilding");
        root.present=true;root.mount();resize(root,850);
        find(root,"container")->sizeObserver->refresh();
#if defined(CPPUI_TEST_QT)
        delete static_cast<QWidget *>(find(root,"window")->outer());
#elif defined(CPPUI_TEST_WX)
        static_cast<wxWindow *>(find(root,"window")->outer())->Destroy();
#else
        lv_obj_clean(lv_screen_active());
#endif
        pump(); // Queued measurements must tolerate externally destroyed native objects.
    }catch(const std::exception & error) {std::cerr<<error.what()<<'\n';result=1;}
#if !defined(CPPUI_TEST_QT) && !defined(CPPUI_TEST_WX)
    lv_deinit();
#endif
    return result;
}

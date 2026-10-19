#if defined(CPPUI_TEST_QT)
#include "cppui/qt/application.hpp"
#include "cppui/qt/window.hpp"
#include "cppui/qt/layout.hpp"
#include "cppui/qt/label.hpp"
#include "cppui/qt/textfield.hpp"
using NativeView=CPPUI::Qt::QtView;
#elif defined(CPPUI_TEST_WX)
#include "cppui/wx/application.hpp"
#include "cppui/wx/window.hpp"
#include "cppui/wx/layout.hpp"
#include "cppui/wx/label.hpp"
#include "cppui/wx/textfield.hpp"
using NativeView=CPPUI::Wx::View;
#else
#include "cppui/lvgl/application.hpp"
#include "cppui/lvgl/window.hpp"
#include "cppui/lvgl/layout.hpp"
#include "cppui/lvgl/label.hpp"
#include "cppui/lvgl/textfield.hpp"
using NativeView=CPPUI::LVGL::View;
#endif
#include <thread>
static void require(bool ok,const char * message) { if(!ok) throw std::runtime_error(message); }
static CPPUI::Node * find(CPPUI::Node & root,const std::string & id) {
    if(root._id==id) return &root;
    for(auto child:*root.children) if(auto result=find(*child,id)) return result;
    return nullptr;
}
static void pump() {
    for(int i=0;i<8;++i) {
#if defined(CPPUI_TEST_QT)
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr,QEvent::LayoutRequest);
#elif defined(CPPUI_TEST_WX)
        wxTheApp->Yield(true);wxTheApp->ProcessPendingEvents();wxTheApp->ProcessIdle();
#else
        lv_tick_inc(LV_DEF_REFR_PERIOD+1);lv_timer_handler();lv_obj_update_layout(lv_screen_active());
#endif
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
static int nestedBuilds=0;
class Nested:public NativeView {
    CPPUI::State<int> & version;
public:
    explicit Nested(CPPUI::State<int> & version):version(version) {}
    void content() override {++nestedBuilds;CPPUI::Label(std::to_string(static_cast<int>(version))).id("nested");}
};
class Root:public CPPUI::Application {
public:
    using Application::Application;
    bool show=true,omit=false,gap=true;
    CPPUI::State<int> version{0};
    CPPUI::State<std::string> text{std::string("initial")};
    void content() override {
        for(auto window:CPPUI::Window().id("window").visible(false)) for(auto outer:CPPUI::VBox().id("outer")) {
            CPPUI::Label("before");
            auto & group=CPPUI::VBox();group.id("group").layout({},120,3,CPPUI::Insets(20));
            if(!omit) group.visible(show);
            for(auto scope:group) {
                CPPUI::TextField(text).id("editor").ref("editor");
                CPPUI::Label("must stay hidden").id("hidden").visible(false);
                (new Nested(version))->id("view");
            }
            CPPUI::Spacer().id("gap").visible(gap);
            CPPUI::Label("after").id("after");
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
static bool hidden(CPPUI::Node * node) {
#if defined(CPPUI_TEST_QT)
    return !static_cast<QWidget *>(node->outer())->isVisible();
#elif defined(CPPUI_TEST_WX)
    if(auto layout=dynamic_cast<CPPUI::Wx::Layout *>(node)) {
        auto owner=dynamic_cast<CPPUI::Wx::Layout *>(node->dom_parent);
        for(auto & mount:owner->mounts) if(mount.node==node) return !mount.item->IsShown();
    }
    return !static_cast<wxWindow *>(node->outer())->IsShownOnScreen();
#else
    return !lv_obj_is_visible(static_cast<lv_obj_t *>(node->outer()));
#endif
}
static void edit(void * editor) {
#if defined(CPPUI_TEST_QT)
    static_cast<QLineEdit *>(editor)->setText("draft");
#elif defined(CPPUI_TEST_WX)
    static_cast<wxTextCtrl *>(editor)->SetValue("draft");
#else
    lv_textarea_set_text(static_cast<lv_obj_t *>(editor),"draft");
#endif
}
static std::string text(void * editor) {
#if defined(CPPUI_TEST_QT)
    return static_cast<QLineEdit *>(editor)->text().toStdString();
#elif defined(CPPUI_TEST_WX)
    return static_cast<wxTextCtrl *>(editor)->GetValue().ToStdString(wxConvUTF8);
#else
    return lv_textarea_get_text(static_cast<lv_obj_t *>(editor));
#endif
}
int main(int argc,char ** argv) {
#if !defined(CPPUI_TEST_QT) && !defined(CPPUI_TEST_WX)
    lv_init();lv_display_create(640,480);
#endif
    int result=0;
    try {
        Root root(argc,argv);root.mount();
#if defined(CPPUI_TEST_QT)
        static_cast<QWidget *>(find(root,"window")->outer())->resize(400,400);
        static_cast<QWidget *>(find(root,"window")->outer())->show();
#elif defined(CPPUI_TEST_WX)
        static_cast<wxWindow *>(find(root,"window")->outer())->SetSize(400,400);
        static_cast<wxWindow *>(find(root,"window")->outer())->Show();
#endif
        pump();
        auto editor=find(root,"editor")->outer();auto group=find(root,"group")->outer();
        auto post=find(root,"editor")->getPoster();
        require(!hidden(find(root,"editor")) && hidden(find(root,"hidden")),"initial visibility was not applied");
        edit(editor);root.show=false;root.gap=false;root.mount();pump();
        require(find(root,"editor")->outer()==editor && find(root,"group")->outer()==group && text(editor)=="draft","hiding replaced native state");
        require(hidden(find(root,"group")) && hidden(find(root,"editor")),"hidden subtree remains visible");
#if defined(CPPUI_TEST_QT)
        auto layout=static_cast<CPPUI::Qt::BaseLayout *>(find(root,"outer"))->qtlayout;
        auto index=layout->indexOf(static_cast<QWidget *>(group));
        require(layout->itemAt(index)->isEmpty() && layout->stretch(index)==0,"hidden Qt layout retained space/stretch");
        auto spacer=static_cast<QSpacerItem *>(find(root,"gap")->outer());
        require(spacer->sizePolicy().verticalPolicy()==QSizePolicy::Fixed && spacer->geometry().height()==0,"hidden spacer retained expansion");
#elif defined(CPPUI_TEST_WX)
        auto layout=static_cast<CPPUI::Wx::Layout *>(find(root,"outer"));
        for(auto & mount:layout->mounts) if(mount.node->_id=="group" || mount.node->_id=="gap")
            require(!mount.item->IsShown() && mount.item->CalcMin()==wxSize(0,0),"hidden wx item retained padding/space");
#else
        require(lv_obj_is_hidden(static_cast<lv_obj_t *>(group)) && lv_obj_is_hidden(static_cast<lv_obj_t *>(find(root,"gap")->outer())),"LVGL hidden layout/spacer flags missing");
#endif
        const int before=nestedBuilds;root.version=1;pump();
        require(nestedBuilds==before,"hidden view rebuilt its content");
        require(hidden(find(root,"nested")),"independent hidden subtree update exposed a child");
        int called=0;post([&](){++called;});pump();require(called==1,"hidden node stopped UI callbacks");
        root.omit=true;root.mount();pump(); // Omission restores the default.
        require(!hidden(find(root,"editor")) && hidden(find(root,"hidden")),"ancestor show exposed an explicitly hidden child");
        require(find(root,"editor")->outer()==editor && text(editor)=="draft","showing lost native editor state");
        require(nestedBuilds>before,"shown view did not refresh pending state");
        require(!hidden(find(root,"window")),"Window unexpectedly accepted visibility modifier");
        Root lazy(argc,argv);lazy.show=false;
        const int beforeLazy=nestedBuilds;lazy.mount();pump();
        require(!find(lazy,"editor") && nestedBuilds==beforeLazy,"initially hidden subtree mounted children");
        lazy.version=3;lazy.show=true;lazy.mount();pump();
        require(find(lazy,"editor") && nestedBuilds>beforeLazy,"show did not lazily mount children");
    } catch(const std::exception & error) { std::cerr<<error.what()<<'\n';result=1; }
#if !defined(CPPUI_TEST_QT) && !defined(CPPUI_TEST_WX)
    lv_deinit();
#endif
    return result;
}

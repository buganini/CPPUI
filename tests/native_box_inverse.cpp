#if defined(CPPUI_TEST_QT)
#include "cppui/qt/backend.hpp"
#elif defined(CPPUI_TEST_WX)
#include "cppui/wx/backend.hpp"
#else
#include "cppui/lvgl/backend.hpp"
#endif
#include <thread>
static void require(bool value,const char * message) { if(!value) throw std::runtime_error(message); }
static CPPUI::Node * find(CPPUI::Node & root,const std::string & id) {
    if(root._id==id) return &root;
    for(auto child:*root.children) if(auto result=find(*child,id)) return result;
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
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
struct Fragment:CPPUI::Backend::View {
    bool extra=false;
    CPPUI::State<std::string> text{std::string("original")};
    void content() override {
        CPPUI::Label("B").id("b");
        CPPUI::TextField(text).id("editor");
        CPPUI::TextField(text);CPPUI::TextField(text); // Equal anonymous identities retain declaration occurrence.
        if(extra) CPPUI::Label("Extra").id("extra");
    }
};
struct Nested:CPPUI::Backend::View {
    void content() override { CPPUI::SubView<Fragment>().id("fragment"); }
};
struct Root:CPPUI::Application {
    using Application::Application;
    int kind=0;
    void * anonymousFirst=nullptr,* anonymousSecond=nullptr;
    bool inverseH=false,inverseV=false,primary=true,present=true,hide=false,prepend=false;
    void content() override {
        for(auto window:CPPUI::Window().id("window"))
            for(auto outer:CPPUI::VBox().layout(CPPUI::layout::weight(1))) {
                if(!present) continue;
                CPPUI::Node * box=nullptr;
                switch(kind) {
                    case 0:box=&CPPUI::HBox(inverseH);break;
                    case 1:box=&CPPUI::VBox(inverseV);break;
                    case 2:box=&CPPUI::HVBox(primary ? 1 : 10000,inverseH,inverseV);break;
                    default:box=&CPPUI::VHBox(primary ? 1 : 10000,inverseH,inverseV);break;
                }
                for(auto scope:box->id("box").layout(CPPUI::layout::weight(1))) {
                    if(prepend) CPPUI::Label("Prefix").id("prefix");
                    CPPUI::Label("A").id("a").visible(!hide);
                    CPPUI::SubView<Nested>();
                    CPPUI::Divider().id("divider");
                    CPPUI::Label("C").id("c");
                    for(auto nested:CPPUI::HBox().id("nested")) {
                        CPPUI::Label("one").id("one");CPPUI::Label("two").id("two");
                    }
                    CPPUI::Spacer().id("spacer");
                }
            }
    }
    void mount() {
#if defined(CPPUI_TEST_QT) || defined(CPPUI_TEST_WX)
        sync();
#else
        redraw();
#endif
        pump();
    }
};
static int position(CPPUI::Node * node,bool horizontal) {
#if defined(CPPUI_TEST_QT)
    auto p=static_cast<QWidget *>(node->outer())->pos();return horizontal ? p.x() : p.y();
#elif defined(CPPUI_TEST_WX)
    auto p=static_cast<wxWindow *>(node->outer())->GetPosition();return horizontal ? p.x : p.y;
#else
    auto object=static_cast<lv_obj_t *>(node->outer());return horizontal ? lv_obj_get_x(object) : lv_obj_get_y(object);
#endif
}
static void setDraft(CPPUI::Node * node,bool focus=true) {
#if defined(CPPUI_TEST_QT)
    auto editor=static_cast<QLineEdit *>(node->outer());if(focus) {editor->setFocus();pump();}editor->setText("draft");if(focus) editor->setSelection(1,2);
#elif defined(CPPUI_TEST_WX)
    auto editor=static_cast<wxTextCtrl *>(node->outer());if(focus) {editor->SetFocus();pump();}editor->ChangeValue("draft");if(focus) editor->SetSelection(1,3);
#else
    auto editor=static_cast<lv_obj_t *>(node->outer());lv_textarea_set_text(editor,"draft");lv_textarea_set_cursor_pos(editor,2);
#endif
}
static void checkDraft(CPPUI::Node * node,bool selection=true) {
#if defined(CPPUI_TEST_QT)
    auto editor=static_cast<QLineEdit *>(node->outer());require(editor->text()=="draft" && (!selection || editor->selectedText()=="ra"),"Qt reversal lost draft/selection");
#elif defined(CPPUI_TEST_WX)
    auto editor=static_cast<wxTextCtrl *>(node->outer());long a,b;editor->GetSelection(&a,&b);require(editor->GetValue()=="draft" && (!selection || (a==1 && b==3)),"wx reversal lost draft/selection");
#else
    auto editor=static_cast<lv_obj_t *>(node->outer());require(std::string(lv_textarea_get_text(editor))=="draft" && (!selection || lv_textarea_get_cursor_pos(editor)==2),"LVGL reversal lost draft/caret");
#endif
}
static void check(Root & root,void * editor,void * divider) {
    bool horizontal=root.kind==0 || (root.kind==2 && root.primary) || (root.kind==3 && !root.primary);
    bool inverse=horizontal ? root.inverseH : root.inverseV;
    require((position(find(root,"b"),horizontal)>position(find(root,"c"),horizontal))==inverse,"box order differs from selected axis/reversal");
    require(position(find(root,"one"),true)<position(find(root,"two"),true),"reversal escaped into a physical child box");
    require(find(root,"editor")->outer()==editor && find(root,"divider")->outer()==divider,"reversal replaced native controls");
    checkDraft(find(root,"editor"));
    auto fragment=static_cast<Fragment *>(find(root,"fragment"));
    checkDraft(fragment->children->at(2),false);checkDraft(fragment->children->at(3),false);
    require(fragment->children->at(2)->outer()==root.anonymousFirst && fragment->children->at(3)->outer()==root.anonymousSecond,
        "reversal exchanged anonymous declaration occurrences");
#if defined(CPPUI_TEST_QT)
    auto line=static_cast<QFrame *>(divider);require(line->frameShape()==(horizontal ? QFrame::VLine : QFrame::HLine),"Qt divider orientation stale");
    require((horizontal ? line->width() : line->height())<=4,"Qt divider thickness expanded");
#elif defined(CPPUI_TEST_WX)
    auto line=static_cast<wxStaticLine *>(divider);require(line->IsVertical()==horizontal,"wx divider orientation stale");
    require((horizontal ? line->GetSize().x : line->GetSize().y)<=4,"wx divider thickness expanded");
#endif
}
int main(int argc,char ** argv) {
#if !defined(CPPUI_TEST_QT) && !defined(CPPUI_TEST_WX)
    lv_init();lv_display_create(900,700);
#endif
    int result=0;
    try {
        Root root(argc,argv);
        for(int kind=0;kind<4;++kind) {
            root.kind=kind;root.mount();
#if defined(CPPUI_TEST_QT)
            auto window=static_cast<QWidget *>(find(root,"window")->outer());window->resize(900,700);window->show();
#elif defined(CPPUI_TEST_WX)
            auto window=static_cast<wxWindow *>(find(root,"window")->outer());window->SetClientSize(900,700);window->Show();
#endif
            pump();root.mount();
            auto editor=find(root,"editor")->outer(),divider=find(root,"divider")->outer();
            auto fragment=static_cast<Fragment *>(find(root,"fragment"));
            root.anonymousFirst=fragment->children->at(2)->outer();root.anonymousSecond=fragment->children->at(3)->outer();
            setDraft(fragment->children->at(2),false);setDraft(fragment->children->at(3),false);setDraft(find(root,"editor"));
            for(bool primary:{true,false,true}) for(int flags:{0,1,2,3,0}) {
                root.primary=primary;root.inverseH=flags&1;root.inverseV=flags&2;root.mount();check(root,editor,divider);
                auto fragment=static_cast<Fragment *>(find(root,"fragment"));
                fragment->extra=true;fragment->redraw();pump();check(root,editor,divider);
                require(find(root,"extra"),"independent nested insertion lost");
                root.hide=true;root.prepend=true;root.mount();check(root,editor,divider);
                fragment->extra=false;fragment->redraw();pump();check(root,editor,divider);
                root.hide=false;root.prepend=false;root.mount();check(root,editor,divider);
            }
            root.present=false;root.mount();root.present=true;
        }
        std::cout<<"native inverse boxes passed\n";
    } catch(const std::exception & error) {std::cerr<<error.what()<<'\n';result=1;}
#if !defined(CPPUI_TEST_QT) && !defined(CPPUI_TEST_WX)
    lv_deinit();
#endif
    return result;
}

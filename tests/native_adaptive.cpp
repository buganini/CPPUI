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
struct BoxContents:CPPUI::Backend::View {
    CPPUI::State<std::string> text{std::string("Ada")};
    void content() override {
        for(auto box:CPPUI::HVBox(600).id("hv")) {
            CPPUI::Label("Name").id("name");
            CPPUI::TextField(text).id("editor");
            CPPUI::Divider().id("divider");
            CPPUI::Spacer().id("spacer");
        }
        for(auto box:CPPUI::VHBox(500).id("vh")) {
            CPPUI::Label("First");CPPUI::Label("Second");
        }
    }
};
struct BoxesRoot:Root {
    using Root::Root;
    void content() override {
        ++builds;
        for(auto window:CPPUI::Window().id("window"))
            for(auto container:CPPUI::VBox().id("container").layout({}, {},1,CPPUI::Insets(10)))
                CPPUI::SubView<BoxContents>();
    }
};
static void resize(Root & root,int width,int height=400) {
#if defined(CPPUI_TEST_QT)
    auto window=static_cast<QWidget *>(find(root,"window")->outer());window->resize(width,height);window->show();
#elif defined(CPPUI_TEST_WX)
    auto window=static_cast<wxWindow *>(find(root,"window")->outer());window->SetClientSize(width,height);window->Show();
#else
    (void)root;lv_display_set_resolution(lv_display_get_default(),width,height);
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
        {
            BoxesRoot boxes(argc,argv);boxes.mount();resize(boxes,850,700);
            auto hv=find(boxes,"hv")->outer(),vh=find(boxes,"vh")->outer();
            auto editor=find(boxes,"editor")->outer();
#if defined(CPPUI_TEST_QT)
            auto input=static_cast<QLineEdit *>(editor);input->setText("Draft");input->setCursorPosition(2);
#elif defined(CPPUI_TEST_WX)
            auto input=static_cast<wxTextCtrl *>(editor);input->SetValue("Draft");input->SetInsertionPoint(2);
#else
            auto input=static_cast<lv_obj_t *>(editor);lv_textarea_set_text(input,"Draft");lv_textarea_set_cursor_pos(input,2);
#endif
            for(auto size:{CPPUI::Size(450,300),CPPUI::Size(850,300),CPPUI::Size(450,700),CPPUI::Size(850,700)}) {
                resize(boxes,size.width,size.height);
                auto measured=find(boxes,"container")->size().get();
                require(bool(measured),"adaptive box failed to observe physical parent through a virtual view");
                const bool horizontal=measured->width>=600,vertical=measured->height>=500;
                require(horizontal==(size.width==850) && vertical==(size.height==700),"test window did not cross the box thresholds");
                require(find(boxes,"hv")->outer()==hv && find(boxes,"vh")->outer()==vh && find(boxes,"editor")->outer()==editor,
                        "adaptive direction change replaced a native container/editor");
#if defined(CPPUI_TEST_QT)
                require(static_cast<CPPUI::Qt::BaseLayout *>(find(boxes,"hv"))->qtlayout->direction()==(horizontal ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom)
                    && static_cast<CPPUI::Qt::BaseLayout *>(find(boxes,"vh"))->qtlayout->direction()==(vertical ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight),"Qt box direction differs");
                require(static_cast<QFrame *>(find(boxes,"divider")->outer())->frameShape()==(horizontal ? QFrame::VLine : QFrame::HLine),"Qt divider did not rotate");
                auto policy=static_cast<QSpacerItem *>(find(boxes,"spacer")->outer())->sizePolicy();
                require(policy.horizontalPolicy()==(horizontal ? QSizePolicy::Expanding : QSizePolicy::Preferred) &&
                        policy.verticalPolicy()==(horizontal ? QSizePolicy::Preferred : QSizePolicy::Expanding),"Qt spacer did not rotate");
                require(input->text()=="Draft" && input->cursorPosition()==2,"Qt direction change lost editor draft/cursor");
#elif defined(CPPUI_TEST_WX)
                require(static_cast<wxBoxSizer *>(hv)->GetOrientation()==(horizontal ? wxHORIZONTAL : wxVERTICAL)
                    && static_cast<wxBoxSizer *>(vh)->GetOrientation()==(vertical ? wxVERTICAL : wxHORIZONTAL),"wx box direction differs");
                auto box=static_cast<CPPUI::Wx::Node *>(find(boxes,"hv"));
                require(box->horizontal==horizontal && box->vertical!=horizontal && box->expandX==(horizontal ? 2 : 1) && box->expandY==(horizontal ? 1 : 2),"wx expansion policy did not rotate");
                require(input->GetValue()=="Draft" && input->GetInsertionPoint()==2,"wx direction change lost editor draft/cursor");
#else
                require(lv_obj_get_style_flex_flow(static_cast<lv_obj_t *>(hv),LV_PART_MAIN)==(horizontal ? LV_FLEX_FLOW_ROW : LV_FLEX_FLOW_COLUMN)
                    && lv_obj_get_style_flex_flow(static_cast<lv_obj_t *>(vh),LV_PART_MAIN)==(vertical ? LV_FLEX_FLOW_COLUMN : LV_FLEX_FLOW_ROW),"LVGL box direction differs");
                auto label=static_cast<lv_obj_t *>(find(boxes,"name")->outer());
                require(lv_obj_get_style_width(label,LV_PART_MAIN)==(horizontal ? LV_SIZE_CONTENT : lv_pct(100)),"LVGL label retained the old column's width");
                auto divider=static_cast<lv_obj_t *>(find(boxes,"divider")->outer());
                require(lv_obj_get_style_width(divider,LV_PART_MAIN)==(horizontal ? 1 : lv_pct(100)) &&
                        lv_obj_get_style_height(divider,LV_PART_MAIN)==(horizontal ? lv_pct(100) : 1),"LVGL divider did not rotate");
                require(std::string(lv_textarea_get_text(input))=="Draft" && lv_textarea_get_cursor_pos(input)==2,"LVGL direction change lost editor draft/cursor");
#endif
            }
        }
    }catch(const std::exception & error) {std::cerr<<error.what()<<'\n';result=1;}
#if !defined(CPPUI_TEST_QT) && !defined(CPPUI_TEST_WX)
    lv_deinit();
#endif
    return result;
}

#include "cppui/backend.hpp"
#include <stdexcept>
#if defined(CPPUI_BACKEND_QT)
#include <QFocusEvent>
#endif
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
    CPPUI::State<std::string> value{std::string("initial")},draft{std::string()};
    bool readonly=false,wrap=true,normalize=false,present=true,removeOnChange=false;
    int inputs=0,changes=0;
    void content() override {
        for(auto w:CPPUI::Window().size(700,450)) for(auto box:CPPUI::VBox()) {
            if(present) CPPUI::TextEdit(CPPUI::Binding<std::string>([this](){return std::string(value);},[this](const std::string & text){value=normalize ? "normalized" : text;}),draft)
                .readonly(readonly).wrap(wrap).placeholder("Several lines")
                .input([this](){++inputs;}).change([this](){++changes;if(removeOnChange) {present=false;redraw();}}).id("edit");
            CPPUI::Button("Other");CPPUI::Label(draft).id("draft");
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
    void * editor() {return find(*this,"edit")->outer();}
};
static std::string text(void * p) {
#if defined(CPPUI_BACKEND_QT)
    return static_cast<QPlainTextEdit *>(p)->toPlainText().toStdString();
#elif defined(CPPUI_BACKEND_WX)
    return static_cast<wxTextCtrl *>(p)->GetValue().ToStdString(wxConvUTF8);
#else
    return lv_textarea_get_text(static_cast<lv_obj_t *>(p));
#endif
}
static void input(void * p,const std::string & value) {
#if defined(CPPUI_BACKEND_QT)
    static_cast<QPlainTextEdit *>(p)->setPlainText(QString::fromStdString(value));
#elif defined(CPPUI_BACKEND_WX)
    static_cast<wxTextCtrl *>(p)->SetValue(wxString::FromUTF8(value));
#else
    lv_textarea_set_text(static_cast<lv_obj_t *>(p),value.c_str());
#endif
    pump();
}
static void blur(void * p) {
#if defined(CPPUI_BACKEND_QT)
    QFocusEvent event(QEvent::FocusOut);QApplication::sendEvent(static_cast<QPlainTextEdit *>(p),&event);
#elif defined(CPPUI_BACKEND_WX)
    wxFocusEvent event(wxEVT_KILL_FOCUS);static_cast<wxTextCtrl *>(p)->ProcessWindowEvent(event);
#else
    lv_obj_send_event(static_cast<lv_obj_t *>(p),LV_EVENT_DEFOCUSED,nullptr);
#endif
    pump();
}
int main(int argc,char ** argv) {
#if defined(CPPUI_BACKEND_LVGL)
    lv_init();lv_display_create(700,450);
#endif
    int result=0;
    try {
        Root root(argc,argv);root.mount();auto editor=root.editor();
        require(std::string(root.draft)=="initial" && root.inputs==0 && root.changes==0,"initial draft sync emitted events");
        input(editor,"one\ntwo");require(text(editor)=="one\ntwo" && std::string(root.draft)=="one\ntwo" && std::string(root.value)=="initial","newline input committed or lost draft");
        root.wrap=false;root.mount();require(root.editor()==editor && text(editor)=="one\ntwo","wrap change replaced editor/draft");
        blur(editor);require(std::string(root.value)=="one\ntwo" && root.changes==1,"blur did not commit once");
        blur(editor);require(root.changes==1,"clean blur emitted duplicate change");
        root.normalize=true;input(editor,"untrimmed\ntext");blur(editor);
        require(text(editor)=="normalized" && std::string(root.draft)=="normalized" && root.changes==2,"normalized commit not reflected in draft/editor");
        input(editor,"uncommitted");root.readonly=true;root.mount();
        require(text(editor)=="normalized" && std::string(root.draft)=="normalized","readonly did not withdraw pending draft");
        int before=root.inputs;root.value="external\nvalue";root.mount();
        require(text(editor)=="external\nvalue" && root.inputs==before,"model update emitted input");
#if defined(CPPUI_BACKEND_QT)
        require(static_cast<QPlainTextEdit *>(editor)->isReadOnly(),"Qt readonly missing");
#elif defined(CPPUI_BACKEND_WX)
        require(!static_cast<wxTextCtrl *>(editor)->IsEditable(),"wx readonly missing");
#else
        lv_textarea_add_text(static_cast<lv_obj_t *>(editor),"blocked");lv_textarea_delete_char(static_cast<lv_obj_t *>(editor));pump();
        require(text(editor)=="external\nvalue" && root.inputs==before,"readonly accepted insertion/deletion");
#endif
        root.readonly=false;root.wrap=true;root.normalize=false;root.mount();input(editor,"last\nedit");blur(editor);
        require(root.editor()==editor && std::string(root.value)=="last\nedit","editor did not resume after readonly");
        root.removeOnChange=true;input(editor,"remove on commit");blur(editor);require(!find(root,"edit"),"change callback removal failed");
    } catch(const std::exception & e) {std::cerr<<e.what()<<'\n';result=1;}
#if defined(CPPUI_BACKEND_LVGL)
    lv_deinit();
#endif
    return result;
}

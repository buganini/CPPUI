#include "cppui/backend.hpp"
#include <stdexcept>
static void require(bool ok,const char * message) {if(!ok)throw std::runtime_error(message);}
static CPPUI::Node * find(CPPUI::Node & n,const char * id) {if(n._id==id)return &n;for(auto c:*n.children)if(auto p=find(*c,id))return p;return nullptr;}
static void pump() {for(int i=0;i<8;++i){
#if defined(CPPUI_BACKEND_QT)
    QCoreApplication::processEvents();QCoreApplication::sendPostedEvents(nullptr,QEvent::LayoutRequest);
#else
    wxTheApp->Yield(true);wxTheApp->ProcessPendingEvents();wxTheApp->ProcessIdle();
#endif
}}
struct Root:CPPUI::Application {
    using Application::Application;
    CPPUI::State<bool> opened{false};
    bool hints=true,spacer=true,reverse=false,nativeMenus=false;
    int opens=0,closes=0;
    CPPUI::Optional<int> background;
    void content() override {
        for(auto w:CPPUI::Window().size(640,420).id("window")) {
            for(auto bar:CPPUI::MenuBar().native(nativeMenus).id("menubar")) {
                for(int n=0;n<2;++n) {
                    if(spacer && n==1) CPPUI::Spacer().id("menu-spacer");
                    int i=reverse?1-n:n;
                    for(auto m:CPPUI::Menu(i?"Second":"First").id(i?"m2":"m1")) CPPUI::MenuAction("About").native(false).id(i?"a2":"a1");
                }
            }
            auto & declaration=CPPUI::VBox();declaration.id("panel");
            if(background) declaration.style(CPPUI::style::bgColor(*background));
            for(auto box:declaration) {
                auto & label=CPPUI::Label("A & B");if(hints)label.layout(CPPUI::layout::width(70),CPPUI::layout::minWidth(130),CPPUI::layout::minHeight(35));label.id("label");
                CPPUI::Button("A & B").id("button");
                CPPUI::Html("<a href='https://example.org'>Link</a>").openURL(false).id("html");
                CPPUI::Spacer();
                for(auto m:CPPUI::Modal(opened).title("Modal").size(250,130).open([this](){++opens;}).close([this](){++closes;}).id("modal")) CPPUI::Label("Inside");
            }
        }
    }
    void mount(){sync();pump();}
};
int main(int argc,char ** argv) {
    try {
        Root root(argc,argv);root.mount();auto label=find(root,"label")->outer();auto modal=find(root,"modal")->outer();
#if defined(CPPUI_BACKEND_QT)
        auto nativeLabel=static_cast<QLabel *>(label);require(nativeLabel->minimumWidth()==130 && nativeLabel->minimumHeight()==35,"Qt minimum hints");
        require(static_cast<QLabel *>(find(root,"html")->outer())->textInteractionFlags()==Qt::NoTextInteraction,"Html links disabled");
#else
        auto nativeLabel=static_cast<wxStaticText *>(label);require(nativeLabel->GetMinSize()==wxSize(130,35),"wx minimum hints");
        require(nativeLabel->GetLabelText()=="A & B" && static_cast<wxButton *>(find(root,"button")->outer())->GetLabelText()=="A & B","literal ampersands");
#endif
        #if defined(CPPUI_BACKEND_WX)
        auto panel=static_cast<wxPanel *>(find(root,"panel")->outer());
        const auto defaultBackground=panel->GetBackgroundColour();
        for(int color:{0x244B5A,0xA84A3A,-1}) {
            root.background=color<0 ? CPPUI::Optional<int>() : CPPUI::Optional<int>(color);root.mount();
            require(find(root,"panel")->outer()==panel && find(root,"label")->outer()==label && nativeLabel->GetParent()==panel,"layout background change replaced native panel or child");
            require(panel->GetBackgroundColour()==(color<0 ? defaultBackground : CPPUI::Wx::Widget::colour(color)),"layout background did not apply or reset");
        }
#endif
        root.hints=false;root.mount();require(find(root,"label")->outer()==label,"hint update replaced widget");
#if defined(CPPUI_BACKEND_QT)
        require(nativeLabel->minimumWidth()<130,"Qt minimum hint omitted");
#else
        require(nativeLabel->GetMinSize().x<130,"wx minimum hint omitted");
#endif
        for(int i=0;i<3;++i) {
            root.reverse=!root.reverse;root.spacer=!root.spacer;root.mount();
#if defined(CPPUI_BACKEND_QT)
            auto bar=static_cast<QMenuBar *>(find(root,"menubar")->outer());require(bar->actions().size()==2 && bar->actions()[0]->text()==(root.reverse?"Second":"First"),"Qt menu logical slots");
            require(static_cast<QAction *>(find(root,"a1")->outer())->menuRole()==QAction::NoRole,"menu role");
#else
            auto bar=static_cast<wxMenuBar *>(find(root,"menubar")->outer());require(bar->GetMenuCount()==2 && bar->GetMenuLabel(0)==(root.reverse?"Second":"First"),"wx menu logical slots");
#endif
        }
        root.opened=true;pump();require(root.opens==1,"Modal open transition");
#if defined(CPPUI_BACKEND_QT)
        auto dialog=static_cast<QDialog *>(modal);require(dialog->isVisible(),"Qt Modal shown");dialog->reject();
#else
        auto dialog=static_cast<wxDialog *>(modal);require(dialog->IsShown() && !static_cast<wxWindow *>(find(root,"window")->outer())->IsEnabled(),"wx app modality");
        wxKeyEvent escape(wxEVT_CHAR_HOOK);escape.m_keyCode=WXK_ESCAPE;dialog->ProcessWindowEvent(escape);
#endif
        pump();require(!bool(root.opened) && root.closes==1,"Escape/reject updates bound state");
        root.opened=true;pump();require(root.opens==2 && find(root,"modal")->outer()==modal,"Modal reopen retains window");
        root.opened=false;pump();require(root.closes==2,"Modal close transition");
#if defined(CPPUI_BACKEND_WX)
        require(static_cast<wxWindow *>(find(root,"window")->outer())->IsEnabled(),"wx owner restored");
#endif
    }catch(const std::exception & e){std::cerr<<e.what()<<'\n';return 1;}
}

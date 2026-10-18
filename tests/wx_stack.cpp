#include "cppui/wx/application.hpp"
#include "cppui/wx/window.hpp"
#include "cppui/wx/layout.hpp"
#include "cppui/wx/button.hpp"
#include "cppui/wx/label.hpp"
#include <wx/stopwatch.h>
static void require(bool ok,const char * message) { if(!ok) throw std::runtime_error(message); }
static void pump() {
    wxStopWatch clock;
    do { wxTheApp->Yield(true); wxTheApp->ProcessPendingEvents(); wxTheApp->ProcessIdle(); wxMilliSleep(1); }
    while(clock.Time()<20);
}
static CPPUI::Node * find(CPPUI::Node & root,const std::string & id) {
    if(root._id==id) return &root;
    for(auto child:*root.children) if(auto result=find(*child,id)) return result;
    return nullptr;
}
class Root: public CPPUI::Application {
public:
    using Application::Application;
    bool reverse=false,empty=false,colored=true;
    void layer(const char * id) {
        for(auto column:CPPUI::VBox().id(id).layout({}, {}, {}, CPPUI::Insets(0)))
            CPPUI::Button(id).id(std::string(id)+"-button").layout(100,50,{},CPPUI::Insets(0));
    }
    void content() override {
        for(auto window:CPPUI::Window().size(320,240).id("window")) {
            auto & stack=CPPUI::Stack(); stack.id("stack").layout({}, {}, {}, CPPUI::Insets(0));
            if(colored) stack.style(0x123456,0xabcdef);
            for(auto scope:stack) if(!empty) {
                if(reverse) layer("top");
                layer("bottom");
                if(!reverse) layer("top");
                CPPUI::Spacer();
            }
        }
    }
};
int main(int argc,char ** argv) {
    wxSetAssertHandler([](const wxString &,int,const wxString &,const wxString & c,const wxString & m){std::cerr<<c<<": "<<m<<'\n';std::abort();});
    try {
        Root root(argc,argv); root.sync(); pump();
        auto panel=static_cast<wxWindow *>(find(root,"stack")->outer());
        auto bottom=static_cast<wxWindow *>(find(root,"bottom-button")->outer());
        auto top=static_cast<wxWindow *>(find(root,"top-button")->outer());
        require(dynamic_cast<wxPanel *>(panel) && bottom->GetParent()==panel && top->GetParent()==panel,"Stack does not own a native panel");
        require(panel->GetForegroundColour()==wxColour(0x12,0x34,0x56) && bottom->GetForegroundColour()!=panel->GetForegroundColour(),"own foreground leaked into child");
        require(panel->GetBackgroundColour()==wxColour(0xab,0xcd,0xef),"Stack background was not applied");
        require(bottom->GetRect()==top->GetRect(),"nested layers do not overlap");
        for(int i=0;i<8;++i) {
            root.reverse=!root.reverse; root.sync(); pump();
            require(find(root,"stack")->outer()==panel && find(root,"bottom-button")->outer()==bottom && find(root,"top-button")->outer()==top,"keyed reorder replaced native layers");
            require(bottom->GetRect()==top->GetRect(),"reordered layers do not overlap");
        }
        auto before=panel->GetSize();
        static_cast<wxWindow *>(find(root,"window")->outer())->SetClientSize(500,400);pump();
        require(panel->GetSize()!=before && panel->GetSizer()->GetSize()==panel->GetClientSize() && bottom->GetRect()==top->GetRect(),"Stack did not follow parent resize");
        root.colored=false;root.sync();pump();
        require(panel->GetForegroundColour()!=wxColour(0x12,0x34,0x56),"omitted Stack color was not restored");
        root.empty=true;root.sync();pump();
        require(panel->GetSizer()->GetItemCount()==0 && panel->GetChildren().empty(),"empty Stack retained native children or wrappers");
        root.empty=false;root.sync();pump();
        require(panel->GetSizer()->GetItemCount()==3,"empty Stack could not repopulate");
    } catch(const std::exception & error) { std::cerr<<error.what()<<'\n';return 1; }
}

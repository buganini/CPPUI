#include "cppui/wx/application.hpp"
#include "cppui/wx/window.hpp"
#include "cppui/wx/layout.hpp"
#include "cppui/wx/label.hpp"
#include "cppui/wx/button.hpp"
#include "cppui/wx/scroll.hpp"
#include "cppui/wx/tab.hpp"
#include "cppui/wx/dialog.hpp"
#include "cppui/wx/timeline.hpp"
#include <wx/stopwatch.h>
#include <stdexcept>
static void require(bool c,const char * m){if(!c)throw std::runtime_error(m);}
static void pump(int ms=10){wxStopWatch clock;do{wxTheApp->Yield(true);wxTheApp->ProcessPendingEvents();wxTheApp->ProcessIdle();wxMilliSleep(1);}while(clock.Time()<ms);}
static CPPUI::Node * find(CPPUI::Node & root,const std::string & id){if(root._id==id)return &root;for(auto c:*root.children)if(auto n=find(*c,id))return n;return nullptr;}
class Root:public CPPUI::Application {
public:
    using Application::Application;
    int rows=50,builds=0,version=1,calls=0;
    bool reverse=false,showMenu=true,ticking=false;
    double offset=CPPUI::ScrollEnd;
    void content() override {
        ++builds;
        for(auto w:CPPUI::Window().size(420,240)) {
            if(showMenu)for(auto bar:CPPUI::MenuBar())for(auto menu:CPPUI::Menu("Menu")) {
                int v=version;CPPUI::MenuAction("Action").trigger([this,v](){calls+=v;showMenu=false;sync();}).id("action");
                for(auto nested:CPPUI::Menu("Nested"))CPPUI::MenuAction("child");
            }
            for(auto tabs:CPPUI::Tabs().id("tabs")) {
                if(reverse)for(auto tab:CPPUI::Tab("Second").id("second"))CPPUI::Label("second");
                for(auto tab:CPPUI::Tab("First").id("first"))for(auto scroll:CPPUI::Scroll().scrollY(offset).id("scroll"))for(auto box:CPPUI::VBox())
                    for(int i=0;i<rows;++i)CPPUI::Label(std::to_string(i)).layout({},24).id(std::to_string(i));
                if(!reverse)for(auto tab:CPPUI::Tab("Second").id("second"))CPPUI::Label("second");
                if(ticking)for(auto timer:CPPUI::TimelineView(.01)){} // virtual, no page
            }
        }
    }
};
class Answer:public wxTimer {
    std::string value;bool accept;bool done=false;
public:
    Answer(const std::string & value,bool accept):value(value),accept(accept){Start(1);}
    void Notify() override {
        for(auto window:wxTopLevelWindows)if(auto dialog=dynamic_cast<wxTextEntryDialog *>(window))if(dialog->IsModal()){
            Stop();done=true;dialog->SetValue(wxString::FromUTF8(value));dialog->EndModal(accept?wxID_OK:wxID_CANCEL);return;
        }
    }
    void check(){require(done,"prompt was not driven");}
};
int main(int argc,char **argv){
    wxSetAssertHandler([](const wxString &,int,const wxString &,const wxString &c,const wxString &m){std::cerr<<c<<": "<<m<<'\n';std::abort();});
    try {
        Root root(argc,argv);root.sync();pump();
        auto scroll=static_cast<CPPUI::Wx::Scroll *>(find(root,"scroll"));
        require(scroll->maximum(1)>0 && static_cast<wxScrolledWindow *>(scroll->ui)->GetViewStart().y==scroll->maximum(1),"scroll did not anchor to end");
        root.rows=70;root.sync();pump();scroll=static_cast<CPPUI::Wx::Scroll *>(find(root,"scroll"));
        require(static_cast<wxScrolledWindow *>(scroll->ui)->GetViewStart().y==scroll->maximum(1),"end anchor did not follow growth");
        static_cast<wxScrolledWindow *>(scroll->ui)->Scroll(0,80);root.sync();pump();scroll=static_cast<CPPUI::Wx::Scroll *>(find(root,"scroll"));
        require(static_cast<wxScrolledWindow *>(scroll->ui)->GetViewStart().y==80,"redraw overwrote manual scroll position");
        root.offset=-20;root.sync();pump();scroll=static_cast<CPPUI::Wx::Scroll *>(find(root,"scroll"));
        require(static_cast<wxScrolledWindow *>(scroll->ui)->GetViewStart().y==scroll->maximum(1)-20,"changed scroll request ignored");
        auto notebook=static_cast<wxNotebook *>(find(root,"tabs")->outer());auto first=notebook->GetPage(0);notebook->ChangeSelection(0);
        root.reverse=true;root.sync();pump();require(notebook->GetPage(1)==first&&notebook->GetSelection()==1,"keyed tab reorder lost page/selection");
        root.version=3;root.sync();auto action=static_cast<CPPUI::Wx::MenuAction *>(find(root,"action"));
        wxCommandEvent event(wxEVT_MENU,action->ui->GetId());static_cast<wxWindow *>(root.children->at(0)->outer())->ProcessWindowEvent(event);pump();
        require(root.calls==3&&!find(root,"action"),"menu callback replacement/self-removal failed");
        root.ticking=true;root.sync();int builds=root.builds;pump(60);require(root.builds>builds,"timeline did not trigger redraw");
        root.ticking=false;root.sync();builds=root.builds;pump(40);require(root.builds==builds,"removed timeline kept firing");
        require(CPPUI::Wx::detail::fileFilters("Images (*.png *.jpg);;All (*)")=="Images (*.png *.jpg)|*.png;*.jpg|All (*)|*","dialog filters lost patterns");
        for(bool accepted:{true,false}){Answer driver("",accepted);auto value=CPPUI::Prompt("Question","Title","default");driver.check();require(accepted?(value&&value->empty()):!value,"empty accepted prompt equals cancellation");}
        {Answer driver("hello 世界",true);auto value=CPPUI::Prompt();driver.check();require(value&&*value=="hello 世界","dialog UTF-8 lost");}
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}

#include "cppui/wx/application.hpp"
#include "cppui/wx/window.hpp"
#include "cppui/wx/layout.hpp"
#include "cppui/wx/splitter.hpp"
#include "cppui/wx/label.hpp"
#include "cppui/wx/textfield.hpp"
#include <wx/stopwatch.h>
#include <wx/weakref.h>

namespace {
    void require(bool ok,const char * message) {if(!ok) throw std::runtime_error(message);}
    void pump() {
        wxStopWatch clock;
        do {wxTheApp->Yield(true);wxTheApp->ProcessPendingEvents();wxTheApp->ProcessIdle();wxMilliSleep(1);} while(clock.Time()<25);
    }
    CPPUI::Node * find(CPPUI::Node & root,const std::string & id) {
        if(root._id==id) return &root;
        for(auto child:*root.children) if(auto result=find(*child,id)) return result;
        return nullptr;
    }
    wxWindow * native(CPPUI::Node & root,const std::string & id) {
        auto node=find(root,id);require(node && node->outer(),"missing native node");return static_cast<wxWindow *>(node->outer());
    }
    std::vector<wxSplitterWindow *> chain(wxWindow * host) {
        std::vector<wxSplitterWindow *> result;
        if(host->GetSizer()->GetItemCount()==0) return result;
        auto next=host->GetSizer()->GetItem(std::size_t(0))->GetWindow();
        while(auto splitter=dynamic_cast<wxSplitterWindow *>(next)) {result.push_back(splitter);next=splitter->GetWindow2();}
        return result;
    }
    class Root: public CPPUI::Application {
    public:
        using Application::Application;
        std::vector<std::string> keys{"a","b","c"};
        CPPUI::State<std::string> text{std::string("initial")};
        bool vertical=false,show=true,visible=true,nested=false,replace=false,extra=false;
        int padding=3;
        void content() override {
            for(auto win:CPPUI::Window().size(800,500).id("window")) if(show)
                for(auto split:CPPUI::Splitter(vertical).id("split").visible(visible).layout({}, {}, {}, CPPUI::Insets(0))) {
                    for(const auto & key:keys) {
                        if(key=="a") {
                            if(replace) CPPUI::Label("replaced").id("a");
                            else for(auto box:CPPUI::VBox().id("a").layout({}, {}, {}, CPPUI::Insets(padding))) {
                                CPPUI::TextField(text).id("editor").layout({}, {}, {}, CPPUI::Insets(0));
                                if(extra) CPPUI::Label("new descendant").id("extra");
                            }
                        } else if(key=="b" && nested) {
                            for(auto inner:CPPUI::Splitter(true).id(key)) {CPPUI::Label("one").id("inner-one");CPPUI::Label("two").id("inner-two");}
                        } else CPPUI::Label(key).id(key).layout({}, {}, {}, CPPUI::Insets(0)).click([this]() {show=false;sync();});
                    }
                }
        }
        void settle() {sync();pump();}
    };
    void geometry(int argc,char ** argv) {
        Root root(argc,argv);root.settle();
        auto host=native(root,"split"),editor=native(root,"editor"),b=native(root,"b"),c=native(root,"c");
        auto splits=chain(host);require(splits.size()==2,"three panes need two native splitters");
        auto pane=editor->GetParent();require(pane==splits[0]->GetWindow1() && b->GetParent()==splits[1]->GetWindow1()
            && c->GetParent()==splits[1]->GetWindow2(),"layout/widget panes were not reparented");
        require(editor->GetPosition()==wxPoint(3,3),"pane padding was not applied");
        const int aw=pane->GetSize().x,bw=b->GetParent()->GetSize().x,cw=c->GetParent()->GetSize().x;
        require(std::abs(aw-bw)<8 && std::abs(bw-cw)<8,"initial pane allocation is not equal");
        auto frame=native(root,"window");auto size=frame->GetClientSize();frame->SetClientSize(size.x+300,size.y);pump();
        require(std::abs(pane->GetSize().x-aw-100)<4 && std::abs(b->GetParent()->GetSize().x-bw-100)<4
            && std::abs(c->GetParent()->GetSize().x-cw-100)<4,"sash gravity did not share resize across panes");
        static_cast<wxTextCtrl *>(editor)->ChangeValue("draft");
        splits[0]->SetSashPosition(145);splits[1]->SetSashPosition(230);pump();
        root.padding=9;root.extra=true;root.settle();
        require(chain(host)==splits && splits[0]->GetSashPosition()==145 && splits[1]->GetSashPosition()==230,"ordinary redraw reset native sashes");
        require(editor->GetPosition()==wxPoint(9,9),"retained pane padding was stale");
        require(native(root,"extra")->GetParent()==pane,"new layout descendant was created outside its retained pane");
        root.visible=false;root.settle();require(!host->IsShown() && !editor->IsShown(),"hidden Splitter retained child visibility");
        root.visible=true;root.settle();require(host->IsShown() && editor->IsShown(),"Splitter did not restore child visibility");
        root.keys={"c","a","d"};root.settle();
        require(native(root,"split")==host && native(root,"editor")==editor && native(root,"c")==c,"keyed pane move replaced native controls");
        require(static_cast<wxTextCtrl *>(editor)->GetValue()=="draft","pane reorder discarded editor state");
        splits=chain(host);require(splits[0]->GetWindow1()==c->GetParent() && splits[1]->GetWindow1()==pane,"pane order did not follow declarations");
        root.vertical=true;root.settle();splits=chain(host);
        require(splits[0]->GetSplitMode()==wxSPLIT_HORIZONTAL && splits[1]->GetSplitMode()==wxSPLIT_HORIZONTAL
            && c->GetScreenPosition().y<editor->GetScreenPosition().y,"vertical panes were not stacked");
        root.keys={"a"};root.settle();require(chain(host).empty() && pane->GetSize()==host->GetClientSize(),"single pane did not fill host");
        root.keys.clear();root.settle();require(host->GetSizer()->GetItemCount()==0 && host->GetChildren().empty(),"empty Splitter retained native wrappers");
        root.keys={"a","b","c","d"};root.vertical=false;root.nested=true;root.settle();
        require(chain(host).size()==3 && chain(native(root,"b")).size()==1,"nested/many-pane splitter was not mounted");
        wxWeakRef<wxWindow> oldEditor(native(root,"editor"));root.replace=true;root.settle();
        require(!oldEditor && native(root,"a")->GetParent()==chain(host)[0]->GetWindow1(),"pane type replacement retained old editor or wrong parent");
        auto declaration=find(root,"split");host->Destroy();pump();require(!declaration->outer(),"external host deletion left stale native handles");
        root.show=false;root.settle();root.show=true;root.settle();
        wxWeakRef<wxWindow> removed(native(root,"split"));
        wxMouseEvent click(wxEVT_LEFT_DOWN);native(root,"c")->ProcessWindowEvent(click);pump();
        require(!removed && !find(root,"split"),"native click removal retained Splitter or its controls");
    }
    class Panes: public CPPUI::Wx::View {
        CPPUI::State<int> & count;
        CPPUI::State<int> & padding;
    public:
        Panes(CPPUI::State<int> & count,CPPUI::State<int> & padding):count(count),padding(padding) {}
        void content() override {for(int i=0;i<count;++i) CPPUI::Label("pane").id("pane"+std::to_string(i)).layout({}, {}, {}, CPPUI::Insets(padding));}
    };
    class VirtualRoot: public CPPUI::Application {
    public:
        using Application::Application;
        CPPUI::State<int> count{2},padding{3};
        void content() override {
            for(auto win:CPPUI::Window().size(600,300)) for(auto split:CPPUI::Splitter().id("split"))
                (new Panes(count,padding))->id("panes");
        }
    };
    void independent(int argc,char ** argv) {
        VirtualRoot root(argc,argv);root.sync();pump();auto host=native(root,"split"),pane=native(root,"pane0");
        for(int count:{4,1,3,0,2}) {
            root.count=count;pump();
            require(chain(host).size()==std::size_t(std::max(0,count-1)),"independent view did not reconcile panes");
            if(count==4) require(native(root,"pane0")==pane,"independent insertion replaced native pane");
        }
        root.padding=11;pump();
        require(native(root,"pane0")->GetPosition()==wxPoint(11,11),"independent view retained old declaration padding");
    }
}
int main(int argc,char ** argv) {
    wxSetAssertHandler([](const wxString &,int,const wxString &,const wxString & condition,const wxString & message) {std::cerr<<condition<<": "<<message<<'\n';std::abort();});
    try {geometry(argc,argv);independent(argc,argv);}
    catch(const std::exception & error) {std::cerr<<error.what()<<'\n';return 1;}
}

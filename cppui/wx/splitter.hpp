#ifndef CPPUI_WX_SPLITTER
#define CPPUI_WX_SPLITTER
#include "base.hpp"
#include <wx/splitter.h>
namespace CPPUI { namespace Wx {
class Splitter: public Widget {
    struct Pane {
        wxPanel * panel=nullptr;
        Mount mount;
        void * key=nullptr;
    };
    bool vertical,previousVertical=false;
    std::vector<Pane> panes,detached;
    std::vector<wxPanel *> signature;
    std::vector<wxSplitterWindow *> splitters;
    static void reparent(wxSizer * sizer,wxWindow * parent) {
        for(auto item:sizer->GetChildren()) {
            if(item->IsWindow()) item->GetWindow()->Reparent(parent);
            else if(item->IsSizer()) reparent(item->GetSizer(),parent);
        }
    }
    static void reparent(Node * child,wxWindow * parent) {
        if(child->window()) child->window()->Reparent(parent);
        else if(child->sizer()) reparent(child->sizer(),parent);
    }
    void clearHandles() {panes.clear();detached.clear();signature.clear();splitters.clear();}
    void showPanes(bool visible) {
        for(auto & pane:panes) showMount(pane.mount,visible && layoutVisible());
    }
public:
    explicit Splitter(bool vertical=false):vertical(vertical) {is_terminal=false;expandX=expandY=3;}
    void updateWidget(Widget * previous) override {
        auto prev=static_cast<Splitter *>(previous);
        ui=prev ? prev->ui : new wxPanel(parentWindow());
        if(prev) {
            panes=std::move(prev->panes);detached=std::move(prev->detached);
            signature=std::move(prev->signature);splitters=std::move(prev->splitters);
            previousVertical=prev->previousVertical;
        } else ui->SetSizer(new wxBoxSizer(wxVERTICAL));
    }
    void rebindChild(::CPPUI::Node * previous,::CPPUI::Node * current) override {
        for(auto & pane:panes) if(pane.mount.node==previous) pane.mount.node=static_cast<Node *>(current);
    }
    void addChild(int index,::CPPUI::Node * value) override {
        if(value->is_outoforder) return;
        auto child=dynamic_cast<Node *>(value);
        if(!child) throw std::logic_error("wx Splitter requires wx children");
        Pane pane;pane.key=child->outer();
        for(auto it=detached.begin();it!=detached.end();++it) if(pane.key && it->key==pane.key) {
            pane.panel=it->panel;detached.erase(it);break;
        }
        if(!pane.panel) pane.panel=new wxPanel(ui);
        reparent(child,pane.panel);
        auto wrapper=new wxBoxSizer(wxVERTICAL);
        if(dynamic_cast<Spacer *>(child)) {pane.mount.node=child;pane.mount.item=wrapper->Add(0,0);}
        else pane.mount=padded(child,wrapper);
        pane.panel->SetSizer(wrapper);
        panes.insert(panes.begin()+index,pane);
    }
    void removeChild(int index,::CPPUI::Node * child) override {
        if(child->is_outoforder || !ui) return;
        auto pane=panes.at(index);
        detachContent(pane.mount);
        pane.panel->SetSizer(nullptr);
        reparent(pane.mount.node,ui);
        pane.mount=Mount();pane.panel->Hide();
        detached.push_back(pane);panes.erase(panes.begin()+index);
    }
    void applyVisibility(bool visible) override {
        Widget::applyVisibility(visible);showPanes(visible);
    }
    void postSync() override {
        if(!ui) return;
        std::vector<wxPanel *> current;
        for(auto & pane:panes) {
            applyPadding(pane.mount);pane.panel->Layout();current.push_back(pane.panel);
        }
        if(current!=signature || vertical!=previousVertical) {
            ui->GetSizer()->Clear(false);
            for(auto splitter:splitters) if(splitter->IsSplit()) splitter->Unsplit();
            // Detach all retained panes before native splitter destruction.
            for(auto & pane:panes) pane.panel->Reparent(ui);
            for(auto & pane:detached) pane.panel->Reparent(ui);
            for(auto splitter:splitters) splitter->Reparent(ui);
            for(auto splitter:splitters) splitter->Destroy();
            splitters.clear();
            for(auto & pane:detached) pane.panel->Destroy();
            detached.clear();
            if(!panes.empty()) {
                wxWindow * tail=panes.back().panel;tail->Show();
                for(int i=int(panes.size())-2;i>=0;--i) {
                    auto splitter=new wxSplitterWindow(ui,wxID_ANY,wxDefaultPosition,wxDefaultSize,wxSP_LIVE_UPDATE);
                    splitter->SetMinimumPaneSize(1);
                    splitter->SetSashGravity(1.0/(panes.size()-i));
                    auto first=panes[i].panel;first->Reparent(splitter);tail->Reparent(splitter);
                    if(vertical) splitter->SplitHorizontally(first,tail);
                    else splitter->SplitVertically(first,tail);
                    splitters.insert(splitters.begin(),splitter);tail=splitter;
                }
                ui->GetSizer()->Add(tail,1,wxEXPAND);ui->Layout();
                for(std::size_t i=0;i<splitters.size();++i) {
                    const auto size=splitters[i]->GetClientSize();
                    const int remaining=int(panes.size()-i);
                    const int usable=(vertical ? size.y : size.x)-splitters[i]->GetSashSize()*(remaining-1);
                    splitters[i]->SetSashPosition(std::max(1,usable/remaining));splitters[i]->UpdateSize();
                }
            }
            signature=std::move(current);previousVertical=vertical;
        }
        showPanes(ancestorsVisible());
        ui->InvalidateBestSize();ui->Layout();
        if(ui->GetParent()) ui->GetParent()->Layout();
    }
    void postUpdate() override {postSync(); ::CPPUI::Node::postUpdate();}
    void invalidateNative() override {clearHandles();Widget::invalidateNative();}
    void releaseRetiredRefs() override {clearHandles();Widget::releaseRetiredRefs();}
    void destroy(bool direct) override {clearHandles();Widget::destroy(direct);}
};
}
inline Wx::Splitter & Splitter(bool vertical=false) {return *new Wx::Splitter(vertical);}
}
#endif

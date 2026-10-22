#ifndef CPPUI_WX_STACK
#define CPPUI_WX_STACK
#include "base.hpp"
#include "painted_controls.hpp"
namespace CPPUI { namespace Wx {
class StackSizer: public wxSizer {
public:
    wxSize CalcMin() override {
        wxSize result(0,0);
        for(auto item:GetChildren()) if(item->IsShown()) result.IncTo(item->CalcMin());
        return result;
    }
    void RepositionChildren(const wxSize &) override {
        for(auto item:GetChildren()) if(item->IsShown()) item->SetDimension(GetPosition(),GetSize());
    }
};
class Stack: public Widget {
    static void raise(wxSizer * sizer) {
        for(auto item:sizer->GetChildren()) {
            if(item->IsWindow()) {
                auto window=item->GetWindow();
#ifdef __WXMSW__
                window->SetWindowStyleFlag(window->GetWindowStyleFlag() | wxCLIP_SIBLINGS);
#endif
                window->Raise();window->Refresh();
            }
            else if(item->IsSizer()) raise(item->GetSizer());
        }
    }
public:
    std::vector<Mount> mounts;
    Stack() { is_terminal=false; expandX=expandY=3; }
    void updateWidget(Widget * previous) override {
        auto prev=static_cast<Stack *>(previous);
        ui=prev ? prev->ui : new Painting::StackPanel(parentWindow());
        if(prev) mounts=std::move(prev->mounts);
        else ui->SetSizer(new StackSizer());
    }
    void rebindChild(::CPPUI::Node * previous,::CPPUI::Node * current) override {
        for(auto & mount:mounts) if(mount.node==previous) mount.node=static_cast<Node *>(current);
    }
    void addChild(int index,::CPPUI::Node * value) override {
        if(value->is_outoforder) return;
        auto child=dynamic_cast<Node *>(value);
        if(!child) throw std::logic_error("wx Stack requires wx children");
        Mount mount; mount.node=child;
        if(dynamic_cast<Spacer *>(child)) mount.item=ui->GetSizer()->Insert(index,0,0);
        else {
            auto wrapper=new wxBoxSizer(wxVERTICAL);
            mount=padded(child,wrapper);
            mount.item=ui->GetSizer()->Insert(index,wrapper,0,wxEXPAND);
        }
        mounts.insert(mounts.begin()+index,mount);
    }
    void removeChild(int index,::CPPUI::Node * child) override {
        if(child->is_outoforder || !ui) return;
        detachContent(mounts.at(index));
        ui->GetSizer()->Remove(index);
        mounts.erase(mounts.begin()+index);
    }
    void applyVisibility(bool visible) override {
        Widget::applyVisibility(visible);
        for(auto & mount:mounts) showMount(mount,visible && layoutVisible());
    }
    void postSync() override {
        if(!ui) return;
        for(auto & mount:mounts) { applyPadding(mount); showMount(mount,ancestorsVisible() && layoutVisible()); }
        ui->InvalidateBestSize(); ui->Layout(); raise(ui->GetSizer());
        if(ui->GetParent()) ui->GetParent()->Layout();
    }
    void postUpdate() override { postSync(); ::CPPUI::Node::postUpdate(); }
    void invalidateNative() override { mounts.clear(); Widget::invalidateNative(); }
    void releaseRetiredRefs() override { mounts.clear(); Widget::releaseRetiredRefs(); }
    void destroy(bool direct) override { mounts.clear(); Widget::destroy(direct); }
};
}
inline Wx::Stack & Stack() { return *new Wx::Stack(); }
}
#endif

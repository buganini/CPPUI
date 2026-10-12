#ifndef CPPUI_WX_STACK
#define CPPUI_WX_STACK
#include "base.hpp"
namespace CPPUI { namespace Wx {
class StackSizer:public wxSizer {
public:
    wxSize CalcMin() override {wxSize result(0,0);for(auto item:GetChildren())if(item->IsShown())result.IncTo(item->CalcMin());return result;}
    void RepositionChildren(const wxSize &) override {for(auto item:GetChildren())if(item->IsShown())item->SetDimension(GetPosition(),GetSize());}
};
class Stack:public Layout {
    static void raise(wxSizer * sizer){for(auto item:sizer->GetChildren()){if(item->IsWindow())item->GetWindow()->Raise();else if(item->IsSizer())raise(item->GetSizer());}}
public:
    Stack(){expandX=expandY=2;}
    wxSizer * createSizer() override{return new StackSizer();}
    void applyChildren() override {for(auto & mount:mounts){applyPadding(mount);mount.item->SetFlag(wxEXPAND);if(mount.item->IsSizer())raise(mount.item->GetSizer());}}
};
}
inline Wx::Stack & Stack(){return *new Wx::Stack();}
}
#endif

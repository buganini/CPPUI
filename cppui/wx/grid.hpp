#ifndef CPPUI_WX_GRID
#define CPPUI_WX_GRID
#include "base.hpp"
#include "../grid.hpp"
#include <wx/gbsizer.h>
namespace CPPUI { namespace Wx {
class Grid: public ::CPPUI::detail::GridLayout<Layout> {
    int growable=0;
    void detach() {
        if(!wxSizerObject) return;
        for(auto & mount:mounts) detachContent(mount);
        wxSizerObject->Clear(false);mounts.clear();
    }
public:
    wxSizer * createSizer() override { return new wxGridBagSizer(); }
    void update(::CPPUI::Node * previous) override {
        auto prev=static_cast<Grid *>(previous);
        if(prev && prev->wxSizerObject) {retainGrid(prev);growable=prev->growable;}
        Layout::update(previous);
    }
    void invalidateNative() override { Layout::invalidateNative();gridChildren.clear();growable=0; }
    void preSync() override { detach(); }
    void addChild(int index,::CPPUI::Node * child) override { mountGrid(index,child); }
    void removeChild(int index,::CPPUI::Node * child) override {
        if(child->is_outoforder) return;
        detach();unmountGrid(index,child);
    }
    void postSync() override {
        if(!wxSizerObject) return;
        auto placed=cells();
        detach();
        auto nativeGrid=static_cast<wxGridBagSizer *>(wxSizerObject);
        for(int column=0;column<growable;++column) nativeGrid->RemoveGrowableCol(column);
        growable=grid_columns;
        for(auto child:gridChildren) if(!child->is_visible)
            static_cast<Node *>(child)->applyVisibility(false);
        for(const auto & item:placed) {
            auto child=dynamic_cast<Node *>(item.node);
            if(!child || dynamic_cast<Spacer *>(child)) continue;
            auto wrapper=new wxBoxSizer(wxVERTICAL);
            auto mount=padded(child,wrapper);
            const auto & cell=item.cell;
            mount.item=nativeGrid->Add(wrapper,wxGBPosition(cell.row,cell.column),
                                      wxGBSpan(cell.rowspan,cell.columnspan),wxEXPAND);
            mounts.push_back(mount);
        }
        for(int column=0;column<growable;++column) nativeGrid->AddGrowableCol(column);
        Layout::postSync();
    }
    void applyChildren() override { for(auto & mount: mounts) { applyPadding(mount); showMount(mount,ancestorsVisible() && layoutVisible()); mount.item->SetFlag(wxEXPAND); } }
};
}
inline Wx::Grid & Grid() { return *new Wx::Grid(); }
}
#endif

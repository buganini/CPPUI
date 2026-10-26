#ifndef CPPUI_WX_GRID
#define CPPUI_WX_GRID
#include "base.hpp"
#include "../grid.hpp"
#include <wx/gbsizer.h>
namespace CPPUI { namespace Wx {
class Grid: public ::CPPUI::detail::GridLayout<Layout> {
    std::vector<int> growable,growableRows;
    void detach() {
        if(!wxSizerObject) return;
        for(auto & mount:mounts) detachContent(mount);
        wxSizerObject->Clear(false);mounts.clear();
    }
public:
    Grid() { expandX=expandY=1; }
    wxSizer * createSizer() override { return new wxGridBagSizer(); }
    void update(::CPPUI::Node * previous) override {
        auto prev=static_cast<Grid *>(previous);
        if(prev && prev->wxSizerObject) {retainGrid(prev);growable=prev->growable;growableRows=prev->growableRows;}
        Layout::update(previous);
    }
    void invalidateNative() override { Layout::invalidateNative();gridChildren.clear();growable.clear();growableRows.clear(); }
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
        for(int column:growable) nativeGrid->RemoveGrowableCol(column);
        for(int row:growableRows) nativeGrid->RemoveGrowableRow(row);
        growable.clear();growableRows.clear();
        auto columns=::CPPUI::detail::nativeStretches(::CPPUI::detail::trackStretches(placed,true,this));
        auto rows=::CPPUI::detail::nativeStretches(::CPPUI::detail::trackStretches(placed,false,this));
        int occupiedColumns=0;
        for(auto child:gridChildren) if(!child->is_visible)
            static_cast<Node *>(child)->applyVisibility(false);
        for(const auto & item:placed) {
            auto child=dynamic_cast<Node *>(item.node);
            if(!child) continue;
            auto wrapper=new wxBoxSizer(wxVERTICAL);
            auto mount=padded(child,wrapper);
            const auto & cell=item.cell;
            occupiedColumns=std::max(occupiedColumns,cell.column+cell.columnspan);
            mount.item=nativeGrid->Add(wrapper,wxGBPosition(cell.row,cell.column),
                                      wxGBSpan(cell.rowspan,cell.columnspan),wxEXPAND);
            mounts.push_back(mount);
        }
        // A sparse automatic grid still has all declared columns. GridBagSizer
        // otherwise discovers only occupied tracks, and rejects growable ones
        // beyond its last calculated column (including after a span change).
        if(grid_columns>occupiedColumns)
            nativeGrid->Add(0,0,wxGBPosition(0,grid_columns-1));
        nativeGrid->CalcMin();
        for(int column=0;column<int(columns.size());++column) if(columns[column]) {
            nativeGrid->AddGrowableCol(column,columns[column]);growable.push_back(column);
        }
        for(int row=0;row<int(rows.size());++row) if(rows[row]) {
            nativeGrid->AddGrowableRow(row,rows[row]);growableRows.push_back(row);
        }
        Layout::postSync();
    }
    void applyChildren() override { for(auto & mount: mounts) { applyPadding(mount); showMount(mount,ancestorsVisible() && layoutVisible()); mount.item->SetFlag(wxEXPAND); } }
};
}
inline Wx::Grid & Grid() { return *new Wx::Grid(); }
}
#endif

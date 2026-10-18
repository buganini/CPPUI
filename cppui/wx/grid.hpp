#ifndef CPPUI_WX_GRID
#define CPPUI_WX_GRID
#include "base.hpp"
#include <wx/gbsizer.h>
namespace CPPUI { namespace Wx {
class Grid: public Layout {
public:
    Grid() { is_grid_layout = true; }
    wxSizer * createSizer() override { return new wxGridBagSizer(); }
    wxSizerItem * insert(int, wxSizer * wrapper, Node * child) override {
        auto grid = static_cast<wxGridBagSizer *>(wxSizerObject);
        return grid->Add(wrapper, wxGBPosition(child->grid_row, child->grid_column),
            wxGBSpan(std::max(1, child->grid_rowspan), std::max(1, child->grid_columnspan)), wxEXPAND);
    }
    void applyChildren() override { for(auto & mount: mounts) { applyPadding(mount); showMount(mount,ancestorsVisible() && layoutVisible()); mount.item->SetFlag(wxEXPAND); } }
};
}
inline Wx::Grid & Grid() { return *new Wx::Grid(); }
}
#endif

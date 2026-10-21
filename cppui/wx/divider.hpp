#ifndef CPPUI_WX_DIVIDER
#define CPPUI_WX_DIVIDER
#include "base.hpp"
#include <wx/statline.h>
namespace CPPUI { namespace Wx {
class Divider: public Widget {
public:
    Divider() { is_movable = false; }
    void updateWidget(Widget * prev) override {
        auto parent = physicalParent();
        bool horizontalLine = !parent || !parent->horizontal;
        expandX = horizontalLine ? 1 : 0; expandY = horizontalLine ? 0 : 1;
        const long style=horizontalLine ? wxLI_HORIZONTAL : wxLI_VERTICAL;
        ui = prev ? prev->ui : new wxStaticLine(parentWindow(), wxID_ANY, wxDefaultPosition, wxDefaultSize, style);
        auto line=static_cast<wxStaticLine *>(ui);
        if(line->IsVertical()==horizontalLine) {
            line->SetWindowStyleFlag((line->GetWindowStyleFlag() & ~(wxLI_HORIZONTAL | wxLI_VERTICAL)) | style);
            line->InvalidateBestSize();
        }
    }
};
}
inline Wx::Divider & Divider() { return *new Wx::Divider(); }
}
#endif

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
        bool horizontalLine = parent && parent->vertical;
        expandX = horizontalLine ? 1 : 0; expandY = horizontalLine ? 0 : 1;
        ui = prev ? prev->ui : new wxStaticLine(parentWindow(), wxID_ANY, wxDefaultPosition, wxSize(15,15), horizontalLine ? wxLI_HORIZONTAL : wxLI_VERTICAL);
    }
};
}
inline Wx::Divider & Divider() { return *new Wx::Divider(); }
}
#endif

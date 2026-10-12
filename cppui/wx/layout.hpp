#ifndef CPPUI_WX_LAYOUT
#define CPPUI_WX_LAYOUT
#include "base.hpp"
namespace CPPUI { namespace Wx {
    class HBox: public Layout {
        public:
        HBox() { horizontal = true; expandX = 2; expandY = 1; }
        wxSizer * createSizer() override { return new wxBoxSizer(wxHORIZONTAL); }
    };
    class VBox: public Layout {
        public:
        VBox() { vertical = true; expandX = 1; expandY = 2; }
        wxSizer * createSizer() override { return new wxBoxSizer(wxVERTICAL); }
    };
}
inline Wx::HBox & HBox() { return *new Wx::HBox(); }
inline Wx::VBox & VBox() { return *new Wx::VBox(); }
inline Wx::Spacer & Spacer() { return *new Wx::Spacer(); }
}
#endif

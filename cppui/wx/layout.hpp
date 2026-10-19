#ifndef CPPUI_WX_LAYOUT
#define CPPUI_WX_LAYOUT
#include "base.hpp"
#include "grid.hpp"
#include "stack.hpp"
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
    class AdaptiveBox: public HBox {
        public:
        AdaptiveBox(int threshold,bool width) {
            horizontal=::CPPUI::detail::adaptiveHorizontal(*this,threshold,width);vertical=!horizontal;
            expandX=horizontal ? 2 : 1;expandY=horizontal ? 1 : 2;
        }
        void update(::CPPUI::Node * prev) override {
            Layout::update(prev);
            static_cast<wxBoxSizer *>(wxSizerObject)->SetOrientation(horizontal ? wxHORIZONTAL : wxVERTICAL);
        }
    };
    class HVBox: public AdaptiveBox { public: explicit HVBox(int minWidth):AdaptiveBox(minWidth,true) {} };
    class VHBox: public AdaptiveBox { public: explicit VHBox(int minHeight):AdaptiveBox(minHeight,false) {} };
}
inline Wx::HBox & HBox() { return *new Wx::HBox(); }
inline Wx::VBox & VBox() { return *new Wx::VBox(); }
inline Wx::HVBox & HVBox(int minWidth) { return *new Wx::HVBox(minWidth); }
inline Wx::VHBox & VHBox(int minHeight) { return *new Wx::VHBox(minHeight); }
inline Wx::Spacer & Spacer() { return *new Wx::Spacer(); }
}
#endif

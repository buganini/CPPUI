#ifndef CPPUI_WX_LAYOUT
#define CPPUI_WX_LAYOUT
#include "base.hpp"
#include "grid.hpp"
#include "stack.hpp"
namespace CPPUI { namespace Wx {
    class Box: public Layout {
        protected:
        bool inverse, mountedInverse=false;
        public:
        explicit Box(bool inverse):inverse(inverse) {}
        int nativeIndex(int index,bool inserting=false) const override {
            return mountedInverse ? static_cast<int>(mounts.size())-index-(inserting ? 0 : 1) : index;
        }
        void update(::CPPUI::Node * previous) override {
            auto prev=static_cast<Box *>(previous);
            mountedInverse=prev && prev->wxSizerObject && prev->mountedInverse;
            Layout::update(previous);
            if(mountedInverse!=inverse) {
                // Reverse native items in place: wrappers, focus and drafts stay alive.
                wxSizerObject->GetChildren().reverse();
                mountedInverse=inverse;
            }
        }
    };
    class HBox: public Box {
        public:
        explicit HBox(bool inverse=false):Box(inverse) { horizontal = true; expandX = 3; expandY = 1; }
        wxSizer * createSizer() override { return new wxBoxSizer(wxHORIZONTAL); }
    };
    class VBox: public Box {
        public:
        explicit VBox(bool inverse=false):Box(inverse) { vertical = true; expandX = 1; expandY = 3; }
        wxSizer * createSizer() override { return new wxBoxSizer(wxVERTICAL); }
    };
    class AdaptiveBox: public HBox {
        public:
        AdaptiveBox(int threshold,bool width,bool inverseH,bool inverseV) {
            horizontal=::CPPUI::detail::adaptiveHorizontal(*this,threshold,width);vertical=!horizontal;
            inverse=horizontal ? inverseH : inverseV;
            expandX=horizontal ? 3 : 1;expandY=horizontal ? 1 : 3;
        }
        void update(::CPPUI::Node * prev) override {
            Box::update(prev);
            static_cast<wxBoxSizer *>(wxSizerObject)->SetOrientation(horizontal ? wxHORIZONTAL : wxVERTICAL);
        }
    };
    class HVBox: public AdaptiveBox { public: explicit HVBox(int minWidth,bool inverseH=false,bool inverseV=false):AdaptiveBox(minWidth,true,inverseH,inverseV) {} };
    class VHBox: public AdaptiveBox { public: explicit VHBox(int minHeight,bool inverseH=false,bool inverseV=false):AdaptiveBox(minHeight,false,inverseH,inverseV) {} };
}
inline Wx::HBox & HBox(bool inverse=false) { return *new Wx::HBox(inverse); }
inline Wx::VBox & VBox(bool inverse=false) { return *new Wx::VBox(inverse); }
inline Wx::HVBox & HVBox(int minWidth,bool inverseH=false,bool inverseV=false) { return *new Wx::HVBox(minWidth,inverseH,inverseV); }
inline Wx::VHBox & VHBox(int minHeight,bool inverseH=false,bool inverseV=false) { return *new Wx::VHBox(minHeight,inverseH,inverseV); }
inline Wx::Spacer & Spacer() { return *new Wx::Spacer(); }
}
#endif

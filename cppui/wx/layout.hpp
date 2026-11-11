#ifndef CPPUI_WX_LAYOUT
#define CPPUI_WX_LAYOUT
#include "base.hpp"
#include "../gap.hpp"
#include "grid.hpp"
#include "stack.hpp"
namespace CPPUI { namespace Wx {
    class Box: public Layout {
        protected:
        bool inverse, mountedInverse=false;
        public:
        int spacing;
        explicit Box(bool inverse,double gap):inverse(inverse),spacing(::CPPUI::detail::gapPixels(gap)) {}
        int nativeIndex(int index,bool inserting=false) const override {
            return mountedInverse ? static_cast<int>(mounts.size())-index-(inserting ? 0 : 1) : index;
        }
        void applyChildren() override {
            Layout::applyChildren();
            bool seen=false;
            for(std::size_t i=0;i<mounts.size();++i) {
                auto & mount=mounts[inverse ? mounts.size()-1-i : i];
                const bool visible=mount.node->layoutVisible();
                const int gap=visible && seen ? spacing : 0;
                if(mount.content) {
                    auto padding=mount.node->layout_padding.valueOr(Insets(0));
                    if(horizontal) mount.padding[3]->SetMinSize(padding.left+gap,0);
                    else mount.padding[0]->SetMinSize(0,padding.top+gap);
                } else {
                    mount.item->SetBorder(gap);
                    mount.item->SetFlag(mount.item->GetFlag() | (horizontal ? wxLEFT : wxTOP));
                }
                seen=seen || visible;
            }
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
        explicit HBox(bool inverse=false,double gap=10):Box(inverse,gap) { horizontal = true; expandX = 3; expandY = 1; }
        wxSizer * createSizer() override { return new wxBoxSizer(wxHORIZONTAL); }
    };
    class VBox: public Box {
        public:
        explicit VBox(bool inverse=false,double gap=10):Box(inverse,gap) { vertical = true; expandX = 1; expandY = 3; }
        wxSizer * createSizer() override { return new wxBoxSizer(wxVERTICAL); }
    };
    class AdaptiveBox: public HBox {
        public:
        AdaptiveBox(int threshold,bool width,bool inverseH,bool inverseV,double gap):HBox(false,gap) {
            horizontal=::CPPUI::detail::adaptiveHorizontal(*this,threshold,width);vertical=!horizontal;
            inverse=horizontal ? inverseH : inverseV;
            expandX=horizontal ? 3 : 1;expandY=horizontal ? 1 : 3;
        }
        void update(::CPPUI::Node * prev) override {
            Box::update(prev);
            static_cast<wxBoxSizer *>(wxSizerObject)->SetOrientation(horizontal ? wxHORIZONTAL : wxVERTICAL);
        }
    };
    class HVBox: public AdaptiveBox { public: explicit HVBox(int minWidth,bool inverseH=false,bool inverseV=false,double gap=10):AdaptiveBox(minWidth,true,inverseH,inverseV,gap) {} };
    class VHBox: public AdaptiveBox { public: explicit VHBox(int minHeight,bool inverseH=false,bool inverseV=false,double gap=10):AdaptiveBox(minHeight,false,inverseH,inverseV,gap) {} };
}
inline Wx::HBox & HBox(bool inverse=false,double gap=10) { return *new Wx::HBox(inverse,gap); }
inline Wx::VBox & VBox(bool inverse=false,double gap=10) { return *new Wx::VBox(inverse,gap); }
inline Wx::HVBox & HVBox(int minWidth,bool inverseH=false,bool inverseV=false,double gap=10) { return *new Wx::HVBox(minWidth,inverseH,inverseV,gap); }
inline Wx::VHBox & VHBox(int minHeight,bool inverseH=false,bool inverseV=false,double gap=10) { return *new Wx::VHBox(minHeight,inverseH,inverseV,gap); }
inline Wx::Spacer & Spacer() { return *new Wx::Spacer(); }
}
#endif

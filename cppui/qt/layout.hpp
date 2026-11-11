#ifndef CPPUI_QT_LAYOUT
#define CPPUI_QT_LAYOUT

#include "base.hpp"
#include "../gap.hpp"
#include "spacer.hpp"
#include "grid.hpp"
#include "stack.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>

namespace CPPUI {
    namespace Qt {
        class HBox: public BaseLayout {
            protected:
            bool horizontal=true, inverse=false;
            public:
            bool expansionAxis(bool axis) const override { return axis==horizontal; }
            explicit HBox(bool inverse=false,double gap=10):inverse(inverse) { spacing=::CPPUI::detail::gapPixels(gap); expandX=3;expandY=1; }
            void update(BaseLayout * prev) {
                if(prev && prev->ui) {
                    ui = prev->ui;
                    qtlayout = prev->qtlayout;
                } else {
                    ui = new Widget<QWidget>();
                    qtlayout = new QHBoxLayout();
                    ((QWidget *)ui)->setLayout(qtlayout);
                }
                qtlayout->setDirection(horizontal
                    ? (inverse ? QBoxLayout::RightToLeft : QBoxLayout::LeftToRight)
                    : (inverse ? QBoxLayout::BottomToTop : QBoxLayout::TopToBottom));
            }
        };

        class VBox: public BaseLayout {
            bool inverse;
            public:
            bool expansionAxis(bool axis) const override { return !axis; }
            explicit VBox(bool inverse=false,double gap=10):inverse(inverse) { spacing=::CPPUI::detail::gapPixels(gap); expandX=1;expandY=3; }
            void update(BaseLayout * prev) override {
                if(prev && prev->ui) {
                    ui=prev->ui;qtlayout=prev->qtlayout;
                } else {
                    ui=new Widget<QWidget>();
                    qtlayout=new QVBoxLayout(ui);
                }
                qtlayout->setDirection(inverse ? QBoxLayout::BottomToTop : QBoxLayout::TopToBottom);
            }
        };

        class AdaptiveBox: public HBox {
            public:
            AdaptiveBox(int threshold,bool width,bool inverseH,bool inverseV,double gap):HBox(false,gap) {
                horizontal=::CPPUI::detail::adaptiveHorizontal(*this,threshold,width);
                inverse=horizontal ? inverseH : inverseV;
                expandX=horizontal ? 3 : 1;expandY=horizontal ? 1 : 3;
            }
        };
        class HVBox: public AdaptiveBox { public: explicit HVBox(int minWidth,bool inverseH=false,bool inverseV=false,double gap=10):AdaptiveBox(minWidth,true,inverseH,inverseV,gap) {} };
        class VHBox: public AdaptiveBox { public: explicit VHBox(int minHeight,bool inverseH=false,bool inverseV=false,double gap=10):AdaptiveBox(minHeight,false,inverseH,inverseV,gap) {} };
    }
    inline Qt::HBox & HBox(bool inverse=false,double gap=10) {
        return *(new Qt::HBox(inverse,gap));
    }
    inline Qt::VBox & VBox(bool inverse=false,double gap=10) {
        return *(new Qt::VBox(inverse,gap));
    }
    inline Qt::HVBox & HVBox(int minWidth,bool inverseH=false,bool inverseV=false,double gap=10) { return *new Qt::HVBox(minWidth,inverseH,inverseV,gap); }
    inline Qt::VHBox & VHBox(int minHeight,bool inverseH=false,bool inverseV=false,double gap=10) { return *new Qt::VHBox(minHeight,inverseH,inverseV,gap); }
}

#endif

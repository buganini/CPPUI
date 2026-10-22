#ifndef CPPUI_QT_LAYOUT
#define CPPUI_QT_LAYOUT

#include "base.hpp"
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
            explicit HBox(bool inverse=false):inverse(inverse) { expandX=3;expandY=1; }
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
            explicit VBox(bool inverse=false):inverse(inverse) { expandX=1;expandY=3; }
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
            AdaptiveBox(int threshold,bool width,bool inverseH,bool inverseV) {
                horizontal=::CPPUI::detail::adaptiveHorizontal(*this,threshold,width);
                inverse=horizontal ? inverseH : inverseV;
                expandX=horizontal ? 3 : 1;expandY=horizontal ? 1 : 3;
            }
        };
        class HVBox: public AdaptiveBox { public: explicit HVBox(int minWidth,bool inverseH=false,bool inverseV=false):AdaptiveBox(minWidth,true,inverseH,inverseV) {} };
        class VHBox: public AdaptiveBox { public: explicit VHBox(int minHeight,bool inverseH=false,bool inverseV=false):AdaptiveBox(minHeight,false,inverseH,inverseV) {} };
    }
    inline Qt::HBox & HBox(bool inverse=false) {
        return *(new Qt::HBox(inverse));
    }
    inline Qt::VBox & VBox(bool inverse=false) {
        return *(new Qt::VBox(inverse));
    }
    inline Qt::HVBox & HVBox(int minWidth,bool inverseH=false,bool inverseV=false) { return *new Qt::HVBox(minWidth,inverseH,inverseV); }
    inline Qt::VHBox & VHBox(int minHeight,bool inverseH=false,bool inverseV=false) { return *new Qt::VHBox(minHeight,inverseH,inverseV); }
}

#endif

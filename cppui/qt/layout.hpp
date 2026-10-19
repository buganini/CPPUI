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
            public:
            void update(BaseLayout * prev) {
                if(prev && prev->ui) {
                    ui = prev->ui;
                    qtlayout = prev->qtlayout;
                } else {
                    ui = new Widget<QWidget>();
                    qtlayout = new QHBoxLayout();
                    ((QWidget *)ui)->setLayout(qtlayout);
                }
            }
        };

        class VBox: public BaseLayout {
            public:
            void update(BaseLayout * prev) {
                if(prev && prev->ui) {
                    ui = prev->ui;
                    qtlayout = prev->qtlayout;
                } else {
                    ui = new Widget<QWidget>();
                    qtlayout = new QVBoxLayout();
                    ((QWidget *)ui)->setLayout(qtlayout);
                }
            }
        };

        class AdaptiveBox: public HBox {
            bool horizontal;
            public:
            AdaptiveBox(int threshold,bool width):horizontal(::CPPUI::detail::adaptiveHorizontal(*this,threshold,width)) {}
            void update(BaseLayout * prev) override {
                HBox::update(prev);
                qtlayout->setDirection(horizontal ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom);
            }
        };
        class HVBox: public AdaptiveBox { public: explicit HVBox(int minWidth):AdaptiveBox(minWidth,true) {} };
        class VHBox: public AdaptiveBox { public: explicit VHBox(int minHeight):AdaptiveBox(minHeight,false) {} };
    }
    inline Qt::HBox & HBox() {
        return *(new Qt::HBox());
    }
    inline Qt::VBox & VBox() {
        return *(new Qt::VBox());
    }
    inline Qt::HVBox & HVBox(int minWidth) { return *new Qt::HVBox(minWidth); }
    inline Qt::VHBox & VHBox(int minHeight) { return *new Qt::VHBox(minHeight); }
}

#endif

#ifndef CPPUI_QT_DIVIDER
#define CPPUI_QT_DIVIDER

#include "layout.hpp"
#include <QFrame>

namespace CPPUI {
    namespace Qt {
        class Divider: public BaseWidget {
            bool boxed = false, row = false;
            public:
            // A line reports no minimum size, so Preferred lets a crowded box
            // or a Scroll shrink it to nothing; keep its thickness fixed.
            QSizePolicy::Policy staticPolicy(bool horizontal) const override {
                return boxed && horizontal == row ? QSizePolicy::Fixed : BaseWidget::staticPolicy(horizontal);
            }
            Divider() { is_terminal = true; is_movable = false; }
            void update(BaseWidget * prev) override {
                ui = prev && prev->ui ? prev->ui : new Widget<QFrame>();
                auto frame = static_cast<QFrame *>(ui);
                auto box=dynamic_cast<BaseLayout *>(dom_parent);
                const auto direction=box ? box->qtlayout->direction() : QBoxLayout::LeftToRight;
                const bool horizontal=direction==QBoxLayout::LeftToRight || direction==QBoxLayout::RightToLeft;
                boxed=box;row=horizontal;
                expandX=horizontal ? 0 : 1;expandY=horizontal ? 1 : 0;
                frame->setFrameShape(!box ? QFrame::NoFrame : horizontal ? QFrame::VLine : QFrame::HLine);
                frame->setFrameShadow(QFrame::Sunken);
            }
        };
    }
    inline Qt::Divider & Divider() { return *new Qt::Divider(); }
}
#endif

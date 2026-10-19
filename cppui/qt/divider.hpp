#ifndef CPPUI_QT_DIVIDER
#define CPPUI_QT_DIVIDER

#include "layout.hpp"
#include <QFrame>

namespace CPPUI {
    namespace Qt {
        class Divider: public BaseWidget {
            public:
            Divider() { is_terminal = true; is_movable = false; }
            void update(BaseWidget * prev) override {
                ui = prev && prev->ui ? prev->ui : new Widget<QFrame>();
                auto frame = static_cast<QFrame *>(ui);
                auto box=dynamic_cast<BaseLayout *>(dom_parent);
                const auto direction=box ? box->qtlayout->direction() : QBoxLayout::LeftToRight;
                const bool horizontal=direction==QBoxLayout::LeftToRight || direction==QBoxLayout::RightToLeft;
                frame->setFrameShape(!box ? QFrame::NoFrame : horizontal ? QFrame::VLine : QFrame::HLine);
                frame->setFrameShadow(QFrame::Sunken);
            }
        };
    }
    inline Qt::Divider & Divider() { return *new Qt::Divider(); }
}
#endif

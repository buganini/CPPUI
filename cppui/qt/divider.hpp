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
                ui = prev && prev->ui ? prev->ui : new QFrame();
                auto frame = static_cast<QFrame *>(ui);
                frame->setFrameShape(dynamic_cast<VBox *>(dom_parent) ? QFrame::HLine
                    : dynamic_cast<HBox *>(dom_parent) ? QFrame::VLine : QFrame::NoFrame);
                frame->setFrameShadow(QFrame::Sunken);
            }
        };
    }
    inline Qt::Divider & Divider() { return *new Qt::Divider(); }
}
#endif

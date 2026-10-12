#ifndef CPPUI_QT_SPLITTER
#define CPPUI_QT_SPLITTER

#include "base.hpp"
#include <QSplitter>

namespace CPPUI {
    namespace Qt {
        class Splitter: public BaseWidget {
            public:
            bool vertical;

            explicit Splitter(bool vertical = false): vertical(vertical) {}

            void update(BaseWidget * prev) override {
                ui = prev && prev->ui ? prev->ui : new Widget<QSplitter>();
                static_cast<QSplitter *>(ui)->setOrientation(vertical ? ::Qt::Vertical : ::Qt::Horizontal);
            }
            void addChild(int idx, Node * child) override {
                if(child->is_outoforder) return;
                static_cast<QSplitter *>(ui)->insertWidget(idx, static_cast<QWidget *>(child->outer()));
            }
            void removeChild(int, Node * child) override {
                if(child->is_outoforder) return;
                if(auto widget = static_cast<QWidget *>(child->outer())) widget->setParent(nullptr);
            }
        };
    }
    inline Qt::Splitter & Splitter(bool vertical = false) { return *new Qt::Splitter(vertical); }
}
#endif

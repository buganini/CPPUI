#ifndef CPPUI_QT_STACK
#define CPPUI_QT_STACK

#include "base.hpp"
#include <QStackedLayout>

namespace CPPUI { namespace Qt {
    class Stack: public LayoutWidget {
        protected:
        void clearNative() override { qtlayout = nullptr; BaseWidget::clearNative(); }
        public:
        QStackedLayout * qtlayout = nullptr;
        Stack() { is_reversed_order = true; expandX=expandY=3; }
        void update(BaseWidget * previous) override {
            auto prev = static_cast<Stack *>(previous);
            if(prev && prev->ui) { ui = prev->ui; qtlayout = prev->qtlayout; }
            else {
                ui = new Widget<QWidget>(); qtlayout = new QStackedLayout(ui);
                qtlayout->setStackingMode(QStackedLayout::StackAll);
            }
            qtlayout->setContentsMargins(0, 0, 0, 0);
            applyPadding();
        }
        void * inner() override { return qtlayout; }
        void addChild(int index, Node * child) override {
            if(child->is_outoforder) return;
            auto widget = dynamic_cast<BaseWidget *>(child);
            if(widget && widget->ui) qtlayout->insertWidget(index, widget->ui);
        }
        void removeChild(int, Node * child) override {
            if(child->is_outoforder) return;
            auto widget = dynamic_cast<BaseWidget *>(child);
            if(widget && widget->ui) widget->ui->setParent(nullptr);
        }
    };
}}
namespace CPPUI { inline Qt::Stack & Stack() { return *new Qt::Stack(); } }
#endif

#ifndef CPPUI_QT_MDI
#define CPPUI_QT_MDI

#include "base.hpp"
#include <QMdiArea>
#include <QMdiSubWindow>

namespace CPPUI { namespace Qt {
    class MdiArea: public BaseWidget {
        public:
        void update(BaseWidget * prev) override { ui = prev && prev->ui ? prev->ui : new QMdiArea(); }
        void addChild(int, Node * child) override {
            if(child->is_outoforder) return;
            auto widget = dynamic_cast<BaseWidget *>(child);
            if(widget && widget->ui) static_cast<QMdiArea *>(ui)->addSubWindow(widget->ui);
        }
        void removeChild(int, Node * child) override {
            if(child->is_outoforder || !ui) return;
            auto widget = dynamic_cast<BaseWidget *>(child);
            if(!widget || !widget->ui) return;
            auto area = static_cast<QMdiArea *>(ui);
            if(qobject_cast<QMdiSubWindow *>(widget->ui)) area->removeSubWindow(widget->ui);
            else {
                // Qt leaves its automatic wrapper behind when only the inner
                // widget is removed. Release that wrapper without deleting a
                // child that reconciliation may be moving to another slot.
                auto wrapper = qobject_cast<QMdiSubWindow *>(widget->ui->parentWidget());
                area->removeSubWindow(widget->ui);
                widget->ui->setParent(nullptr);
                if(wrapper && wrapper->mdiArea() == area) {
                    area->removeSubWindow(wrapper);
                    wrapper->deleteLater();
                }
            }
        }
    };

    class MdiSubWindow: public BaseWidget {
        public:
        void update(BaseWidget * prev) override { ui = prev && prev->ui ? prev->ui : new QMdiSubWindow(); }
        void addChild(int index, Node * child) override {
            if(child->is_outoforder) return;
            auto widget = dynamic_cast<BaseWidget *>(child);
            if(!widget || !widget->ui) return;
            if(index == 0) static_cast<QMdiSubWindow *>(ui)->setWidget(widget->ui);
            else {
                // PUI exposes only the first physical child. Keep ignored
                // widgets native-parent-owned so indirect teardown is complete.
                widget->ui->setParent(ui);
                widget->ui->hide();
            }
        }
        void removeChild(int, Node * child) override {
            if(child->is_outoforder) return;
            auto widget = dynamic_cast<BaseWidget *>(child);
            if(widget && widget->ui) widget->ui->setParent(nullptr);
        }
    };
}}
namespace CPPUI {
    inline Qt::MdiArea & MdiArea() { return *new Qt::MdiArea(); }
    inline Qt::MdiSubWindow & MdiSubWindow() { return *new Qt::MdiSubWindow(); }
}
#endif

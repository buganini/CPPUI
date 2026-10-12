#ifndef CPPUI_QT_TOOLBAR
#define CPPUI_QT_TOOLBAR

#include "menu.hpp"
#include <QToolBar>
#include <QWidgetAction>
#include <QIcon>

namespace CPPUI { namespace Qt {
    class ToolBarAction: public MenuAction {
        std::string iconPath;
        public:
        ToolBarAction(const std::string & text, const std::string & iconPath = {}) : MenuAction(text), iconPath(iconPath) {}
        void update(Node * prev) override {
            MenuAction::update(prev);
            if(!iconPath.empty()) ui->setIcon(QIcon(QString::fromStdString(iconPath)));
        }
    };
    class ToolBar: public BaseWidget {
        public:
        ToolBar() { is_outoforder = true; }
        void update(BaseWidget * prev) override { ui = prev && prev->ui ? prev->ui : new Widget<QToolBar>(); }
        void addChild(int index, Node * child) override {
            if(child->is_outoforder) return;
            auto toolbar = static_cast<QToolBar *>(ui);
            auto before = toolbar->actions().value(index, nullptr);
            if(auto action = dynamic_cast<MenuAction *>(child)) {
                action->ui->setParent(toolbar);
                toolbar->insertAction(before, action->ui);
            } else if(auto widget = dynamic_cast<BaseWidget *>(child)) {
                // QWidgetAction keeps ownership during a keyed move. Reuse its
                // default widget; destroying the action would delete the widget.
                for(auto action: toolbar->findChildren<QWidgetAction *>(QString(), ::Qt::FindDirectChildrenOnly)) {
                    if(action->defaultWidget() == widget->ui) { toolbar->insertAction(before, action); return; }
                }
                auto action = toolbar->insertWidget(before, widget->ui);
                QObject::connect(widget->ui, &QObject::destroyed, action, &QObject::deleteLater);
            }
        }
        void removeChild(int, Node * child) override {
            if(child->is_outoforder || !ui) return;
            auto toolbar = static_cast<QToolBar *>(ui);
            if(auto action = dynamic_cast<MenuAction *>(child)) {
                if(action->ui) { toolbar->removeAction(action->ui); action->ui->setParent(nullptr); }
            } else if(auto widget = dynamic_cast<BaseWidget *>(child)) {
                for(auto action: toolbar->actions()) if(toolbar->widgetForAction(action) == widget->ui) {
                    toolbar->removeAction(action);
                    return;
                }
            }
        }
    };
}}
namespace CPPUI {
    inline Qt::ToolBar & ToolBar() { return *new Qt::ToolBar(); }
    inline Qt::ToolBarAction & ToolBarAction(const std::string & text, const std::string & iconPath = {}) {
        return *new Qt::ToolBarAction(text, iconPath);
    }
}
#endif

#ifndef CPPUI_QT_MENU
#define CPPUI_QT_MENU

#include "base.hpp"
#include <QAction>
#include <QMenu>
#include <QMenuBar>

namespace CPPUI { namespace Qt {
    class MenuAction: public Node {
        QMetaObject::Connection nativeDestroyed, triggered;
        std::function<void()> callback;
        void release() {
            QObject::disconnect(triggered); QObject::disconnect(nativeDestroyed);
            triggered = {}; nativeDestroyed = {}; ui = nullptr;
        }
        public:
        QAction * ui = nullptr;
        std::string text;
        explicit MenuAction(const std::string & text): text(text) { is_terminal = true; }
        ~MenuAction() override { release(); }
        MenuAction & trigger(std::function<void()> value) { callback = std::move(value); return *this; }
        void update(Node * previous) override {
            auto prev = static_cast<MenuAction *>(previous);
            if(prev) QObject::disconnect(prev->triggered);
            ui = prev && prev->ui ? prev->ui : new QAction();
            ui->setText(QString::fromStdString(text));
            auto invoke = callback;
            triggered = QObject::connect(ui, &QAction::triggered, ui, [invoke]() {
                auto current = invoke;
                try { if(current) current(); }
                catch(const std::exception & error) { std::cerr << "CPPUI action: " << error.what() << '\n'; }
                catch(...) { std::cerr << "CPPUI action: unknown exception\n"; }
            });
            nativeDestroyed = QObject::connect(ui, &QObject::destroyed, [this]() { ui = nullptr; });
            Node::update(previous);
        }
        void * outer() override { return ui; }
        void releaseRetiredRefs() override { release(); callback = {}; Node::releaseRetiredRefs(); }
        void destroy(bool direct) override { auto action = ui; release(); if(direct && action) action->deleteLater(); }
    };

    inline QAction * menuAction(Node * child) {
        if(auto action = dynamic_cast<MenuAction *>(child)) return action->ui;
        if(auto widget = dynamic_cast<BaseWidget *>(child)) {
            if(auto menu = qobject_cast<QMenu *>(widget->ui)) return menu->menuAction();
        }
        return nullptr;
    }
    inline void insertMenuChild(QWidget * parent, int index, Node * child) {
        auto action = menuAction(child);
        if(!action) return;
        if(auto widget = dynamic_cast<BaseWidget *>(child)) widget->ui->setParent(parent, widget->ui->windowFlags());
        else action->setParent(parent);
        parent->insertAction(parent->actions().value(index, nullptr), action);
    }
    inline void removeMenuChild(QWidget * parent, Node * child) {
        auto action = menuAction(child);
        if(!action || !parent) return;
        parent->removeAction(action);
        if(auto widget = dynamic_cast<BaseWidget *>(child)) {
            widget->ui->hide(); widget->ui->setParent(nullptr, widget->ui->windowFlags());
        } else action->setParent(nullptr);
    }
    class Menu: public BaseWidget {
        std::string text;
        public:
        explicit Menu(const std::string & text): text(text) {}
        void update(BaseWidget * prev) override {
            ui = prev && prev->ui ? prev->ui : new QMenu();
            static_cast<QMenu *>(ui)->setTitle(QString::fromStdString(text));
        }
        void addChild(int index, Node * child) override { insertMenuChild(ui, index, child); }
        void removeChild(int, Node * child) override { removeMenuChild(ui, child); }
    };
    class MenuBar: public BaseWidget {
        public:
        MenuBar() { is_outoforder = true; }
        void update(BaseWidget * prev) override { ui = prev && prev->ui ? prev->ui : new QMenuBar(); }
        void addChild(int index, Node * child) override { insertMenuChild(ui, index, child); }
        void removeChild(int, Node * child) override { removeMenuChild(ui, child); }
    };
}}
namespace CPPUI {
    inline Qt::MenuBar & MenuBar() { return *new Qt::MenuBar(); }
    inline Qt::Menu & Menu(const std::string & text) { return *new Qt::Menu(text); }
    inline Qt::MenuAction & MenuAction(const std::string & text) { return *new Qt::MenuAction(text); }
}
#endif

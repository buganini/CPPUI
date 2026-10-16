#ifndef CPPUI_QT_WINDOW
#define CPPUI_QT_WINDOW

#include "base.hpp"
#include "../application_title.hpp"
#include "menu.hpp"
#include "toolbar.hpp"
#include "../optional.hpp"

#include <QMainWindow>
#include <QApplication>
#include <QLineEdit>
#include <QIcon>
#include <QPointer>

namespace CPPUI {
    namespace Qt {
        class WindowWidget: public Widget<QMainWindow> {
            public:
            WindowWidget() {
                // Content size hints must not force the top-level window to grow.
                layout()->setSizeConstraint(QLayout::SetNoConstraint);
            }
            protected:
            void mousePressEvent(QMouseEvent * event) override {
                QPointer<WindowWidget> alive(this);
                if(auto editor = qobject_cast<QLineEdit *>(QApplication::focusWidget())) editor->clearFocus();
                if(alive) QMainWindow::mousePressEvent(event);
            }
        };
        class Window: public BaseWidget {
            OptionalText caption, iconPath;
            Optional<QSize> dimensions;
            Optional<bool> initialMaximized, initialFullscreen;
            static void windowFlag(QWidget * widget, ::Qt::WindowState flag, bool value) {
                widget->setWindowState(value ? widget->windowState() | flag : widget->windowState() & ~flag);
            }
            public:
            Window & title(const std::string & value) { caption = value; return *this; }
            Window & icon(const std::string & path) { iconPath = path.empty() ? OptionalText() : OptionalText(path); return *this; }
            Window & size(int width, int height) { dimensions = QSize(width, height); return *this; }
            Window & maximize(bool value = true) {
                initialMaximized = value;
                return *this;
            }
            Window & fullscreen(bool value = true) {
                initialFullscreen = value;
                return *this;
            }

            void update(BaseWidget * previous) override {
                auto prev = static_cast<Window *>(previous);
                const bool creating = !prev || !prev->ui;
                ui = creating ? new WindowWidget() : prev->ui;
                if(dimensions && (creating || !prev->dimensions || *dimensions != *prev->dimensions)) ui->resize(*dimensions);
                auto title = windowTitle(this, caption);
                if(title) ui->setWindowTitle(QString::fromStdString(*title));
                if(iconPath) ui->setWindowIcon(QIcon(QString::fromStdString(*iconPath)));
                if(creating) {
                    if(initialMaximized) windowFlag(ui, ::Qt::WindowMaximized, *initialMaximized);
                    if(initialFullscreen) windowFlag(ui, ::Qt::WindowFullScreen, *initialFullscreen);
                }
            }

            void addChild(int idx, Node *child) override {
                auto window = static_cast<QMainWindow *>(ui);
                if(auto menu = dynamic_cast<MenuBar *>(child)) { window->setMenuBar(static_cast<QMenuBar *>(menu->ui)); return; }
                if(auto toolbar = dynamic_cast<ToolBar *>(child)) { window->addToolBar(static_cast<QToolBar *>(toolbar->ui)); return; }
                if(child->is_outoforder) return;
                ((QMainWindow *)ui)->setCentralWidget((QWidget *) child->outer());
            }

            void removeChild(int idx, Node *child) override {
                if(auto menu = dynamic_cast<MenuBar *>(child)) { if(menu->ui) menu->ui->setParent(nullptr); return; }
                if(auto toolbar = dynamic_cast<ToolBar *>(child)) {
                    if(toolbar->ui) {
                        static_cast<QMainWindow *>(ui)->removeToolBar(static_cast<QToolBar *>(toolbar->ui));
                        toolbar->ui->setParent(nullptr);
                    }
                    return;
                }
                if(child->is_outoforder) return;
                if(auto widget = static_cast<QWidget *>(child->outer())) widget->setParent(nullptr);
            }
       };
    }
    inline Qt::Window & Window() {
        return *(new Qt::Window());
    }
}

#endif

#ifndef CPPUI_COOKBOOK_QT_BACKEND
#define CPPUI_COOKBOOK_QT_BACKEND
#include "cppui/qt/application.hpp"
#include "cppui/qt/window.hpp"
#include "cppui/qt/layout.hpp"
#include "cppui/qt/divider.hpp"
#include "cppui/qt/label.hpp"
#include "cppui/qt/button.hpp"
#include "cppui/qt/checkbox.hpp"
#include "cppui/qt/progressbar.hpp"
#include "cppui/qt/text.hpp"
#include "cppui/qt/splitter.hpp"
#include "cppui/qt/canvas.hpp"
#include "cppui/qt/tab.hpp"
#include "cppui/qt/dialog.hpp"
#include "cppui/qt/modal.hpp"
#include "cppui/qt/mdi.hpp"
#include "cppui/qt/image.hpp"
#include "cppui/qt/timeline.hpp"
#include <QTimer>
#include "../recipes.hpp"
#include "presentation.hpp"
#include <stdexcept>

namespace Cookbook { namespace Backend {
    inline void mount(CPPUI::Application & app) { app.sync(); }
    void configureRecipes(std::vector<Recipe> & pages);
    const char * const name = "Qt";
    inline void settle() {
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
    inline bool clickButton(CPPUI::Node & node, const std::string & text) {
        auto button = dynamic_cast<CPPUI::Qt::Button *>(&node);
        if(!button || button->text != text) return false;
        static_cast<QPushButton *>(button->outer())->click();
        return true;
    }
    inline bool labelHasText(CPPUI::Node & node, const std::string & text) {
        auto widget = dynamic_cast<CPPUI::Qt::BaseWidget *>(&node);
        auto label = widget ? qobject_cast<QLabel *>(widget->ui) : nullptr;
        return label && label->text().toStdString() == text;
    }
    inline bool selectTab(CPPUI::Node & node, int index, const std::string & label) {
        auto tabs = dynamic_cast<CPPUI::Qt::Tabs *>(&node);
        if(!tabs) return false;
        auto widget = static_cast<QTabWidget *>(tabs->outer());
        if(index < 0 || index >= widget->count() || widget->tabText(index).toStdString() != label) return false;
        widget->setCurrentIndex(index);
        return widget->currentIndex() == index && widget->currentWidget()->isVisible();
    }
    inline bool toggleCheckbox(CPPUI::Node & node) {
        auto checkbox = dynamic_cast<CPPUI::Qt::Checkbox *>(&node);
        if(!checkbox || !checkbox->ui) return false;
        static_cast<QCheckBox *>(checkbox->ui)->click();
        return true;
    }
    inline void answerPrompt(const std::string & text, bool accepted) {
        QTimer::singleShot(0, qApp, [text, accepted]() {
            if(auto dialog = qobject_cast<QInputDialog *>(QApplication::activeModalWidget())) {
                dialog->setTextValue(QString::fromStdString(text));
                accepted ? dialog->accept() : dialog->reject();
            }
        });
    }
    inline bool modalVisible(CPPUI::Node & node) {
        auto modal = dynamic_cast<CPPUI::Qt::Modal *>(&node);
        return modal && modal->ui && modal->ui->isVisible()
            && static_cast<QDialog *>(modal->ui)->isModal();
    }
    inline bool selectPage(CPPUI::Node & node) {
        auto link = dynamic_cast<Presentation::PageLinkNode *>(&node);
        if(!link) return false;
        auto label = static_cast<QWidget *>(link->outer());
        for(auto parent = label->parentWidget(); parent; parent = parent->parentWidget()) {
            if(auto scroll = qobject_cast<QScrollArea *>(parent)) scroll->ensureWidgetVisible(label);
        }
        QMouseEvent event(QEvent::MouseButtonPress, QPointF(2, 2), QPointF(2, 2),
                          ::Qt::LeftButton, ::Qt::NoButton, ::Qt::NoModifier);
        QCoreApplication::sendEvent(label, &event);
        return true;
    }
    inline void prepareLayoutCheck(CPPUI::Node & root) {
        auto window = static_cast<QMainWindow *>(find(root, "cookbook")->outer());
        if(!window->isMaximized() || window->windowTitle() != "CPPUI Cookbook") {
            throw std::runtime_error("cookbook should open maximized with its window title");
        }
        // Force an overflowing list independently of the host's screen size.
        window->showNormal();
        window->resize(800, 600);
        settle();
    }
    inline bool pageVisible(CPPUI::Node & root, const std::string & key) {
        auto scroll = static_cast<QScrollArea *>(find(root, "menu-scroller")->outer());
        auto label = static_cast<QWidget *>(find(root, "page:" + key)->outer());
        return scroll->viewport()->rect().contains(label->mapTo(scroll->viewport(), label->rect().center()));
    }
    inline void checkLayout(CPPUI::Node & root) {
        auto window = static_cast<QMainWindow *>(find(root, "cookbook")->outer());
        auto sidebar = static_cast<QWidget *>(find(root, "menu-container")->outer());
        auto result = static_cast<QWidget *>(find(root, "result")->outer());
        auto scroll = static_cast<QScrollArea *>(find(root, "menu-scroller")->outer());
        if(sidebar->geometry().right() >= result->geometry().left()
           || result->width() <= sidebar->width() || result->height() < 200) {
            throw std::runtime_error("cookbook must retain a sidebar beside an expanding result pane");
        }
        if(scroll->verticalScrollBar()->maximum() <= 0 || !scroll->widget()
           || scroll->widget()->findChildren<QLabel *>().size() != static_cast<int>(recipes().size())) {
            throw std::runtime_error("cookbook page list must scroll and include every recipe");
        }
        const auto menus = window->menuBar()->actions();
        if(menus.size() != 2 || menus[0]->text() != "Menu" || menus[1]->text() != "Menu3"
           || menus[0]->menu()->actions().size() != 3
           || menus[0]->menu()->actions()[2]->menu()->actions().size() != 2
           || menus[1]->menu()->actions().size() != 2) {
            throw std::runtime_error("reference menus are missing or duplicated");
        }
        if(find(root, "code")) throw std::runtime_error("source code pane must be omitted");
    }
}}
#endif

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
#include "cppui/qt/views.hpp"
#include "cppui/qt/table.hpp"
#include "cppui/qt/tree.hpp"
#include "cppui/qt/dialog.hpp"
#include "cppui/qt/modal.hpp"
#include "cppui/qt/mdi.hpp"
#include "cppui/qt/image.hpp"
#include "cppui/qt/textfield.hpp"
#include "cppui/qt/radiobutton.hpp"
#include "cppui/qt/combobox.hpp"
#include "cppui/qt/scroll.hpp"
#include "cppui/qt/timeline.hpp"
#include <QTimer>
#include "../recipes.hpp"
#include <stdexcept>

namespace Cookbook { namespace Backend {
    using View = CPPUI::Qt::QtView;
    using Views = CPPUI::Qt::Views;
    inline bool viewSelected(CPPUI::Node & node, const std::string & key) {
        auto views = dynamic_cast<Views *>(&node);
        if(!views || !views->ui) return false;
        const int index = static_cast<QStackedWidget *>(views->ui)->currentIndex();
        return key.empty() ? index == 0 : index > 0 && views->children->at(index - 1)->_id == key;
    }
    inline void configureWindow(CPPUI::Context * node) { static_cast<CPPUI::Qt::Window *>(node)->title("CPPUI Cookbook").maximize(); }
    inline void menus() {
        for(auto bar: CPPUI::MenuBar()) {
            for(auto menu: CPPUI::Menu("Menu")) {
                CPPUI::MenuAction("Action").trigger([]() { std::cout << "action\n"; });
                CPPUI::MenuAction("Action2").trigger([]() { std::cout << "action2\n"; });
                for(auto submenu: CPPUI::Menu("Menu2")) {
                    CPPUI::MenuAction("Action3").trigger([]() { std::cout << "action3\n"; });
                    CPPUI::MenuAction("Action4").trigger([]() { std::cout << "action4\n"; });
                }
            }
            for(auto menu: CPPUI::Menu("Menu3")) {
                CPPUI::MenuAction("Action5");
                CPPUI::MenuAction("Action6");
            }
        }
    }

    inline void mount(CPPUI::Application & app) { app.sync(); }
    inline bool wheelCanvas(CPPUI::Node & node, int delta) {
        auto canvas = dynamic_cast<CPPUI::Qt::Canvas *>(&node);
        if(!canvas || !canvas->ui) return false;
        auto widget = static_cast<QWidget *>(canvas->ui);
        widget->grab(); // Paint once so the recipe has the current plot bounds.
        const QPointF point(widget->width()/2.0, 100);
        QWheelEvent event(point, widget->mapToGlobal(point), {}, QPoint(0, delta),
                          ::Qt::NoButton, ::Qt::NoModifier, ::Qt::NoScrollPhase, false);
        QCoreApplication::sendEvent(widget, &event); return true;
    }
    void configureRecipes(std::vector<Recipe> & pages);
    const char * const name = "Qt";
    inline void settle() {
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
    inline QModelIndex treeIndex(CPPUI::Qt::Tree & tree, const std::vector<int> & path) {
        QModelIndex index;
        for(int row: path) {
            index = tree.model->index(row, 0, index);
            if(!index.isValid()) return {};
        }
        return index;
    }
    inline int treeChildCount(CPPUI::Node & node, const std::vector<int> & path) {
        auto tree = dynamic_cast<CPPUI::Qt::Tree *>(&node);
        if(!tree || !tree->model) return -1;
        const auto index = treeIndex(*tree, path);
        return path.empty() || index.isValid() ? tree->model->rowCount(index) : -1;
    }
    inline bool treeExpanded(CPPUI::Node & node, const std::vector<int> & path) {
        auto tree = dynamic_cast<CPPUI::Qt::Tree *>(&node);
        return tree && tree->model && static_cast<QTreeView *>(tree->ui)->isExpanded(treeIndex(*tree, path));
    }
    inline std::string tableText(CPPUI::Node & node, int row, int column) {
        auto table = dynamic_cast<CPPUI::Qt::Table *>(&node);
        return table && table->model ? table->model->data(table->model->index(row, column)).toString().toStdString() : std::string();
    }
    inline bool editTable(CPPUI::Node & node, int row, int column, const std::string & value) {
        auto table = dynamic_cast<CPPUI::Qt::Table *>(&node);
        return table && table->model && table->model->setData(table->model->index(row, column), QString::fromStdString(value));
    }
    inline bool atScrollEnd(CPPUI::Node & node) {
        auto scroll = dynamic_cast<CPPUI::Qt::Scroll *>(&node);
        if(!scroll || !scroll->ui) return false;
        auto bar = static_cast<QScrollArea *>(scroll->ui)->verticalScrollBar();
        return bar->maximum() > 0 && bar->value() == bar->maximum();
    }
    inline bool selectCombo(CPPUI::Node & node, int index) {
        auto combo = dynamic_cast<CPPUI::Qt::ComboBox *>(&node);
        if(!combo || !combo->ui) return false;
        static_cast<QComboBox *>(combo->ui)->setCurrentIndex(index); return true;
    }
    inline bool selectRadio(CPPUI::Node & node) {
        auto widget = dynamic_cast<CPPUI::Qt::BaseWidget *>(&node);
        auto radio = widget ? qobject_cast<QRadioButton *>(widget->ui) : nullptr;
        if(!radio) return false;
        radio->click(); return true;
    }
    inline bool editTextField(CPPUI::Node & node, const std::string & text, bool commit) {
        auto editor = dynamic_cast<CPPUI::Qt::TextField *>(&node);
        if(!editor || !editor->ui) return false;
        auto native = static_cast<QLineEdit *>(editor->ui);
        native->setText(QString::fromStdString(text));
        if(commit) native->editingFinished();
        return true;
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
        auto link = dynamic_cast<CPPUI::Qt::Label *>(&node);
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

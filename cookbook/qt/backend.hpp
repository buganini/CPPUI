#ifndef CPPUI_COOKBOOK_QT_BACKEND
#define CPPUI_COOKBOOK_QT_BACKEND
#include "cppui/qt/application.hpp"
#include "cppui/qt/window.hpp"
#include "cppui/qt/layout.hpp"
#include "cppui/qt/label.hpp"
#include "cppui/qt/button.hpp"
#include "cppui/qt/progressbar.hpp"
#include "../recipes.hpp"

namespace Cookbook { namespace Backend {
    void configureRecipes(std::vector<Recipe> & pages);
    const char * const name = "Qt";
    inline void settle() {
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
        auto label = dynamic_cast<CPPUI::Qt::Label *>(&node);
        return label && static_cast<QLabel *>(label->outer())->text().toStdString() == text;
    }
}}
#endif

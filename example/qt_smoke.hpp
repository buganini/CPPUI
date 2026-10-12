#ifndef CPPUI_EXAMPLE_QT_SMOKE
#define CPPUI_EXAMPLE_QT_SMOKE

#include "../cppui/qt/button.hpp"
#include <QApplication>
#include <QEvent>
#include <QLabel>
#include <stdexcept>
#include <string>

namespace QtSmoke {
    inline bool requested(int argc, char ** argv) {
        for(int i = 1; i < argc; ++i) {
            if(std::string(argv[i]) == "--smoke") return true;
        }
        return false;
    }
    inline void require(bool value, const char * message) {
        if(!value) throw std::runtime_error(message);
    }
    inline void settle() {
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
    inline bool click(CPPUI::Node & node, const std::string & text) {
        auto button = dynamic_cast<CPPUI::Qt::Button *>(&node);
        if(button && button->text == text) {
            static_cast<QPushButton *>(button->outer())->click();
            return true;
        }
        for(auto child: *node.children) {
            if(click(*child, text)) return true;
        }
        return false;
    }
    inline bool hasText(CPPUI::Node & node, const std::string & text) {
        auto widget = dynamic_cast<CPPUI::Qt::BaseWidget *>(&node);
        auto label = widget ? qobject_cast<QLabel *>(widget->ui) : nullptr;
        if(label && label->text().toStdString() == text) return true;
        for(auto child: *node.children) {
            if(hasText(*child, text)) return true;
        }
        return false;
    }
}
#endif

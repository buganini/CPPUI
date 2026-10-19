#include "backend.hpp"

namespace ExampleTest {
    void settle() {
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
    bool clickButton(CPPUI::Node & node, const std::string & text) {
        auto button = dynamic_cast<CPPUI::Qt::Button *>(&node);
        if(!button || button->text != text) return false;
        static_cast<QPushButton *>(button->outer())->click();
        return true;
    }
    bool labelHasText(CPPUI::Node & node, const std::string & text) {
        auto widget = dynamic_cast<CPPUI::Qt::BaseWidget *>(&node);
        auto label = widget ? qobject_cast<QLabel *>(widget->ui) : nullptr;
        return label && label->text().toStdString() == text;
    }
    bool toggleCheckbox(CPPUI::Node & node) {
        auto checkbox = dynamic_cast<CPPUI::Qt::Checkbox *>(&node);
        if(!checkbox || !checkbox->ui) return false;
        static_cast<QCheckBox *>(checkbox->ui)->click();
        return true;
    }
}

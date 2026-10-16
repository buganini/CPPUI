#ifndef CPPUI_QT_CHECKBOX
#define CPPUI_QT_CHECKBOX

#include "base.hpp"
#include "../binding.hpp"
#include <QCheckBox>
#include <QSignalBlocker>
#include <QApplication>

namespace CPPUI {
    namespace Qt {
        class Checkbox: public BaseWidget {
            Binding<bool> model;
            bool checked, isDisabled = false;
            public:
            std::string text;
            Checkbox(const std::string & text, Binding<bool> model): model(model), checked(model.get()), text(text) {
                is_terminal = true;
            }
            Checkbox & disabled(bool value = true) { isDisabled = value; return *this; }
            void update(BaseWidget * prev) override {
                ui = prev && prev->ui ? prev->ui : new Widget<QCheckBox>();
                auto checkbox = static_cast<QCheckBox *>(ui);
                {
                    QSignalBlocker block(checkbox);
                    checkbox->setText(QString::fromStdString(text));
                    checkbox->setChecked(checked);
                    checkbox->setDisabled(isDisabled);
                }
                auto binding = model;
                connectSignal(QObject::connect(checkbox, &QCheckBox::toggled, checkbox, [binding](bool value) {
                    auto current = binding;
                    try { current.set(value); }
                    catch(const std::exception & error) { std::cerr << "CPPUI Checkbox: " << error.what() << '\n'; }
                    catch(...) { std::cerr << "CPPUI Checkbox: unknown exception\n"; }
                }));
                auto callback = onClicked;
                connectSignal(QObject::connect(checkbox, &QCheckBox::clicked, checkbox, [callback]() {
                    auto invoke = callback;
                    try {
                        if(auto focused = QApplication::focusWidget()) focused->clearFocus();
                        if(invoke) invoke();
                    } catch(const std::exception & error) { std::cerr << "CPPUI Checkbox click: " << error.what() << '\n'; }
                    catch(...) { std::cerr << "CPPUI Checkbox click: unknown exception\n"; }
                }));
            }
        };
    }
    inline Qt::Checkbox & Checkbox(const std::string & text, Binding<bool> model) { return *new Qt::Checkbox(text, model); }
}
#endif

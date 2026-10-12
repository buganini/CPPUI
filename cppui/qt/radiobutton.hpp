#ifndef CPPUI_QT_RADIOBUTTON
#define CPPUI_QT_RADIOBUTTON

#include "base.hpp"
#include "../binding.hpp"
#include <QRadioButton>
#include <QSignalBlocker>
#include <QApplication>

namespace CPPUI { namespace Qt {
    template<class T>
    class RadioButton: public BaseWidget {
        std::string text;
        T value;
        Binding<T> model;
        bool checked;
        public:
        using Value = T;
        RadioButton(const std::string & text, const T & value, Binding<T> model)
            : text(text), value(value), model(model), checked(model.get() == value) { is_terminal = true; }
        void update(BaseWidget * prev) override {
            ui = prev && prev->ui ? prev->ui : new QRadioButton();
            auto button = static_cast<QRadioButton *>(ui);
            {
                QSignalBlocker block(button);
                button->setText(QString::fromStdString(text));
                button->setChecked(checked);
            }
            auto binding = model;
            auto selected = value;
            auto callback = onClicked;
            connectSignal(QObject::connect(button, &QRadioButton::clicked, button, [binding, selected, callback]() {
                auto current = binding;
                auto value = selected;
                auto invoke = callback;
                try {
                    current.set(value);
                    if(auto focused = QApplication::focusWidget()) focused->clearFocus();
                    if(invoke) invoke();
                } catch(const std::exception & error) { std::cerr << "CPPUI RadioButton: " << error.what() << '\n'; }
                catch(...) { std::cerr << "CPPUI RadioButton: unknown exception\n"; }
            }));
        }
    };
}}
namespace CPPUI {
    template<class T>
    inline Qt::RadioButton<T> & RadioButton(const std::string & text, const typename Qt::RadioButton<T>::Value & value, Binding<T> model) {
        return *new Qt::RadioButton<T>(text, value, model);
    }
    template<class T>
    inline Qt::RadioButton<T> & RadioButton(const std::string & text, const typename Qt::RadioButton<T>::Value & value, State<T> & model) {
        return *new Qt::RadioButton<T>(text, value, Binding<T>(model));
    }
}
#endif

#ifndef CPPUI_QT_BUTTON
#define CPPUI_QT_BUTTON

#include "base.hpp"

#include <QPushButton>
#include <QApplication>

namespace CPPUI {
    namespace Qt {
        class Button: public BaseWidget {
            public:
            std::string text;

            explicit Button(const std::string & text): text(text) {}
            void update(BaseWidget * prev) {
                if(prev && prev->ui) {
                    ui = prev->ui;
                    ((QPushButton *)ui)->setText(QString::fromStdString(text));
                } else {
                    ui = new QPushButton(QString::fromStdString(text));
                }
                // Copy before invoking user code: it may redraw or remove this
                // declaration and disconnect the currently executing connection.
                auto callback = onClicked;
                connectSignal(QObject::connect(static_cast<QPushButton *>(ui),
                    &QPushButton::clicked, ui, [callback]() {
                        auto invoke = callback;
                        // Focus-out handlers may synchronously rebuild this button.
                        if(auto focused = QApplication::focusWidget()) focused->clearFocus();
                        invoke();
                    }));
            }
        };
    }
    inline Qt::Button & Button(const std::string & text) {
        return *(new Qt::Button(text));
    }
}

#endif

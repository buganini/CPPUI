#ifndef CPPUI_QT_BUTTON
#define CPPUI_QT_BUTTON

#include "base.hpp"

#include <QPushButton>

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
                    QObject::disconnect((QPushButton *)ui, &QPushButton::clicked, nullptr, nullptr);
                } else {
                    ui = new QPushButton(QString::fromStdString(text));
                }
                QObject::connect((QPushButton *)ui, &QPushButton::clicked, [=]() {
                    onClicked();
                });
            }
        };
    }
    inline Qt::Button & Button(const std::string & text) {
        return *(new Qt::Button(text));
    }
}

#endif

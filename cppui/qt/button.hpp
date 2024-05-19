#include "base.hpp"

#include <QPushButton>

namespace CPPUI {
    namespace Qt {
        class Button: public BaseWidget {
            public:
            const char * text;

            Button(const char * text) {
                this->text = text;
            }
            void update(BaseWidget * prev) {
                if(prev && prev->ui) {
                    ui = prev->ui;
                    ((QPushButton *)ui)->setText(text);
                    QObject::disconnect((QPushButton *)ui, &QPushButton::clicked, nullptr, nullptr);
                } else {
                    ui = new QPushButton(text);
                }
                QObject::connect((QPushButton *)ui, &QPushButton::clicked, [=]() {
                    onClicked();
                });
            }
        };
    }
    Qt::Button & Button(const char * text) {
        return *(new Qt::Button(text));
    }
}

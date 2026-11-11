#ifndef CPPUI_QT_BUTTON
#define CPPUI_QT_BUTTON

#include "base.hpp"

#include <QPushButton>
#include <QApplication>
#include <QStyleOptionButton>
#include <QIcon>

namespace CPPUI {
    namespace Qt {
        class ContentButton: public QPushButton {
        public:
            using QPushButton::QPushButton;
            QSize sizeHint() const override {
                auto size=QPushButton::sizeHint();
                if(!icon().isNull() || menu()) return size;
                QStyleOptionButton option;initStyleOption(&option);
                const auto content=fontMetrics().size(::Qt::TextShowMnemonic,option.text);
                option.text.clear();
                size.setWidth(style()->sizeFromContents(QStyle::CT_PushButton,&option,content,this).width());
                return size;
            }
        };
        class Button: public BaseWidget {
            bool isDisabled = false;
            public:
            std::string text;
            std::string iconPath;

            explicit Button(const std::string & text = ""): text(text) {}
            Button & icon(const std::string & path) { iconPath = path; return *this; }
            Button & disabled(bool value = true) { isDisabled = value; return *this; }
            void update(BaseWidget * prev) {
                if(prev && prev->ui) {
                    ui = prev->ui;
                    ((QPushButton *)ui)->setText(QString::fromStdString(text));
                } else {
                    ui = new Widget<ContentButton>(QString::fromStdString(text));
                }
                auto button = static_cast<QPushButton *>(ui);
                if(!iconPath.empty())
                    button->setIcon(QIcon(QString::fromStdString(iconPath)));
                else
                    button->setIcon(QIcon());
                ui->setDisabled(isDisabled);
                // Copy before invoking user code: it may redraw or remove this
                // declaration and disconnect the currently executing connection.
                auto callback = onClicked;
                connectSignal(QObject::connect(button,
                    &QPushButton::clicked, ui, [callback]() {
                        auto invoke = callback;
                        // Focus-out handlers may synchronously rebuild this button.
                        if(auto focused = QApplication::focusWidget()) focused->clearFocus();
                        invoke();
                    }));
            }
        };
    }
    inline Qt::Button & Button(const std::string & text = "") {
        return *(new Qt::Button(text));
    }
}

#endif

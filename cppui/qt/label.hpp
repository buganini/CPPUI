#ifndef CPPUI_QT_LABEL
#define CPPUI_QT_LABEL

#include <string>

#include <QLabel>
#include <QMouseEvent>
#include <QApplication>
#include <QPointer>

#include "base.hpp"

namespace CPPUI {
    namespace Qt {
        class ClickableLabel: public QLabel {
            public:
            std::function<void()> callback;
            void mousePressEvent(QMouseEvent * event) override {
                auto invoke = callback;
                QPointer<ClickableLabel> alive(this);
                try {
                    if(invoke) {
                        if(auto focused = QApplication::focusWidget()) focused->clearFocus();
                        invoke();
                    }
                } catch(const std::exception & error) { std::cerr << "CPPUI Label click: " << error.what() << '\n'; }
                catch(...) { std::cerr << "CPPUI Label click: unknown exception\n"; }
                if(alive) QLabel::mousePressEvent(event);
            }
        };
        class Label: public BaseWidget {
            bool hasCallback = false;
            public:
            std::string text;
            bool selectable;
            Label(const char * text, bool selectable = false): selectable(selectable) {
                this->text = std::string(text);
                is_terminal = true;
            }
            Label(const std::string & text, bool selectable = false): selectable(selectable) {
                this->text = text;
                is_terminal = true;
            }
            void click(std::function<void()> callback) override {
                hasCallback = bool(callback);
                Node::click(std::move(callback));
            }
            void destroy(bool direct) override {
                if(ui) static_cast<ClickableLabel *>(ui)->callback = {};
                BaseWidget::destroy(direct);
            }
            void update(BaseWidget * prev) {
                if(prev && prev->ui) {
                    ui = prev->ui;
                } else {
                    ui = new ClickableLabel();
                }
                auto label = static_cast<ClickableLabel *>(ui);
                label->callback = hasCallback ? onClicked : std::function<void()>();
                if(hasCallback) label->setCursor(::Qt::PointingHandCursor); else label->unsetCursor();
                ((QLabel *)ui)->setTextFormat(::Qt::PlainText);
                ((QLabel *)ui)->setText(text.c_str());
                ((QLabel *)ui)->setTextInteractionFlags(
                    selectable ? ::Qt::TextSelectableByMouse : ::Qt::NoTextInteraction);
            }
        };
    }
    inline Qt::Label & Label(const char * text, bool selectable = false) {
        return *(new Qt::Label(text, selectable));
    }
    inline Qt::Label & Label(const std::string & text, bool selectable = false) {
        return *(new Qt::Label(text, selectable));
    }
}

#endif

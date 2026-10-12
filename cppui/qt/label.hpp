#ifndef CPPUI_QT_LABEL
#define CPPUI_QT_LABEL

#include <string>

#include <QLabel>

#include "base.hpp"

namespace CPPUI {
    namespace Qt {
        class Label: public BaseWidget {
            public:
            std::string text;
            bool selectable;
            Label(const char * text, bool selectable = false): selectable(selectable) {
                this->text = std::string(text);
            }
            Label(const std::string & text, bool selectable = false): selectable(selectable) {
                this->text = text;
            }
            void update(BaseWidget * prev) {
                if(prev && prev->ui) {
                    ui = prev->ui;
                } else {
                    ui = new QLabel();
                }
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

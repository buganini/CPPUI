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
            Label(const char * text) {
                this->text = std::string(text);
            }
            Label(const std::string text) {
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
            }
        };
    }
    inline Qt::Label & Label(const char * text) {
        return *(new Qt::Label(text));
    }
    inline Qt::Label & Label(const std::string text) {
        return *(new Qt::Label(text));
    }
}

#endif

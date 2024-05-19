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
                ((QLabel *)ui)->setText(text.c_str());
            }
        };
    }
    Qt::Label & Label(const char * text) {
        return *(new Qt::Label(text));
    }
    Qt::Label & Label(const std::string text) {
        return *(new Qt::Label(text));
    }
}

#include "base.hpp"

#include <QLabel>

namespace CPPUI {
    namespace Qt {
        class Label: public Node {
            public:
            const char * text;
            Label(const char * text) {
                this->text = text;
            }
            void update(Node * prev) {
                if(prev && prev->ui) {
                    ui = prev->ui;
                } else {
                    ui = new QLabel();
                }
                ((QLabel *)ui)->setText(text);
                Node::update(prev);
            }
        };
    }
    Qt::Label & Label(const char * text) {
        return *(new Qt::Label(text));
    }
}

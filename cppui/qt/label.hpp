#include "qtnode.hpp"

#include <QLabel>

namespace CPPUI {
    namespace Qt {
        class Label: public QtNode {
            public:
            const char * text;
            Label(const char * text) {
                this->text = text;
            }
            void update(View * prev) {
                if(!ui) {
                    ui = new QLabel();
                }
                ((QLabel *)ui)->setText(text);
                QtNode::update(prev);
            }
        };
    }
    Qt::Label & Label(const char * text) {
        return *(new Qt::Label(text));
    }
}

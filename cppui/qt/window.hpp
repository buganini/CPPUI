#include "qtnode.hpp"

#include <QMainWindow>

namespace CPPUI {
    namespace Qt {
        class Window: public QtNode {
            public:
            void update(Node * prev) {
                if(!ui) {
                    ui = new QMainWindow();
                }
                this->QtNode::update(prev);
            }

            void addChild(int idx, Node *child) {
                ((QMainWindow *)ui)->setCentralWidget((QWidget *) child->outer());
            }

            void removeChild(int idx, Node *child) {
                ((QWidget *)child->outer())->setParent(nullptr);
            }

        };
    }
    Qt::Window & Window() {
        return *(new Qt::Window());
    }
}

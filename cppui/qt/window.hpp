#include "base.hpp"

#include <QMainWindow>

namespace CPPUI {
    namespace Qt {
        class Window: public Node {
            public:
            void update(Node * prev) {
                if(prev && prev->ui) {
                    ui = prev->ui;
                } else {
                    ui = new QMainWindow();

                }
                Node::update(prev);
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

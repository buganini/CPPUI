#ifndef CPPUI_QT_WINDOW
#define CPPUI_QT_WINDOW

#include "base.hpp"

#include <QMainWindow>

namespace CPPUI {
    namespace Qt {
        class Window: public BaseWidget {
            public:
            void update(BaseWidget * prev) {
                if(prev && prev->ui) {
                    ui = prev->ui;
                } else {
                    ui = new QMainWindow();

                }
            }

            void addChild(int idx, Node *child) override {
                ((QMainWindow *)ui)->setCentralWidget((QWidget *) child->outer());
            }

            void removeChild(int idx, Node *child) override {
                ((QWidget *)child->outer())->setParent(nullptr);
            }
       };
    }
    inline Qt::Window & Window() {
        return *(new Qt::Window());
    }
}

#endif

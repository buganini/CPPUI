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

            void addChild(int idx, BaseWidget *child) {
                ((QMainWindow *)ui)->setCentralWidget((QWidget *) child->outer());
            }

            void removeChild(int idx, BaseWidget *child) {
                ((QWidget *)child->outer())->setParent(nullptr);
            }
       };
    }
    Qt::Window & Window() {
        return *(new Qt::Window());
    }
}

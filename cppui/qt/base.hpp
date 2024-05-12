#include "../view.hpp"

#include <QWidget>
#include <QBoxLayout>

#ifndef CPPUI_QT_BASE
#define CPPUI_QT_BASE

namespace CPPUI {
    namespace Qt {
        class BaseLayout: public Node {
            public:
            QBoxLayout * layout = nullptr;

            void update(Node * prev) {
                update((BaseLayout *) prev);
            }

            virtual void update(BaseLayout * prev) = 0;

            void * inner(void) {
                return layout;
            }

            void * outer(void) {
                return ui;
            }

            void addChild(int idx, Node *child) {
                layout->insertWidget(idx, (QWidget *) child->outer());
            }

            void removeChild(int idx, Node *child) {
                ((QWidget *)child->outer())->setParent(nullptr);
            }

        };
    }
}

#endif
#include "../view.hpp"

#include <QWidget>
#include <QBoxLayout>

#ifndef CPPUI_QT_BASE
#define CPPUI_QT_BASE

namespace CPPUI {
    namespace Qt {
        class QtView: public View {
            public:
            void destroy(bool direct) {
                if(direct && ui){
                    ((QWidget *)ui)->deleteLater();
                }
            }
         };

        class BaseWidget: public Node {
            public:
            void update(Node * prev) {
                update((BaseWidget *) prev);
            }

            virtual void update(BaseWidget * prev) = 0;
            virtual void addChild(int idx, BaseWidget * child) {}
            virtual void removeChild(int idx, BaseWidget * child) {}

            void addChild(int idx, Node *child) {
                addChild(idx, (BaseWidget *) child);
            }

            void removeChild(int idx, Node *child) {
                removeChild(idx, (BaseWidget *) child);
            }

            void destroy(bool direct) {
                if(direct){
                    ((QWidget *)ui)->deleteLater();
                }
            }
         };

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

            void destroy(bool direct) {
                if(direct){
                    ((QWidget *)ui)->deleteLater();
                }
            }
        };
    }
}

#endif
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
        };
    }
    Qt::Window & Window() {
        return *(new Qt::Window());
    }
}

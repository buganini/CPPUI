#include "base.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>

namespace CPPUI {
    namespace Qt {
        class HBox: public BaseLayout {
            public:
            void update(BaseLayout * prev) {
                if(prev && prev->ui) {
                    ui = prev->ui;
                    layout = prev->layout;
                } else {
                    ui = new QWidget();
                    layout = new QHBoxLayout();
                    ((QWidget *)ui)->setLayout(layout);
                }
            }
        };

        class VBox: public BaseLayout {
            public:
            void update(BaseLayout * prev) {
                if(prev && prev->ui) {
                    ui = prev->ui;
                    layout = prev->layout;
                } else {
                    ui = new QWidget();
                    layout = new QVBoxLayout();
                    ((QWidget *)ui)->setLayout(layout);
                }
            }
        };
    }
    Qt::HBox & HBox() {
        return *(new Qt::HBox());
    }
    Qt::VBox & VBox() {
        return *(new Qt::VBox());
    }
}

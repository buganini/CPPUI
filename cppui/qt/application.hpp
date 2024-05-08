#include "qtview.hpp"

#include <iostream>
#include <QApplication>

namespace CPPUI {
    class Application: public QtView {
        int argc;
        char **argv;

        public:
        Application(int argc, char *argv[]): QtView() {
            this->argc = argc;
            this->argv = argv;
        }

        void update(View * prev) {
            if(!ui) {
                ui = new QApplication(argc, argv);
            }
            QtView::update(prev);
        }

        void start(void) {
            std::cout << "start\n";
            ((QApplication *)ui)->exec();
        }
    };
}

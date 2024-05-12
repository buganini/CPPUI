#include "base.hpp"

#include <iostream>
#include <QMainWindow>
#include <QApplication>

namespace CPPUI {
    class Application: public View {
        int argc;
        char **argv;

        public:
        Application(int argc, char *argv[]): View() {
            this->argc = argc;
            this->argv = argv;
        }

        void update(Node * prev) {
            if(!ui) {
                ui = new QApplication(argc, argv);
            }
            View::update(prev);
        }

        void addChild(int idx, Node *child) {
            ((QMainWindow *)child->outer())->show();
        }

        void removeChild(int idx, Node *child) {
            ((QMainWindow *)child->outer())->close();
        }


        void start(void) {
            std::cout << "start\n";
            ((QApplication *)ui)->exec();
        }
    };
}

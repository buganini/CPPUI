#include <iostream>

#include "node.hpp"

#ifndef CPPUI_VIEW
#define CPPUI_VIEW

namespace CPPUI {
    class View: public Node {
        public:
        virtual void content(void) = 0;
        void start(void){};

        void update(View * prev) {
            content();
        }


        void redraw(void) {
            std::cout << "redraw\n";
            update(nullptr);
        }

        void run(void) {
            std::cout << "run\n";
            redraw();
            start();
            std::cout << "run end\n";
        }
    };
}

#endif
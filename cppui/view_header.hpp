#include <iostream>
#include <vector>

#ifndef CPPUI_VIEW_HEADER
#define CPPUI_VIEW_HEADER

#include "node_header.hpp"

namespace CPPUI {
    class View: public Node {
        public:
        virtual void content(void) = 0;
        void start(void){};

        std::vector<Node *> frames = std::vector<Node *>();

        View & tag(const char * name) {
            _tag = name;
            return *this;
        }

        void update(View * prev);
        void enter(void);
        void exit(void);
        void redraw(void);
        void run(void);
    };
}

#endif
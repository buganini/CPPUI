#include <iostream>
#include <vector>

#ifndef CPPUI_VIEW_HEADER
#define CPPUI_VIEW_HEADER

#include "node_header.hpp"

namespace CPPUI {
    class View: public Node {
        public:
        virtual void content(void) = 0;
        virtual void start(void){};

        std::vector<Node *> frames = std::vector<Node *>();

        View & id(const char * name) {
            _id = name;
            return *this;
        }

        virtual void enter(void);
        virtual void exit(void);

        virtual void update(Node * prev);
        virtual void redraw(void);
        virtual void run(void);
    };
}

#endif
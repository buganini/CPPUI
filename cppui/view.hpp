#include <iostream>

#ifndef CPPUI_VIEW
#define CPPUI_VIEW

#include "view_header.hpp"
#include "node.hpp"

namespace CPPUI {
    void View::update(View * prev) {
        enter();
        content();
        exit();
    }

    void View::enter(void) {
        std::vector<View *> * stack = getStack();
        stack->push_back(this);
        cppui_root->frames.push_back(this);
    }

    void View::exit(void) {
        std::vector<View *> * stack = getStack();
        stack->pop_back();
        cppui_root->frames.pop_back();
    }

    void View::redraw(void) {
        update(nullptr);
    }

    void View::run(void) {
        redraw();
        start();
    }
}

#endif
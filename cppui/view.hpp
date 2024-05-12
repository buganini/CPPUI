#include <iostream>

#ifndef CPPUI_VIEW
#define CPPUI_VIEW

#include "view_header.hpp"
#include "node.hpp"
#include "dom.hpp"

namespace CPPUI {
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

    void View::update(Node * prev) {
        std::cout << "update " << typeid(*this).name() << std::endl;
        if(retired_by) {
            return;
        }
        if(destroyed){
            return;
        }
        if(prev) {
            this->children = prev->children;
            prev->children = nullptr;
            prev->retired_by = this;
        }

        std::vector<Node *> * last_children = this->children;
        this->children = new std::vector<Node *>();
        enter();
        content();
        exit();

        sync(*this, last_children, this->children);
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
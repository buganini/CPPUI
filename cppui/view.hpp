#include <iostream>
#include <algorithm>

#ifndef CPPUI_VIEW
#define CPPUI_VIEW

#include "view_header.hpp"
#include "node.hpp"
#include "dom.hpp"

namespace CPPUI {
    inline View::View(): Node() {
        is_view = true;
        is_virtual = true;
        cppui_root = this;
    }

    inline View::~View() {
        std::vector<StateBase *> subscriptions(stateSubscriptions);
        stateSubscriptions.clear();
        for(StateBase * state: subscriptions) {
            state->removeListener(this);
        }
    }

    inline void View::trackState(StateBase * state) {
        if(std::find(stateSubscriptions.begin(), stateSubscriptions.end(), state)
           == stateSubscriptions.end()) {
            stateSubscriptions.push_back(state);
        }
    }

    inline void View::untrackState(StateBase * state) {
        stateSubscriptions.erase(
            std::remove(stateSubscriptions.begin(), stateSubscriptions.end(), state),
            stateSubscriptions.end()
        );
    }

    inline void View::enter(void) {
        std::vector<View *> * stack = getStack();
        stack->push_back(this);
        cppui_root->frames.push_back(this);
    }

    inline void View::exit(void) {
        std::vector<View *> * stack = getStack();
        stack->pop_back();
        cppui_root->frames.pop_back();
    }

    inline void View::update(Node * prev) {
        std::cout << "update " << typeid(*this).name() << std::endl;
        if(retired_by) {
            return;
        }
        if(destroyed){
            return;
        }
        if(prev) {
            prev->retired_by = this;
        }

        enter();
        content();
        exit();
    }

    inline void View::redraw(void) {
        if(retired_by || destroyed) {
            return;
        }

        Node * physicalParent = (dom_parent ? dom_parent : this)->getNode();
        int offset = 0;
        physicalParent->findDomOffsetForNode(this, &offset);

        std::vector<Node *> * lastChildren = children;
        children = new std::vector<Node *>();
        update(nullptr);

        int num = 0;
        int delta = 0;
        sync(this, physicalParent, offset, lastChildren, children, 0, &num, &delta);
        delete lastChildren;
    }

    inline void View::run(void) {
        redraw();
        start();
    }

    template <typename T>
    T & SubView(void) {
        return *(new T());
    }
}

#endif

#include <thread>
#include <vector>
#include <iterator>

#ifndef CPPUI_NODE_HEADER
#define CPPUI_NODE_HEADER

#include "view_header.hpp"

namespace CPPUI {
    class Node;
    class View;

    typedef void (*OnClickedListener) (void);

    class NodeIterator {
        public:
        Node * node;
        bool inContext;
        NodeIterator(Node *node);
        void operator++();
        bool operator!=(NodeIterator &obj);
        Node * operator*(void);
    };

    static inline std::vector<View *> * getStack() {
        thread_local std::vector<View *> stack;
        return &stack;
    }

    class Node {
        public:

        View * cppui_root = nullptr;
        Node * parent = nullptr;

        bool destroyed = false;
        Node * retired_by = nullptr;
        int debug = 0;
        const char * _tag = nullptr;

        void * ui;
        NodeIterator iter = NodeIterator(nullptr);

        std::vector<Node *> children;
        OnClickedListener onClicked = nullptr;

        Node();
        ~Node();

        NodeIterator & begin();
        NodeIterator & end();
        Node & tag(const char * name) {
            _tag = name;
            return *this;
        }
        void enter(void);
        void exit(void);
        void update(View * prev);
        void addChild(int idx, View *child);
        void removeChild(int idx, View *child);
        virtual void printUI(int indent);
    };
}

#endif
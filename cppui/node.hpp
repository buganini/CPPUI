#include <thread>
#include <vector>
#include <iterator>

#ifndef CPPUI_NODE
#define CPPUI_NODE

using namespace std;

namespace CPPUI {
    class View;
    class Node;
    class NodeIterator {
        public:
        Node * node;
        bool inContext;
        NodeIterator(Node *node) {
            inContext = true;
            this->node = node;
        }
        void operator++();

        bool operator!=(NodeIterator &obj) {
            return inContext;
        }

        Node * operator*(void) {
            return node;
        }
    };

    typedef void (*OnClickedListener) (void);

    class Node {
        public:

        static thread_local vector<Node> stack;
        bool destroyed = false;
        Node * retired_by = nullptr;
        int debug = 0;
        string tag = "";

        void * ui;
        NodeIterator iter = NodeIterator(nullptr);

        OnClickedListener onClicked = nullptr;

        NodeIterator &begin() {
            iter = NodeIterator(this);
            cout << "enter\n";
            return iter;
        }
        NodeIterator &end() { return iter; }

        void exit(void){
            cout << "exit\n";
        }

        void update(View * prev) {

        }

        void addChild(int idx, View *child) {}
        void removeChild(int idx, View *child) {}
    };


    void NodeIterator::operator++()
    {
        inContext = false;
        node->exit();
    }
}

#endif
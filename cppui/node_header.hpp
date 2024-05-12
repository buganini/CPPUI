#include <thread>
#include <vector>
#include <iterator>

#ifndef CPPUI_NODE_HEADER
#define CPPUI_NODE_HEADER

#include "context.hpp"
#include "view_header.hpp"

namespace CPPUI {
    class Node;
    class View;

    typedef void (*OnClickedListener) (void);

    static inline std::vector<View *> * getStack() {
        thread_local std::vector<View *> stack;
        return &stack;
    }

    class Node: public Context {
        public:

        View * cppui_root = nullptr;
        Node * parent = nullptr;

        bool terminal = false;
        bool destroyed = false;
        Node * retired_by = nullptr;
        int debug = 0;
        const char * _tag = nullptr;

        void * ui;

        std::vector<Node *> * children = nullptr;
        OnClickedListener onClicked = nullptr;

        Node();
        ~Node();

        Node & tag(const char * name) {
            _tag = name;
            return *this;
        }
        virtual std::size_t typecode() const;
        bool operator==(Node &obj);
        virtual void enter(void);
        virtual void exit(void);

        virtual void update(Node * prev);
        virtual void postUpdate(void);
        virtual void preSync(void);
        virtual void postSync(void);

        virtual void * inner(void);
        virtual void * outer(void);

        virtual void addChild(int idx, Node *child);
        virtual void removeChild(int idx, Node *child);

        virtual void click(OnClickedListener callback);

        virtual void writeStream(std::ostream & out, int indent) const;
    };
}

#endif
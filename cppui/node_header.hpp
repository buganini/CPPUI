#include <functional>
#include <iosfwd>
#include <typeinfo>
#include <thread>
#include <vector>
#include <iterator>

#ifndef CPPUI_NODE_HEADER
#define CPPUI_NODE_HEADER

#include "context.hpp"

namespace CPPUI {
    class Node;
    class View;

    // External inline linkage shares this thread-local stack across source files.
    inline std::vector<View *> * getStack() {
        thread_local std::vector<View *> stack;
        return &stack;
    }

    class Node: public Context {
        public:

        View * cppui_root = nullptr;
        Node * parent = nullptr;

        int classtype = 0;

        bool is_terminal = false;
        bool is_outoforder = false;
        bool is_reversed_order = false;
        bool is_view = false;
        bool is_virtual = false;
        bool is_movable = true;
        bool is_grid_layout = false;

        int grid_row = -1;
        int grid_column = -1;
        int grid_rowspan = -1;
        int grid_columnspan = -1;

        Node * dom_parent = nullptr;
        bool destroyed = false;
        Node * retired_by = nullptr;
        int debug = 0;
        const char * _id = nullptr;

        void * ui = nullptr;

        std::vector<Node *> * children = nullptr;
        std::function<void()> onClicked = [](){};

        Node();
        virtual ~Node();

        Node & id(const char * name) {
            _id = name;
            return *this;
        }
        virtual std::size_t typecode() const;
        bool operator==(Node &obj);
        virtual void enter(void);
        virtual void exit(void);

        virtual void update(Node * prev);
        virtual void releaseRetiredRefs(void);
        virtual void postUpdate(void);
        virtual void preSync(void);
        virtual void postSync(void);
        virtual void destroy(bool direct) = 0;

        virtual void * inner(void);
        virtual void * outer(void);

        virtual void addChild(int idx, Node *child);
        virtual void removeChild(int idx, Node *child);

        Node * getNode(void);
        bool findDomOffsetForNode(Node * node, int * offset);

        virtual void click(std::function<void()> callback);

        virtual void writeStream(std::ostream & out, int indent) const;
    };
}

#endif

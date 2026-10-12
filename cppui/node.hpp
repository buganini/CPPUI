#include <thread>
#include <vector>
#include <iterator>
#include <cstring>


#ifndef CPPUI_NODE
#define CPPUI_NODE

#include "view_header.hpp"

namespace CPPUI {
    class Node;
    class View;

    inline std::size_t Node::typecode() const {
        return typeid(*this).hash_code();
    }

    inline bool Node::operator==(Node &obj) {
        if(typecode() != obj.typecode()) {
            return false;
        }
        if(_id || obj._id) {
            return _id && obj._id && strcmp(_id, obj._id) == 0;
        }
        return true;
    }

    inline Node::Node() {
        std::vector<View *> * stack = getStack();
        children = new std::vector<Node *>();
        if(stack->size()==0){
            // View establishes its root after the Node base is constructed.
            // Standalone non-view nodes have no construction root.
            parent = this;
        }else{
            cppui_root = stack->back();
            parent = cppui_root->frames.back();
            parent->children->push_back(this);
        }
    }

    inline Node::~Node() {
        delete children;
    }

    inline void Node::enter(void) {
        cppui_root->frames.push_back(this);
    }

    inline void Node::exit(void) {
        cppui_root->frames.pop_back();
    }


    inline void Node::update(Node * prev) {
        if(prev) {
            prev->retired_by = this;
        }
    }

    inline void Node::releaseRetiredRefs(void) {
        cppui_root = nullptr;
        parent = nullptr;
        dom_parent = nullptr;
        ui = nullptr;
        onClicked = [](){};
        delete children;
        children = nullptr;
    }

    inline void Node::postUpdate(void) {

    }

    inline void Node::preSync(void) {

    }

    inline void Node::postSync(void) {

    }

    inline void * Node::inner(void) {
        if(ui){
            return ui;
        }
        return parent->inner();
    }

    inline void * Node::outer(void) {
        if(ui) {
            return ui;
        }
        if(children->size()) {
            return children->at(0)->outer();
        }
        return nullptr;
    }

    inline void Node::addChild(int idx, Node *child) {}
    inline void Node::removeChild(int idx, Node *child) {}

    inline Node * Node::getNode(void) {
        Node * node = this;
        while(node->retired_by) {
            node = node->retired_by;
        }
        if(node != this) {
            retired_by = node;
        }
        return node;
    }

    inline bool Node::findDomOffsetForNode(Node * node, int * offset) {
        if(this == node) {
            *offset = 0;
            return true;
        }

        int current = 0;
        for(Node * child: *children) {
            if(child == node) {
                *offset = current;
                return true;
            }
            if(child->is_virtual) {
                int nested = 0;
                bool found = child->findDomOffsetForNode(node, &nested);
                current += nested;
                if(found) {
                    *offset = current;
                    return true;
                }
            } else if(!child->is_outoforder) {
                current += 1;
            }
        }
        *offset = current;
        return false;
    }

    inline void Node::click(std::function<void()> callback) {
        onClicked = callback;
    }

    inline void Node::writeStream(std::ostream & out, int indent) const {
        for(int i=0;i<indent;i++){
            out << "  ";
        }
        out << typeid(*this).name();
        if(_id){
            out << "#" << _id;
        }
        out << " {" << std::endl;

        for(Node *child: *children){
            child->writeStream(out, indent+1);
        }

        for(int i=0;i<indent;i++){
            out << "  ";
        }
        out << "}" << std::endl;
    }

    inline std::ostream & operator<<(std::ostream & out, Node const & node) {
        node.writeStream(out, 0);
        return out;
    }
}

#endif

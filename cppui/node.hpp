#include <thread>
#include <vector>
#include <iterator>
#include <cstring>


#ifndef CPPUI_NODE
#define CPPUI_NODE

#include "node_header.hpp"

namespace CPPUI {
    class Node;
    class View;

    std::size_t Node::typecode() const {
        return typeid(*this).hash_code();
    }

    bool Node::operator==(Node &obj) {
        if(typecode() != obj.typecode()) {
            return false;
        }
        if(_id || obj._id) {
            return _id && obj._id && strcmp(_id, obj._id) == 0;
        }
        return true;
    }

    Node::Node() {
        std::vector<View *> * stack = getStack();
        children = new std::vector<Node *>();
        if(stack->size()==0){
            cppui_root = (View *)this;
            parent = this;
        }else{
            cppui_root = stack->back();
            parent = cppui_root->frames.back();
            parent->children->push_back(this);
        }
    }

    Node::~Node() {
        delete children;
    }

    void Node::enter(void) {
        cppui_root->frames.push_back(this);
    }

    void Node::exit(void) {
        cppui_root->frames.pop_back();
    }


    void Node::update(Node * prev) {
        if(prev) {
            prev->retired_by = this;
        }
    }

    void Node::releaseRetiredRefs(void) {
        cppui_root = nullptr;
        parent = nullptr;
        dom_parent = nullptr;
        ui = nullptr;
        onClicked = [](){};
        delete children;
        children = nullptr;
    }

    void Node::postUpdate(void) {

    }

    void Node::preSync(void) {

    }

    void Node::postSync(void) {

    }

    void * Node::inner(void) {
        if(ui){
            return ui;
        }
        return parent->inner();
    }

    void * Node::outer(void) {
        if(ui) {
            return ui;
        }
        if(children->size()) {
            return children->at(0)->outer();
        }
        return nullptr;
    }

    void Node::addChild(int idx, Node *child) {}
    void Node::removeChild(int idx, Node *child) {}

    Node * Node::getNode(void) {
        Node * node = this;
        while(node->retired_by) {
            node = node->retired_by;
        }
        if(node != this) {
            retired_by = node;
        }
        return node;
    }

    bool Node::findDomOffsetForNode(Node * node, int * offset) {
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

    void Node::click(std::function<void()> callback) {
        onClicked = callback;
    }

    void Node::writeStream(std::ostream & out, int indent) const {
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

    std::ostream & operator<<(std::ostream & out, Node const & node) {
        node.writeStream(out, 0);
        return out;
    }
}

#endif

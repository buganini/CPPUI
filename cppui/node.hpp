#include <thread>
#include <vector>
#include <iterator>


#ifndef CPPUI_NODE
#define CPPUI_NODE

#include "node_header.hpp"

namespace CPPUI {
    class Node;
    class View;

    void Node::enter(void) {
        cppui_root->frames.push_back(this);
    }

    void Node::exit(void) {
        cppui_root->frames.pop_back();
    }

    void Node::update(View * prev) {

    }

    void Node::addChild(int idx, View *child) {}
    void Node::removeChild(int idx, View *child) {}

    Node::Node() {
        std::vector<View *> * stack = getStack();
        if(stack->size()==0){
            cppui_root = (View *)this;
            parent = this;
        }else{
            cppui_root = stack->back();
            parent = cppui_root->frames.back();
            parent->children.push_back(this);
        }
    }

    Node::~Node() {

    }

    void Node::writeStream(std::ostream & out, int indent) const {
        for(int i=0;i<indent;i++){
            out << "  ";
        }
        out << typeid(*this).name();
        if(_tag){
            out << " (" << _tag << ")";
        }
        out << " {" << std::endl;

        for(Node *child: children){
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
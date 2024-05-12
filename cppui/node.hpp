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

    void Node::printUI(int indent) {
        for(int i=0;i<indent;i++){
            std::cout << "  ";
        }
        std::cout << typeid(*this).name();
        if(_tag){
            std::cout << " (" << _tag << ")";
        }
        std::cout << " {" << std::endl;

        for(Node *child: children){
            child->printUI(indent+1);
        }

        for(int i=0;i<indent;i++){
            std::cout << "  ";
        }
        std::cout << "}" << std::endl;
    }

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
}

#endif
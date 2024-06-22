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
        if(_id && obj._id && strcmp(_id, obj._id)){
            return false;
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
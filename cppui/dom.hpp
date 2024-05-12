#include <iostream>
#include <vector>

#ifndef CPPUI_DOM
#define CPPUI_DOM

#include "node_header.hpp"

namespace CPPUI {
    static inline int indexOfSince(std::vector<Node *> * dom, int since, Node * node) {
        for(int i=since;i<dom->size();i+=1){
            if(*dom->at(i) == *node){
                return i;
            }
        }
        return -1;
    }

    static inline int indexOf(std::vector<Node *> * dom, Node * node) {
        return indexOfSince(dom, 0, node);
    }


    void sync(Node & node, std::vector<Node *> * oldDOM, std::vector<Node *> * newDOM) {
        std::cout << "sync " << typeid(node).name() << std::endl;

        node.preSync();

        std::vector<Node *> tbd;
        for (int i = 0; i < newDOM->size(); i+=1) {
            Node * newNode = newDOM->at(i);
            while(1) {
                // Step 1. skip common prefix
                if(i < oldDOM->size()) {
                    Node * oldNode = oldDOM->at(i);
                    if(*oldNode == *newNode){
                        std::cout << "matched, update " << typeid(*newNode).name() << std::endl;
                        newNode->update(oldNode);

                        if(!newNode->terminal){
                            sync(*newNode, oldNode->children, newNode->children);
                        }
                    }
                    break;
                }

                bool trimmed = false;

                // Step 2. trim removed nodes after common prefix
                while(i < oldDOM->size() && indexOf(newDOM, oldDOM->at(i))==-1){
                    Node * oldNode = oldDOM->at(i);
                    std::cout << "TRIM " << i << " " << typeid(*oldNode).name() << std::endl;
                    oldDOM->erase(oldDOM->begin()+i);
                    node.removeChild(i, oldNode);
                    tbd.push_back(oldNode);
                    trimmed = true;
                }

                if(trimmed) {
                    continue;
                }

                // Step 3. setup target node
                int idx = indexOfSince(oldDOM, i+1, newNode);

                // Step 3-1. new node
                if(idx == -1) {
                    std::cout << "new node " << typeid(*newNode).name() << std::endl;
                    newNode->update(nullptr);
                    if(!newNode->terminal){
                        std::vector<Node *> empty;
                        sync(*newNode, &empty, newNode->children);
                    }
                    node.addChild(i, newNode);
                    oldDOM->insert(oldDOM->begin()+i, newNode);

                }
                // Step 3-2. existed node
                else {
                    // if the target node is in just next position, requeue the front to prevent repositioning every nodes coming after
                    // eg. when an element is removed from a long list, do single 3-2-1 instead of many 3-2-2

                    // Step 3-2-1. yield the next position for the target node
                    if(idx == i+1) {
                        Node * oldNode = oldDOM->at(i);
                        oldDOM->erase(oldDOM->begin()+i);
                        node.removeChild(i, oldNode);

                        node.addChild(oldDOM->size(), oldNode);
                        oldDOM->push_back(oldNode);

                        // sync will be peformed in next step 1
                        continue;
                    }

                    // Step 3-2-2. move target node
                    else {
                        Node * oldNode = oldDOM->at(idx);
                        oldDOM->erase(oldDOM->begin()+idx);
                        node.removeChild(idx, oldNode);

                        newNode->update(oldNode);
                        if(!newNode->terminal) {
                            sync(*newNode, oldNode->children, newNode->children);
                        }

                        node.addChild(i, newNode);
                        oldDOM->insert(oldDOM->begin()+i, newNode);
                    }
                }

                break;
            }
        }
        // Step 4. trim removed trail
        int nl = newDOM->size();
        while(oldDOM->size() > nl) {
            Node * oldNode = oldDOM->back();
            oldDOM->pop_back();
            node.removeChild(oldDOM->size(), oldNode);
            tbd.push_back(oldNode);
        }

        for(Node *n: *newDOM) {
            n->postUpdate();
        }

        node.postSync();

        // release deleted nodes
    }
}

#endif
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <tuple>
#include <vector>

#ifndef CPPUI_DOM
#define CPPUI_DOM

#define DEBUG_CPPUI_DOM 0

#include "node_header.hpp"

namespace CPPUI {
    class VDomError: public std::runtime_error {
        public:
        explicit VDomError(const std::string & message): std::runtime_error(message) {}
    };

    inline int indexOfSince(const std::vector<Node *> * dom, int since, Node * node) {
        for(int i = since; i < static_cast<int>(dom->size()); i += 1) {
            if(*dom->at(i) == *node) {
                return i;
            }
        }
        return -1;
    }

    inline void recur_delete(Node * child, bool direct) {
        child->clearRef();
        child->destroyed = true;
        for(Node * subchild: *child->children) {
            if(subchild) {
                // Virtual nodes have no native owner to delete descendants.
                // Carry direct ownership through them until a physical node.
                recur_delete(subchild, direct && child->is_virtual);
            }
        }
        child->children->clear();
        child->destroy(direct);
        delete child;
    }

    inline std::vector<Node *> dom_remove_node(Node * dom_parent, int dom_offset, Node * child) {
        std::vector<Node *> removed(1, child);
        if(child->is_virtual) {
            for(Node * subchild: *child->children) {
                std::vector<Node *> descendants = dom_remove_node(dom_parent, dom_offset, subchild);
                removed.insert(removed.end(), descendants.begin(), descendants.end());
            }
        } else {
            dom_parent->removeChild(dom_offset, child);
        }
        return removed;
    }

    inline void dom_add_nodes(Node * dom_parent, int dom_offset, const std::vector<Node *> & children) {
        int childIdx = 0;
        for(Node * child: children) {
            if(!child->is_virtual) {
                dom_parent->addChild(dom_offset + childIdx, child);
                if(!child->is_outoforder) childIdx += 1;
            }
        }
    }

    inline int count_dom_children(const std::vector<Node *> * nodes) {
        int num = 0;
        for(Node * child: *nodes) {
            if(child->is_virtual) {
                num += count_dom_children(child->children);
            } else if(!child->is_outoforder) {
                num += 1;
            }
        }
        return num;
    }

    inline int count_ordered_nodes(const std::vector<Node *> & nodes) {
        int num = 0;
        for(Node * node: nodes) {
            if(!node->is_virtual && !node->is_outoforder) {
                num += 1;
            }
        }
        return num;
    }

    inline void rebind_dom_parent(Node * node, Node * dom_parent) {
        node->dom_parent = dom_parent;
        // Reused views keep their declarations. Virtual descendants still share
        // this physical parent, whose previous declaration will be reclaimed.
        // A physical child owns the parent relationship for its own subtree.
        if(node->is_virtual) {
            for(Node * child: *node->children) {
                rebind_dom_parent(child, dom_parent);
            }
        }
    }

    inline void prepare_dom(Node * node, std::vector<Node *> * dom) {
        if(node->is_grid_layout) {
            // Unplaced declarations never mount, but still require C++ teardown.
            std::vector<Node *> unplaced;
            for(auto child: *dom) {
                if(child->grid_row < 0 || child->grid_column < 0) unplaced.push_back(child);
            }
            dom->erase(
                std::remove_if(dom->begin(), dom->end(), [](Node * child) {
                    return child->grid_row < 0 || child->grid_column < 0;
                }),
                dom->end()
            );
            for(auto child: unplaced) recur_delete(child, true);
            std::stable_sort(dom->begin(), dom->end(), [](Node * lhs, Node * rhs) {
                return std::make_tuple(lhs->grid_row, lhs->grid_column, lhs->grid_rowspan, lhs->grid_columnspan)
                    < std::make_tuple(rhs->grid_row, rhs->grid_column, rhs->grid_rowspan, rhs->grid_columnspan);
            });
            return;
        }

        std::vector<Node *> ordered;
        std::vector<Node *> outoforder;
        for(Node * child: *dom) {
            (child->is_outoforder ? outoforder : ordered).push_back(child);
        }
        if(node->is_reversed_order) {
            std::reverse(ordered.begin(), ordered.end());
        }
        dom->clear();
        dom->insert(dom->end(), ordered.begin(), ordered.end());
        dom->insert(dom->end(), outoforder.begin(), outoforder.end());
    }

    inline void sync(
        Node * node,
        Node * dom_parent,
        int dom_offset,
        std::vector<Node *> * oldDOM,
        std::vector<Node *> * newDOM,
        int depth,
        int * result_num,
        int * result_delta
    ) {
        int orig_dom_children_num = count_dom_children(oldDOM);
        int dom_children_num = orig_dom_children_num;
        int dom_children_curr = 0;

        // The old DOM already records native mounting order from the last sync.
        // Reversing it again would make our offsets disagree with the backend.
        prepare_dom(node, newDOM);

#if DEBUG_CPPUI_DOM > 0
        std::cout << std::string(depth * 4, ' ') << "sync " << typeid(*node).name()
                  << " old=" << oldDOM->size() << " new=" << newDOM->size()
                  << " offset=" << dom_offset << std::endl;
#endif

        node->preSync();

        std::vector<Node *> toBeDeleted;
        std::vector<Node *> retired;
        for(int childIdx = 0; childIdx < static_cast<int>(newDOM->size()); childIdx += 1) {
            Node * newNode = newDOM->at(childIdx);
            newNode->dom_parent = dom_parent;

            while(true) {
                // Step 1: the current nodes match.
                if(childIdx < static_cast<int>(oldDOM->size())) {
                    Node * oldNode = oldDOM->at(childIdx);
                    if(*oldNode == *newNode) {
                        if(oldNode->is_view) {
                            dom_children_curr += count_dom_children(oldNode->children);
                            oldNode->parent = newNode->parent;
                            rebind_dom_parent(oldNode, newNode->dom_parent);
                            newDOM->at(childIdx) = oldNode;
                            newNode->destroy(true);
                            delete newNode;
                        } else {
                            newNode->update(oldNode);
                            newNode->publishRef();
                            if(newNode->is_virtual) {
                                int num = 0;
                                int delta = 0;
                                sync(newNode, dom_parent, dom_offset + dom_children_curr,
                                     oldNode->children, newNode->children, depth + 1, &num, &delta);
                                dom_children_curr += num;
                                dom_children_num += delta;
                            } else {
                                if(!newNode->is_outoforder) {
                                    dom_children_curr += 1;
                                }
                                if(!newNode->is_terminal) {
                                    int num = 0;
                                    int delta = 0;
                                    sync(newNode, newNode, 0, oldNode->children,
                                         newNode->children, depth + 1, &num, &delta);
                                } else if(oldNode->is_terminal) {
                                    // Terminal children are declarations only; no
                                    // native recursion will retire them for us.
                                    for(auto child: *oldNode->children) recur_delete(child, true);
                                    oldNode->children->clear();
                                } else {
                                    std::vector<Node *> empty;
                                    int num = 0, delta = 0;
                                    sync(newNode, newNode, 0, oldNode->children,
                                         &empty, depth + 1, &num, &delta);
                                }
                            }
                            oldNode->releaseRetiredRefs();
                            retired.push_back(oldNode);
                        }
                        break;
                    }
                }

                // Step 2: trim old nodes that do not occur in the remaining new DOM.
                bool trimmed = false;
                while(childIdx < static_cast<int>(oldDOM->size()) &&
                      indexOfSince(newDOM, childIdx, oldDOM->at(childIdx)) == -1) {
                    Node * oldNode = oldDOM->at(childIdx);
                    oldDOM->erase(oldDOM->begin() + childIdx);
                    std::vector<Node *> nodes = dom_remove_node(
                        dom_parent, dom_offset + dom_children_curr, oldNode
                    );
                    dom_children_num -= count_ordered_nodes(nodes);
                    toBeDeleted.push_back(oldNode);
                    trimmed = true;
                }
                if(trimmed) {
                    continue;
                }

                // Step 3: add the target or move an existing matching node.
                int matchedIdx = newNode->is_movable
                    ? indexOfSince(oldDOM, childIdx + 1, newNode)
                    : -1;

                if(matchedIdx == -1) {
                    // Step 3-1: new node.
                    newNode->update(nullptr);
                    newNode->publishRef();
                    if(newNode->is_virtual) {
                        std::vector<Node *> empty;
                        int num = 0;
                        int delta = 0;
                        sync(newNode, dom_parent, dom_offset + dom_children_curr, &empty,
                             newNode->children, depth + 1, &num, &delta);
                        dom_children_curr += num;
                        dom_children_num += num;
                    } else {
                        dom_parent->addChild(dom_offset + dom_children_curr, newNode);
                        if(!newNode->is_outoforder) {
                            dom_children_curr += 1;
                            dom_children_num += 1;
                        }
                        if(!newNode->is_terminal) {
                            std::vector<Node *> empty;
                            int num = 0;
                            int delta = 0;
                            sync(newNode, newNode, 0, &empty, newNode->children,
                                 depth + 1, &num, &delta);
                        }
                    }
                    oldDOM->insert(oldDOM->begin() + childIdx, newNode);
                } else {
                    if(matchedIdx == childIdx + 1 && oldDOM->at(childIdx)->is_movable) {
                        // Step 3-2-1: yield the next position by requeueing its blocker.
                        Node * toBeRequeued = oldDOM->at(childIdx);
                        oldDOM->erase(oldDOM->begin() + childIdx);
                        std::vector<Node *> nodes = dom_remove_node(
                            dom_parent, dom_offset + dom_children_curr, toBeRequeued
                        );
                        int n = count_ordered_nodes(nodes);
                        dom_add_nodes(dom_parent, dom_offset + dom_children_num - n, nodes);
                        oldDOM->push_back(toBeRequeued);
                    } else {
                        // Step 3-2-2: move the target into the current position.
                        Node * oldNode = oldDOM->at(matchedIdx);
                        int offset = dom_offset;
                        for(int i = 0; i < matchedIdx; i += 1) {
                            std::vector<Node *> one(1, oldDOM->at(i));
                            offset += count_dom_children(&one);
                        }
                        oldDOM->erase(oldDOM->begin() + matchedIdx);
                        std::vector<Node *> nodes = dom_remove_node(dom_parent, offset, oldNode);
                        dom_add_nodes(dom_parent, dom_offset + dom_children_curr, nodes);
                        oldDOM->insert(oldDOM->begin() + childIdx, oldNode);
                    }
                    continue;
                }
                break;
            }
        }

        // Step 4: trim the old trail.
        int newLength = static_cast<int>(newDOM->size());
        while(static_cast<int>(oldDOM->size()) > newLength) {
            Node * oldNode = oldDOM->at(newLength);
            oldDOM->erase(oldDOM->begin() + newLength);
            std::vector<Node *> nodes = dom_remove_node(
                dom_parent, dom_offset + dom_children_curr, oldNode
            );
            dom_children_num -= count_ordered_nodes(nodes);
            toBeDeleted.push_back(oldNode);
        }

        for(Node * child: *newDOM) {
            child->postUpdate();
        }
        node->postSync();

        for(Node * oldNode: toBeDeleted) {
            recur_delete(oldNode, true);
        }
        for(Node * oldNode: retired) {
            delete oldNode;
        }

        if(dom_children_curr != dom_children_num) {
            throw VDomError("DOM child count mismatch");
        }
        *result_num = dom_children_curr;
        *result_delta = dom_children_num - orig_dom_children_num;
    }
}

#endif

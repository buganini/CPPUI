#include "cppui/node.hpp"
#include <stdexcept>
struct Node:CPPUI::Node {
    int x=1,y=1;
    bool axisX=false,axisY=false,boundary=false;
    int baseExpansion(bool h) const override { return h ? x : y; }
    bool expansionAxis(bool h) const override { return h ? axisX : axisY; }
    bool expansionBoundary(bool) const override { return boundary; }
    void destroy(bool) override {}
};
static void require(bool ok) { if(!ok) throw std::runtime_error("expansion contract"); }
static void append(Node & parent,Node & child) { parent.children->push_back(&child);child.parent=&parent; }
int main() {
    Node root,box,view,inner,leaf;
    root.axisX=true;append(root,box);append(box,view);view.is_virtual=true;
    append(view,inner);append(inner,leaf);leaf.x=4;leaf.y=0;
    require(box.expansionPriority(true)==2 && box.expansionPriority(false)==1);
    leaf.is_visible=false;require(box.expansionPriority(true)==1);
    leaf.is_visible=true;require(box.expansionPriority(true)==2);
    inner.children->clear();require(box.expansionPriority(true)==1);
    append(inner,leaf);inner.axisX=true;leaf.x=0;leaf.layout_weight=9;
    require(leaf.expansionPriority(true)==5 && box.expansionPriority(true)==2);
    box.layout_weight=0;require(box.expansionPriority(true)==0);box.layout_weight.reset();
    box.layout_width=100;require(box.expansionPriority(true)==1);box.layout_width.reset();
    box.boundary=true;require(box.expansionPriority(true)==1);box.boundary=false;
    box.is_terminal=true;require(box.expansionPriority(true)==1);box.is_terminal=false;
    for(int base:{3,4}) {box.x=base;require(box.expansionPriority(true)==base);}box.x=1;
    inner.layout_weight=0;require(box.expansionPriority(true)==2); // box is not an x container yet
    box.axisX=true;require(box.expansionPriority(true)==1); // virtual node must not break parent axes
    inner.layout_weight=1;require(box.expansionPriority(true)==2);
    view.is_visible=false;require(box.expansionPriority(true)==1);
    root.children->clear();box.children->clear();view.children->clear();inner.children->clear();
}

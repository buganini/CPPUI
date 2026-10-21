#include <algorithm>
#include <thread>
#include <vector>
#include <iterator>
#include <tuple>


#ifndef CPPUI_NODE
#define CPPUI_NODE

#include "view_header.hpp"

namespace CPPUI {
    class Node;
    class View;

    namespace detail {
        // Native geometry is read-only, so it needs dependency tracking without
        // exposing State's public mutation operators to callers.
        class SizeState: public StateBase {
            Optional<Size> value;
            std::vector<View *> listeners, pending;
        public:
            Node * owner=nullptr;
            ~SizeState() override {
                for(auto view:listeners) view->untrackState(this);
            }
            void removeListener(View * view) override {
                listeners.erase(std::remove(listeners.begin(),listeners.end(),view),listeners.end());
                pending.erase(std::remove(pending.begin(),pending.end(),view),pending.end());
            }
            Optional<Size> get() {
                if(!getStack()->empty()) {
                    auto view=getStack()->back();view->trackState(this);
                    if(std::find(listeners.begin(),listeners.end(),view)==listeners.end()) listeners.push_back(view);
                }
                return value;
            }
            void publish(int width,int height) {
                if(!owner || owner->destroyed) return;
                if(!getStack()->empty()) throw std::logic_error("Size must be published after view construction");
                Size next(std::max(0,width),std::max(0,height));
                if(value && *value==next) return;
                value=next;pending=listeners;
                while(!pending.empty()) {
                    auto view=pending.front();pending.erase(pending.begin());view->redraw();
                }
            }
        };
    }

    inline Optional<Size> SizeBinding::get() const { return state->get(); }

    inline Node * Node::previousSizeNode() {
        if(!cppui_root || !cppui_root->previousSizeChildren || this==cppui_root) return nullptr;
        const std::vector<Node *> * previous=nullptr;
        if(parent==cppui_root) previous=cppui_root->previousSizeChildren;
        else if(parent && parent!=this) {
            auto owner=parent->previousSizeNode();
            if(owner) previous=owner->children;
        }
        if(!previous) return nullptr;
        // Reconciliation pairs equal identities in order. Absolute sibling
        // indices change when an unrelated declaration is inserted/removed.
        std::size_t ordinal=0;
        for(auto sibling:*parent->children) {
            if(sibling==this) break;
            if(*sibling==*this) ++ordinal;
        }
        std::vector<Node *> candidates;
        for(auto old:*previous) if(*old==*this) candidates.push_back(old);
        // The mounted DOM may have been reversed or grid-sorted. Restore its
        // declaration order before pairing anonymous siblings.
        std::sort(candidates.begin(),candidates.end(),[](Node * a,Node * b) {
            return a->declarationIndex<b->declarationIndex;
        });
        return ordinal<candidates.size() ? candidates[ordinal] : nullptr;
    }

    inline SizeBinding Node::size() {
        if(!sizeState) {
            if(auto previous=previousSizeNode()) sizeState=previous->sizeState;
            if(!sizeState) sizeState=std::make_shared<detail::SizeState>();
        }
        if(sizeMounted) Node::postUpdate();
        return SizeBinding(sizeState);
    }

    namespace detail {
        inline bool adaptiveHorizontal(Node & node,int threshold,bool width) {
            auto owner=node.parent;
            while(owner && owner->is_virtual && owner->parent!=owner) owner=owner->parent;
            const auto size=owner && owner!=&node ? owner->size().get() : Optional<Size>();
            const bool primary=size && (width ? size->width : size->height)>=threshold;
            return width ? primary : !primary;
        }
    }

    inline bool Node::wideMatches(int minWidth) {
        return detail::adaptiveHorizontal(*this,minWidth,true);
    }

    inline Node & Node::visible(bool value,const VisibilityBounds & bounds) {
        if(bounds.constrained()) {
            auto owner=parent;
            while(owner && owner->is_virtual && owner->parent!=owner) owner=owner->parent;
            value=value && owner && owner!=this && bounds.matches(owner->size().get());
        }
        is_visible=value;is_frozen=supportsVisibility() && !value;
        return *this;
    }

    inline void Node::stopObservingSize() {
        sizeObserver.reset();
        if(sizeState && sizeState->owner==this) sizeState->owner=nullptr;
    }

    inline std::size_t Node::typecode() const {
        return typeid(*this).hash_code();
    }

    inline bool Node::operator==(Node &obj) {
        if(typecode() != obj.typecode() || refName != obj.refName) {
            return false;
        }
        const bool placed = grid_row >= 0 && grid_column >= 0;
        const bool otherPlaced = obj.grid_row >= 0 && obj.grid_column >= 0;
        if(placed != otherPlaced) return false;
        if(placed && std::make_tuple(grid_row, grid_column, grid_rowspan, grid_columnspan)
           != std::make_tuple(obj.grid_row, obj.grid_column, obj.grid_rowspan, obj.grid_columnspan)) return false;
        if(has_id || obj.has_id) {
            return has_id && obj.has_id && _id == obj._id;
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
            declarationIndex=parent->children->size();
            parent->children->push_back(this);
        }
    }

    inline Node::~Node() {
        stopObservingSize();
        clearRef();
        // A derived constructor can throw after Node registered itself with
        // its parent. Do not leave a pointer to that failed allocation behind.
        if(!destroyed && parent && parent != this && parent->children) {
            auto & siblings = *parent->children;
            siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());
        }
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
            if(prev->sizeState) sizeState=prev->sizeState;
            if(prev->sizeObserver && prev->outer()!=outer()) prev->stopObservingSize();
            sizeObserver=std::move(prev->sizeObserver);
            prev->retired_by = this;
            if(dom_parent && dom_parent!=this && !is_outoforder)
                dom_parent->getNode()->rebindChild(prev,this);
            if(prev->refTarget) { refTarget=prev->refTarget; std::lock_guard<std::mutex> lock(refTarget->mutex); refTarget->node=this; }
        }
        sizeMounted=true;
        if(sizeState) sizeState->owner=this;
    }

    inline void Node::publishRef() {
        if(refName.empty() || destroyed) return;
        if(!refTarget) refTarget=std::make_shared<detail::RefTarget>();
        {
            std::lock_guard<std::mutex> lock(refTarget->mutex);
            refTarget->node=this; refTarget->post=commandPoster();
            refTarget->selection=refSelection(); refTarget->emptySelection=refEmptySelection(); refTarget->window=refWindow();
            refTarget->invoke=[this](RefCommand cmd,const OptionalText & key,bool value) { refCommand(cmd,key,value); };
        }
        detail::refRegistry()[refName]=refTarget;
    }
    inline Poster Node::getPoster() {
        if(std::this_thread::get_id()!=constructionThread)
            throw std::logic_error("getPoster must be called on the UI thread");
        if(destroyed || retired_by) throw std::logic_error("getPoster requires a current mounted node");
        auto post=commandPoster(); auto later=timerPoster();
        if(!post || !later) throw std::logic_error("This node has no mounted UI dispatcher");
        if(!refTarget) refTarget=std::make_shared<detail::RefTarget>();
        { std::lock_guard<std::mutex> lock(refTarget->mutex);
          refTarget->node=this; refTarget->post=std::move(post); refTarget->later=std::move(later); }
        std::weak_ptr<detail::RefTarget> weak=refTarget;
        auto guard=[weak](Callback callback) -> Callback {
            return [weak,callback]() {
                auto target=weak.lock(); if(!target) return;
                { std::lock_guard<std::mutex> lock(target->mutex); if(!target->node) return; }
                callback(); // UI thread; callbacks may remove their own node.
            };
        };
        return Poster([weak,guard](Callback callback) {
            auto target=weak.lock(); if(!target) return;
            std::lock_guard<std::mutex> lock(target->mutex);
            if(target->node) target->post(guard(std::move(callback)));
        },[weak,guard](int delay,Callback callback) {
            auto target=weak.lock(); if(!target) return;
            std::lock_guard<std::mutex> lock(target->mutex);
            if(target->node) target->later(delay,guard(std::move(callback)));
        });
    }
    inline void Node::clearRef() {
        if(!refTarget) return;
        std::lock_guard<std::mutex> lock(refTarget->mutex);
        if(refTarget->node!=this) return;
        refTarget->node=nullptr; refTarget->post={}; refTarget->later={}; refTarget->invoke={};
        auto & registry=detail::refRegistry(); auto it=registry.find(refName);
        if(it!=registry.end() && it->second==refTarget) registry.erase(it);
    }

    inline void Node::prepare(void) {
        if(is_frozen) return;
        for(auto child: *children) child->prepare();
    }

    inline void Node::releaseRetiredRefs(void) {
        domOffsetVersion.reset(); cachedDomOffsetVersion.reset();
        cppui_root = nullptr;
        parent = nullptr;
        dom_parent = nullptr;
        ui = nullptr;
        onClicked = [](){};
        dropHandlers={};
        delete children;
        children = nullptr;
    }

    inline void Node::postUpdate(void) {
        if(sizeState && !destroyed && !retired_by) {
            sizeState->owner=this;
            if(!sizeObserver) sizeObserver=observeSize();
            if(sizeObserver) sizeObserver->refresh();
        }
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
        if(node && domOffsetVersion && node->cachedDomOffsetVersion==domOffsetVersion) {
            *offset=node->cachedDomOffset;
            return true;
        }
        const bool found=findDomOffsetUncached(node,offset);
        if(found) {
            if(!domOffsetVersion) domOffsetVersion=std::make_shared<const char>(0);
            node->cachedDomOffsetVersion=domOffsetVersion;
            node->cachedDomOffset=*offset;
        }
        return found;
    }

    inline bool Node::findDomOffsetUncached(Node * node, int * offset) {
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
                bool found = child->findDomOffsetUncached(node, &nested);
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
        if(has_id){
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

// Standalone users also need View's inline virtual definitions (and RTTI for
// checked/sanitized builds). view.hpp's guard breaks the normal reverse include.
#include "view.hpp"

#endif

#ifndef CPPUI_REFS
#define CPPUI_REFS

#include "optional.hpp"
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>

namespace CPPUI {
    class Node;
    enum class RefCommand { Select, Maximize, Minimize, Fullscreen };
    using CommandPoster = std::function<void(std::function<void()>)>;
    namespace detail {
        struct RefTarget {
            std::mutex mutex;
            Node * node = nullptr; // Read/dereferenced only on the UI thread.
            CommandPoster post;
            std::function<void(RefCommand, const OptionalText &, bool)> invoke;
            bool selection = false, emptySelection = false, window = false;
        };
        inline std::map<std::string,std::shared_ptr<RefTarget>> & refRegistry() {
            static std::map<std::string,std::shared_ptr<RefTarget>> registry;
            return registry;
        }
    }
    class NodeBinder {
        std::shared_ptr<detail::RefTarget> target;
        void schedule(RefCommand command, const OptionalText & key, bool value) const {
            auto state=target;
            std::lock_guard<std::mutex> lock(state->mutex);
            if(!state->node) throw std::runtime_error("The referenced node has been destroyed");
            if(command==RefCommand::Select) {
                if(!state->selection) throw std::logic_error("This ref does not support selection");
                if(key && key->empty() && !state->emptySelection) throw std::invalid_argument("Selection must be non-empty or absent");
            } else if(!state->window) throw std::logic_error("This ref does not support desktop window commands");
            if(!state->post) throw std::logic_error("This ref has no UI dispatcher");
            std::weak_ptr<detail::RefTarget> weak=state;
            state->post([weak,command,key,value]() {
                auto current=weak.lock(); if(!current) return;
                std::function<void(RefCommand,const OptionalText &,bool)> invoke;
                { std::lock_guard<std::mutex> lock(current->mutex); if(current->node) invoke=current->invoke; }
                // UI-thread execution; user callbacks may reconcile/delete the node.
                if(invoke) invoke(command,key,value);
            });
        }
        public:
        explicit NodeBinder(std::shared_ptr<detail::RefTarget> target):target(std::move(target)) {}
        Node * node() const { std::lock_guard<std::mutex> lock(target->mutex); return target->node; }
        const NodeBinder & select(OptionalText key = {}) const { schedule(RefCommand::Select,key,false); return *this; }
        const NodeBinder & select(const std::string & key) const { return select(OptionalText(key)); }
        const NodeBinder & select(const char * key) const { return select(key ? OptionalText(std::string(key)) : OptionalText()); }
        void maximize(bool value) const { schedule(RefCommand::Maximize,{},value); }
        void minimize(bool value) const { schedule(RefCommand::Minimize,{},value); }
        void fullscreen(bool value) const { schedule(RefCommand::Fullscreen,{},value); }
    };
    // Lookup is UI-thread-only. Copy a binder before handing it to a worker.
    inline NodeBinder ref(const std::string & name) { return NodeBinder(detail::refRegistry().at(name)); }
}
#endif

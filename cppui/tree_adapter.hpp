#ifndef CPPUI_TREE_ADAPTER
#define CPPUI_TREE_ADAPTER

#include <memory>
#include <string>

namespace CPPUI {
    // Handles refer to application data, not borrowed CPPUI declarations.
    // Null denotes the invisible root. Keep child ownership acyclic.
    template<class T> class BaseTreeAdapter {
        public:
        using Value = T;
        using Handle = std::shared_ptr<T>;
        virtual ~BaseTreeAdapter() = default;
        virtual Handle parent(const Handle & node) const = 0;
        virtual Handle child(const Handle & parent, int index) const = 0;
        virtual std::string data(const Handle & node) const = 0;
        virtual int rowCount(const Handle & parent) const = 0;
        virtual void clicked(const Handle &) {}
        virtual void dblclicked(const Handle &) {}
        virtual void expanded(const Handle &) {}
        virtual void collapsed(const Handle &) {}
    };
}
#endif

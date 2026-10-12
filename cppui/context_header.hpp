#include <thread>
#include <vector>
#include <iterator>

#ifndef CPPUI_CONTEXT_HEADER
#define CPPUI_CONTEXT_HEADER

namespace CPPUI {
    class Context;

    class ContextIterator {
        public:
        Context * context;
        bool inContext;
        explicit ContextIterator(Context *context = nullptr);
        ContextIterator(const ContextIterator &) = delete;
        ContextIterator & operator=(const ContextIterator &) = delete;
        ContextIterator(ContextIterator && other) noexcept;
        ~ContextIterator();
        void operator++();
        bool operator!=(const ContextIterator &obj) const;
        Context * operator*(void);
    };

    class Context {
        public:
        ContextIterator begin();
        ContextIterator end();
        virtual void enter(void) = 0;
        virtual void exit(void) = 0;
    };
}

#endif

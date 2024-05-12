#include <thread>
#include <vector>
#include <iterator>

#ifndef CPPUI_CONTEXT_HEADER
#define CPPUI_CONTEXT_HEADER

#include "view_header.hpp"

namespace CPPUI {
    class Context;

    class ContextIterator {
        public:
        Context * context;
        bool inContext;
        ContextIterator(Context *context);
        void operator++();
        bool operator!=(ContextIterator &obj);
        Context * operator*(void);
    };

    class Context {
        public:
        ContextIterator iter = ContextIterator(nullptr);

        ContextIterator & begin();
        ContextIterator & end();
        virtual void enter(void) = 0;
        virtual void exit(void) = 0;
    };
}

#endif
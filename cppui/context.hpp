#include <thread>
#include <vector>
#include <iterator>


#ifndef CPPUI_CONTEXT
#define CPPUI_CONTEXT

#include "context_header.hpp"

namespace CPPUI {
    class Context;

    inline ContextIterator::ContextIterator(Context *context) {
        inContext = true;
        this->context = context;
    }

    inline void ContextIterator::operator++()
    {
        inContext = false;
        context->exit();
    }

    inline bool ContextIterator::operator!=(ContextIterator &obj) {
        return inContext;
    }

    inline Context * ContextIterator::operator*(void) {
        return context;
    }

    inline ContextIterator & Context::begin() {
        iter = ContextIterator(this);
        enter();
        return iter;
    }
    inline ContextIterator & Context::end() { return iter; }
}

#endif

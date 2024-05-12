#include <thread>
#include <vector>
#include <iterator>


#ifndef CPPUI_CONTEXT
#define CPPUI_CONTEXT

#include "context_header.hpp"

namespace CPPUI {
    class Context;

    ContextIterator::ContextIterator(Context *context) {
        inContext = true;
        this->context = context;
    }

    void ContextIterator::operator++()
    {
        inContext = false;
        context->exit();
    }

    bool ContextIterator::operator!=(ContextIterator &obj) {
        return inContext;
    }

    Context * ContextIterator::operator*(void) {
        return context;
    }

    ContextIterator & Context::begin() {
        iter = ContextIterator(this);
        enter();
        return iter;
    }
    ContextIterator & Context::end() { return iter; }
}

#endif
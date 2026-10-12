#include <thread>
#include <vector>
#include <iterator>


#ifndef CPPUI_CONTEXT
#define CPPUI_CONTEXT

#include "context_header.hpp"

namespace CPPUI {
    class Context;

    inline ContextIterator::ContextIterator(Context *context) {
        inContext = context != nullptr;
        this->context = context;
        if(context) context->enter();
    }

    inline ContextIterator::ContextIterator(ContextIterator && other) noexcept
        : context(other.context), inContext(other.inContext) {
        other.inContext = false;
    }

    inline ContextIterator::~ContextIterator() {
        if(inContext) context->exit();
    }

    inline void ContextIterator::operator++()
    {
        if(inContext) {
            inContext = false;
            context->exit();
        }
    }

    inline bool ContextIterator::operator!=(const ContextIterator &obj) const {
        return inContext;
    }

    inline Context * ContextIterator::operator*(void) {
        return context;
    }

    inline ContextIterator Context::begin() {
        return ContextIterator(this);
    }
    inline ContextIterator Context::end() { return ContextIterator(); }
}

#endif

#ifndef CPPUI_MODIFIERS
#define CPPUI_MODIFIERS

#include "optional.hpp"

namespace CPPUI {
    // PUI's padding/margin order: top, right, bottom, left.
    struct Insets {
        int top, right, bottom, left;
        Insets(int all): Insets(all, all, all, all) {}
        Insets(int vertical, int horizontal): Insets(vertical, horizontal, vertical, horizontal) {}
        Insets(int top, int horizontal, int bottom): Insets(top, horizontal, bottom, horizontal) {}
        Insets(int top, int right, int bottom, int left): top(top), right(right), bottom(bottom), left(left) {}
    };
    struct QtOptions;
}
#endif

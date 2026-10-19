#ifndef CPPUI_ADAPTIVE
#define CPPUI_ADAPTIVE

#include "optional.hpp"
#include <memory>

namespace CPPUI {
    struct Size {
        int width, height;
        Size(int width=0,int height=0):width(width),height(height) {}
        bool operator==(const Size & other) const { return width==other.width && height==other.height; }
    };

    struct VisibilityBounds {
        Optional<int> minWidth,maxWidth,minHeight,maxHeight;
        VisibilityBounds(Optional<int> minWidth={},Optional<int> maxWidth={},
                         Optional<int> minHeight={},Optional<int> maxHeight={})
            :minWidth(minWidth),maxWidth(maxWidth),minHeight(minHeight),maxHeight(maxHeight) {}
        bool constrained() const { return minWidth || maxWidth || minHeight || maxHeight; }
        bool matches(const Optional<Size> & size) const {
            return size && (!minWidth || size->width>=*minWidth) && (!maxWidth || size->width<*maxWidth)
                && (!minHeight || size->height>=*minHeight) && (!maxHeight || size->height<*maxHeight);
        }
    };

    namespace detail {
        class SizeState;
        struct SizeObserver {
            virtual ~SizeObserver() {}
            virtual void refresh() = 0;
        };
    }
    // Read-only reactive measurement in native logical pixels. Empty until the
    // first completed layout; a saved binding follows retained declarations.
    class SizeBinding {
        std::shared_ptr<detail::SizeState> state;
    public:
        explicit SizeBinding(std::shared_ptr<detail::SizeState> state):state(std::move(state)) {}
        Optional<Size> get() const;
    };
}
#endif

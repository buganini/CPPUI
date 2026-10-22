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

    namespace detail {
        enum class WideMethod { Layout, Style, Visible, Grid, Columns, Count };
        // Implicit conversion keeps integer call sites while recording the
        // declaration location, including variadic modifier overloads.
        struct Breakpoint {
            int width;
            const char * file;
            int line;
#if defined(__GNUC__) || defined(__clang__) || (defined(_MSC_VER) && _MSC_VER >= 1926)
            Breakpoint(int width,const char * file=__builtin_FILE(),int line=__builtin_LINE())
#else
            Breakpoint(int width,const char * file="<unknown>",int line=0)
#endif
                :width(width),file(file),line(line) {}
        };

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

#ifndef CPPUI_GAP
#define CPPUI_GAP
#include <cmath>
#include <limits>
#include <stdexcept>
namespace CPPUI {
    namespace detail {
        inline int gapPixels(double value) {
            if(!std::isfinite(value) || value<0 || value>std::numeric_limits<int>::max())
                throw std::invalid_argument("gap must be a finite nonnegative pixel count");
            // Match Python's round-to-even conversion to native logical pixels.
            const double lower=std::floor(value), fraction=value-lower;
            return static_cast<int>(lower+(fraction>0.5 || (fraction==0.5 && std::fmod(lower,2)!=0)));
        }
    }
    struct Gap {
        int rows, columns;
        Gap(double value=10):Gap(value,value) {}
        Gap(double rows,double columns):rows(detail::gapPixels(rows)),columns(detail::gapPixels(columns)) {}
    };
}
#endif

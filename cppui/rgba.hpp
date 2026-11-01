#ifndef CPPUI_RGBA
#define CPPUI_RGBA
#include <cstddef>
#include <stdexcept>
#include <vector>
namespace CPPUI { namespace detail {
    inline void validateRGBA(int width,int height,const std::vector<unsigned char> & data) {
        if(width<1 || height<1 || width>16384 || height>16384 || std::size_t(width)*height>1048576 || data.size()!=std::size_t(width)*height*4)
            throw std::invalid_argument("RGBA buffer requires positive dimensions, at most 1048576 pixels and four bytes per pixel");
    }
}}
#endif

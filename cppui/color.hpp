#ifndef CPPUI_COLOR
#define CPPUI_COLOR
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
namespace CPPUI {
    // TRGB: the high byte stores transparency; ordinary 0xRRGGBB is opaque.
    inline std::uint32_t Color(std::uint32_t rgb,double alpha=1) {
        if(rgb>0xffffff || !std::isfinite(alpha) || alpha<0 || alpha>1)
            throw std::invalid_argument("Color requires RGB and an alpha between 0 and 1");
        return (std::uint32_t(std::nearbyint((1-alpha)*255))<<24)|rgb;
    }
    inline unsigned colorAlpha(std::uint32_t color) {return 255-(color>>24);}
    inline std::string colorCSS(std::uint32_t c) {
        if(colorAlpha(c)==255) {
            const char * hex="0123456789abcdef";std::string result="#";
            for(int shift=20;shift>=0;shift-=4) result+=hex[(c>>shift)&15];return result;
        }
        return "rgba("+std::to_string((c>>16)&255)+","+std::to_string((c>>8)&255)+","+std::to_string(c&255)+","+std::to_string(colorAlpha(c))+ ")";
    }
}
#endif

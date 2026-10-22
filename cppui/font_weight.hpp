#ifndef CPPUI_FONT_WEIGHT
#define CPPUI_FONT_WEIGHT

#include <stdexcept>

namespace CPPUI {
    // Numeric values follow the CSS/OpenType weight scale.
    enum class FontWeight : int {
        Thin = 100,
        Hairline = Thin,
        ExtraLight = 200,
        UltraLight = ExtraLight,
        Light = 300,
        Normal = 400,
        Regular = Normal,
        Medium = 500,
        DemiBold = 600,
        SemiBold = DemiBold,
        Bold = 700,
        ExtraBold = 800,
        UltraBold = ExtraBold,
        Black = 900,
        Heavy = Black,
        ExtraBlack = 950,
        UltraBlack = ExtraBlack
    };
    namespace detail {
        inline void validateFontWeight(int value) {
            if(value < 1 || value > 1000)
                throw std::invalid_argument("Font weight must be an integer from 1 to 1000");
        }
    }
}
#endif

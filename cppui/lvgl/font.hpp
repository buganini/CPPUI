#ifndef CPPUI_LVGL_FONT
#define CPPUI_LVGL_FONT

#include <lvgl.h>
#include <cstdlib>
#include <stdexcept>

#if CPPUI_LVGL_BOLD_FONTS
LV_FONT_DECLARE(cppui_font_montserrat_bold_14)
LV_FONT_DECLARE(cppui_font_montserrat_bold_28)
#endif

namespace CPPUI { namespace LVGL { namespace Fonts {
    inline bool isBoldFont(const lv_font_t * font) {
#if CPPUI_LVGL_BOLD_FONTS
        return font==&cppui_font_montserrat_bold_14 || font==&cppui_font_montserrat_bold_28;
#else
        (void)font;return false;
#endif
    }
    inline const lv_font_t * withFontWeight(const lv_font_t * font,bool bold) {
#if CPPUI_LVGL_BOLD_FONTS
        if(bold) {
            return std::abs(int(font->line_height)-int(cppui_font_montserrat_bold_14.line_height))
                <= std::abs(int(font->line_height)-int(cppui_font_montserrat_bold_28.line_height))
                ? &cppui_font_montserrat_bold_14 : &cppui_font_montserrat_bold_28;
        }
        if(font==&cppui_font_montserrat_bold_14) return &lv_font_montserrat_14;
        if(font==&cppui_font_montserrat_bold_28) return &lv_font_montserrat_28;
#else
        if(bold) throw std::invalid_argument("LVGL bold text requires the CPPUI Montserrat bold fonts");
#endif
        return font;
    }
}}}
#endif

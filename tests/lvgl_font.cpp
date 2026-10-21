#include "cppui/lvgl/font.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>

int main() {
    lv_init();int result=0;
    try {
        // Shared cookbook captions use these Unicode characters at both the
        // default size and the larger size selected by style::fontSize().
        for(auto font:{LV_FONT_DEFAULT,&lv_font_montserrat_28,&cppui_font_montserrat_bold_14,&cppui_font_montserrat_bold_28}) {
            for(uint32_t codepoint:{0x00b7u,0x00d7u,0x2013u,0x2192u,0x41u,0xf00cu}) {
                lv_font_glyph_dsc_t glyph{};
                if(!lv_font_get_glyph_dsc(font,&glyph,codepoint,0) || glyph.is_placeholder
                   || !glyph.adv_w || !glyph.box_w || !glyph.box_h
                   || !lv_font_get_glyph_static_bitmap(&glyph)) {
                    std::ostringstream error;
                    error<<"missing LVGL glyph U+"<<std::hex<<codepoint
                         <<" at line height "<<std::dec<<font->line_height;
                    throw std::runtime_error(error.str());
                }
                if(glyph.resolved_font->line_height!=font->line_height
                   || glyph.resolved_font->base_line!=font->base_line)
                    throw std::runtime_error("fallback glyph metrics differ from the selected font");
            }
        }
        for(auto regular:{LV_FONT_DEFAULT,&lv_font_montserrat_28}) {
            const auto bold=CPPUI::LVGL::Fonts::withFontWeight(regular,true);
            if(regular->line_height!=bold->line_height || regular->base_line!=bold->base_line)
                throw std::runtime_error("weight change altered line metrics");
            unsigned ink[2]={};int index=0;
            for(auto font:{regular,bold}) {
                lv_font_glyph_dsc_t glyph{};lv_font_get_glyph_dsc(font,&glyph,'M',0);
                if(glyph.resolved_font!=font || glyph.format!=LV_FONT_GLYPH_FORMAT_A4)
                    throw std::runtime_error("weight font did not supply its own 4-bit glyph");
                const auto pixels=static_cast<const uint8_t *>(lv_font_get_glyph_static_bitmap(&glyph));
                for(unsigned i=0;i<unsigned(glyph.box_w)*glyph.box_h;++i)
                    ink[index]+=(pixels[i/2]>>(i%2 ? 0 : 4))&15;
                ++index;
            }
            if(ink[1]<=ink[0]) throw std::runtime_error("bold glyph is not heavier than the regular glyph");
        }
    } catch(const std::exception & error) {std::cerr<<error.what()<<'\n';result=1;}
    lv_deinit();return result;
}

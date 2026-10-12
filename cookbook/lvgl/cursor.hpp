#ifndef CPPUI_COOKBOOK_LVGL_CURSOR
#define CPPUI_COOKBOOK_LVGL_CURSOR

#include "lvgl.h"
#include <array>

namespace Cookbook { namespace Backend {
    // The X11 driver hides the desktop cursor. Supply its optional image using
    // static storage: LVGL keeps the descriptor and pixels by reference.
    inline const lv_image_dsc_t * mouseCursor() {
        static const char shape[][13] = {
            "#           ",
            "##          ",
            "#.#         ",
            "#..#        ",
            "#...#       ",
            "#....#      ",
            "#.....#     ",
            "#......#    ",
            "#.......#   ",
            "#........#  ",
            "#.........# ",
            "#.....######",
            "#..#..#     ",
            "#.# #..#    ",
            "##  #..#    ",
            "#    #..#   ",
            "     #..#   ",
            "      ##    "
        };
        static const std::array<lv_color32_t, 12 * 18> pixels = []() {
            std::array<lv_color32_t, 12 * 18> result{};
            for(unsigned y = 0; y < 18; ++y) {
                for(unsigned x = 0; x < 12; ++x) {
                    auto & pixel = result[y * 12 + x];
                    pixel.red = pixel.green = pixel.blue = shape[y][x] == '.' ? 255 : 0;
                    pixel.alpha = shape[y][x] == ' ' ? 0 : 255;
                }
            }
            return result;
        }();
        static const lv_image_dsc_t image = []() {
            lv_image_dsc_t result{};
            result.header.magic = LV_IMAGE_HEADER_MAGIC;
            result.header.cf = LV_COLOR_FORMAT_ARGB8888;
            result.header.w = 12;
            result.header.h = 18;
            result.header.stride = 12 * sizeof(lv_color32_t);
            result.data_size = sizeof(pixels);
            result.data = reinterpret_cast<const uint8_t *>(pixels.data());
            return result;
        }();
        return &image;
    }
}}

#endif

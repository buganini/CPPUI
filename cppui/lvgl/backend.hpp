#ifndef CPPUI_LVGL_BACKEND
#define CPPUI_LVGL_BACKEND

#include "application.hpp"
#include "window.hpp"
#include "layout.hpp"
#include "grid.hpp"
#include "stack.hpp"
#include "divider.hpp"
#include "label.hpp"
#include "button.hpp"
#include "checkbox.hpp"
#include "progressbar.hpp"
#include "canvas.hpp"
#include "tab.hpp"
#include "views.hpp"
#include "table.hpp"
#include "image.hpp"
#include "textfield.hpp"
#include "text.hpp"
#include "radiobutton.hpp"
#include "combobox.hpp"
#include "scroll.hpp"
#include "timeline.hpp"

#define CPPUI_HAS_TREE 0
#define CPPUI_HAS_SPLITTER 0
#define CPPUI_HAS_DIALOG 0
#define CPPUI_HAS_DROP 0
#define CPPUI_HAS_SCROLL_ANCHOR 0
#define CPPUI_HAS_MENUS 0
#define CPPUI_HAS_WINDOW_COMMANDS 0
#define CPPUI_HAS_MDI 0
#define CPPUI_HAS_MODAL 0
#define CPPUI_HAS_TAB_BINDING 0

namespace CPPUI { namespace Backend {
    using View = LVGL::View;
    using Views = LVGL::Views;
#if LV_USE_SDL
    constexpr const char * name = "LVGL SDL2";
#elif LV_USE_X11
    constexpr const char * name = "LVGL X11";
#else
    constexpr const char * name = LVGL::backendName;
#endif
    constexpr bool formattedHtml = false;
    constexpr const char * textSupport = "Plain multiline text implemented; HTML and Markdown display raw source; selection not yet implemented";
}}
#endif

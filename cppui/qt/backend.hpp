#ifndef CPPUI_QT_BACKEND
#define CPPUI_QT_BACKEND

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
#include "splitter.hpp"
#include "tree.hpp"
#include "dialog.hpp"
#include "mdi.hpp"
#include "modal.hpp"

#define CPPUI_HAS_TREE 1
#define CPPUI_HAS_SPLITTER 1
#define CPPUI_HAS_DIALOG 1
#define CPPUI_HAS_DROP 1
#define CPPUI_HAS_SCROLL_ANCHOR 1
#define CPPUI_HAS_MENUS 1
#define CPPUI_HAS_WINDOW_COMMANDS 1
#define CPPUI_HAS_MDI 1
#define CPPUI_HAS_MODAL 1
#define CPPUI_HAS_TAB_BINDING 1

namespace CPPUI { namespace Backend {
    using View = Qt::QtView;
    using Views = Qt::Views;
    constexpr const char * name = Qt::backendName;
    constexpr bool formattedHtml = true;
    constexpr const char * textSupport = "";
}}
#endif

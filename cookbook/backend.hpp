#ifndef CPPUI_COOKBOOK_BACKEND
#define CPPUI_COOKBOOK_BACKEND

#if (defined(CPPUI_COOKBOOK_QT) + defined(CPPUI_COOKBOOK_WX) + defined(CPPUI_COOKBOOK_LVGL)) != 1
#error "Select exactly one cookbook backend: CPPUI_COOKBOOK_QT, CPPUI_COOKBOOK_WX or CPPUI_COOKBOOK_LVGL"
#endif

#if defined(CPPUI_COOKBOOK_LVGL) && (CPPUI_LVGL_SDL2 + CPPUI_LVGL_X11 != 1)
#error "Select exactly one LVGL desktop driver: CPPUI_LVGL_SDL2 or CPPUI_LVGL_X11"
#endif

#if defined(CPPUI_COOKBOOK_QT)
#define CPPUI_COOKBOOK_HEADER(file) <cppui/qt/file>
#elif defined(CPPUI_COOKBOOK_WX)
#define CPPUI_COOKBOOK_HEADER(file) <cppui/wx/file>
#else
#define CPPUI_COOKBOOK_HEADER(file) <cppui/lvgl/file>
#endif

#include CPPUI_COOKBOOK_HEADER(application.hpp)
#include CPPUI_COOKBOOK_HEADER(window.hpp)
#include CPPUI_COOKBOOK_HEADER(layout.hpp)
#include CPPUI_COOKBOOK_HEADER(grid.hpp)
#include CPPUI_COOKBOOK_HEADER(stack.hpp)
#include CPPUI_COOKBOOK_HEADER(divider.hpp)
#include CPPUI_COOKBOOK_HEADER(label.hpp)
#include CPPUI_COOKBOOK_HEADER(button.hpp)
#include CPPUI_COOKBOOK_HEADER(checkbox.hpp)
#include CPPUI_COOKBOOK_HEADER(progressbar.hpp)
#include CPPUI_COOKBOOK_HEADER(canvas.hpp)
#include CPPUI_COOKBOOK_HEADER(tab.hpp)
#include CPPUI_COOKBOOK_HEADER(views.hpp)
#include CPPUI_COOKBOOK_HEADER(table.hpp)
#include CPPUI_COOKBOOK_HEADER(image.hpp)
#include CPPUI_COOKBOOK_HEADER(textfield.hpp)
#include CPPUI_COOKBOOK_HEADER(text.hpp)
#include CPPUI_COOKBOOK_HEADER(radiobutton.hpp)
#include CPPUI_COOKBOOK_HEADER(combobox.hpp)
#include CPPUI_COOKBOOK_HEADER(scroll.hpp)
#include CPPUI_COOKBOOK_HEADER(timeline.hpp)
#if !defined(CPPUI_COOKBOOK_LVGL)
#include CPPUI_COOKBOOK_HEADER(splitter.hpp)
#include CPPUI_COOKBOOK_HEADER(tree.hpp)
#include CPPUI_COOKBOOK_HEADER(dialog.hpp)
#endif
#if defined(CPPUI_COOKBOOK_QT)
#include CPPUI_COOKBOOK_HEADER(modal.hpp)
#include CPPUI_COOKBOOK_HEADER(mdi.hpp)
#endif
#undef CPPUI_COOKBOOK_HEADER

#include "recipes.hpp"

namespace Cookbook { namespace Backend {
#if defined(CPPUI_COOKBOOK_QT)
    namespace Native = CPPUI::Qt;
    using View = Native::QtView;
#elif defined(CPPUI_COOKBOOK_WX)
    namespace Native = CPPUI::Wx;
    using View = Native::View;
#else
    namespace Native = CPPUI::LVGL;
    using View = Native::View;
#endif
    using Views = Native::Views;
#if defined(CPPUI_COOKBOOK_LVGL)
    constexpr bool desktopWindowCommands = false;
#else
    constexpr bool desktopWindowCommands = true;
#endif
    extern const std::string name;
    extern const std::string title;

    void configureWindow(CPPUI::Context * node);
    void menus();
    void configureRecipes(std::vector<Recipe> & pages);
    void mount(CPPUI::Application & app);

    // Native operations used by the shared cookbook smoke test.
    bool viewSelected(CPPUI::Node & node, const std::string & key);
    bool wheelCanvas(CPPUI::Node & node, int delta);
    void settle();
    int treeChildCount(CPPUI::Node & node, const std::vector<int> & path);
    bool treeExpanded(CPPUI::Node & node, const std::vector<int> & path);
    std::string tableText(CPPUI::Node & node, int row, int column);
    bool editTable(CPPUI::Node & node, int row, int column, const std::string & value);
    bool atScrollEnd(CPPUI::Node & node);
    bool selectCombo(CPPUI::Node & node, int index);
    bool selectRadio(CPPUI::Node & node);
    bool editTextField(CPPUI::Node & node, const std::string & text, bool commit);
    bool clickButton(CPPUI::Node & node, const std::string & text);
    bool labelHasText(CPPUI::Node & node, const std::string & text);
    bool selectTab(CPPUI::Node & node, int index, const std::string & text);
    bool toggleCheckbox(CPPUI::Node & node);
    void answerPrompt(const std::string & text, bool accepted);
    bool modalVisible(CPPUI::Node & node);
    bool selectPage(CPPUI::Node & node);
    void prepareLayoutCheck(CPPUI::Node & root);
    bool pageVisible(CPPUI::Node & root, const std::string & key);
    void checkLayout(CPPUI::Node & root);
}}
#endif

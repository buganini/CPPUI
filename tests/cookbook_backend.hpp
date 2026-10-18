#ifndef CPPUI_TEST_COOKBOOK_BACKEND
#define CPPUI_TEST_COOKBOOK_BACKEND
#include "../cookbook/gallery.hpp"
namespace Cookbook {
    CPPUI::Node * find(CPPUI::Node & node, const std::string & id);
    namespace TestBackend {
    using Views = CPPUI::Backend::Views;
    constexpr const char * name = CPPUI::Backend::name;
    const std::string title = Cookbook::title();
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

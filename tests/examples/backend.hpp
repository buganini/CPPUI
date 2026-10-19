#ifndef CPPUI_TEST_EXAMPLES_BACKEND
#define CPPUI_TEST_EXAMPLES_BACKEND
#include "cppui/backend.hpp"
#include <stdexcept>

namespace ExampleTest {
    void settle();
    bool clickButton(CPPUI::Node & node, const std::string & text);
    bool labelHasText(CPPUI::Node & node, const std::string & text);
    bool toggleCheckbox(CPPUI::Node & node);

    inline void require(bool value, const char * message) {
        if(!value) throw std::runtime_error(message);
    }
    inline CPPUI::Node * find(CPPUI::Node & node, const std::string & id) {
        if(node.has_id && node._id == id) return &node;
        for(auto child: *node.children) if(auto found = find(*child, id)) return found;
        return nullptr;
    }
    inline bool click(CPPUI::Node & node, const std::string & text) {
        if(clickButton(node, text)) return true;
        for(auto child: *node.children) if(click(*child, text)) return true;
        return false;
    }
    inline bool hasText(CPPUI::Node & node, const std::string & text) {
        if(labelHasText(node, text)) return true;
        for(auto child: *node.children) if(hasText(*child, text)) return true;
        return false;
    }
}
#endif

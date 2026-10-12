#ifndef CPPUI_COOKBOOK_ADAPTER_TREE_DATA
#define CPPUI_COOKBOOK_ADAPTER_TREE_DATA

#include "cppui/state.hpp"
#include <memory>
#include <string>
#include <vector>

namespace Cookbook {
    struct AdapterTreeNode: std::enable_shared_from_this<AdapterTreeNode> {
        std::string text;
        std::weak_ptr<AdapterTreeNode> parent;
        std::vector<std::shared_ptr<AdapterTreeNode>> children;
        explicit AdapterTreeNode(std::string text): text(std::move(text)) {}
        std::shared_ptr<AdapterTreeNode> add(const std::string & text) {
            auto node = std::make_shared<AdapterTreeNode>(text);
            node->parent = shared_from_this(); children.push_back(node); return node;
        }
    };
    struct AdapterTreeData {
        std::shared_ptr<AdapterTreeNode> root = std::make_shared<AdapterTreeNode>("Root");
        CPPUI::State<int> revision{0};
        AdapterTreeData() {
            auto sub1 = root->add("Sub1");
            sub1->add("Sub1.1");
            auto sub12 = sub1->add("Sub1.2");
            sub12->add("Sub1.2.1"); sub12->add("Sub1.2.2");
            sub1->add("Sub1.3"); root->add("Sub2");
            auto sub3 = root->add("Sub3"); sub3->add("Sub3.1"); sub3->add("Sub3.2");
        }
        void addItem() { root->add("New item")->add("Sub"); revision += 1; }
    };
}
#endif

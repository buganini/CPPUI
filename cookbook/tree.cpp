#include "backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    void tree(Model & model) {
        for(auto column: Presentation::VBox(1)) {
            for(auto row: Presentation::HBox()) {
                CPPUI::Button("Add item").click([&model]() { model.treeItems += 1; });
                Presentation::Spacer();
            }
            for(auto row: Presentation::HBox(1)) {
                for(auto tree: CPPUI::Tree().expandAll().id("example-tree")) {
                    for(auto root: CPPUI::TreeNode("Root")) {
                        for(auto branch: CPPUI::TreeNode("Sub 1")) {
                            CPPUI::TreeNode("Sub 1-1");
                            auto & child = CPPUI::TreeNode("Sub 1-2").dblclick([]() { std::cout << "Sub 1-2 dblclicked\n"; });
                            child.click([]() { std::cout << "Sub 1-2 clicked\n"; });
                        }
                        CPPUI::TreeNode("Sub 2");
                        for(auto branch: CPPUI::TreeNode("Sub 3")) CPPUI::TreeNode("Sub 3-1");
                        for(auto branch: CPPUI::TreeNode("Recursive")) {
                            for(int i = 0; i < model.treeItems; ++i) {
                                for(auto added: CPPUI::TreeNode("New item").id(std::to_string(i))) CPPUI::TreeNode("Sub");
                            }
                        }
                    }
                }
            }
        }
    }
}

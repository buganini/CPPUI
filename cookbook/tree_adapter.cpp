#include "backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    namespace {
        class HierarchyAdapter: public CPPUI::BaseTreeAdapter<AdapterTreeNode> {
            std::shared_ptr<AdapterTreeData> values;
            public:
            explicit HierarchyAdapter(std::shared_ptr<AdapterTreeData> values): values(std::move(values)) {}
            Handle parent(const Handle & node) const override { return node ? node->parent.lock() : Handle(); }
            Handle child(const Handle & parent, int index) const override { return (parent ? parent : values->root)->children.at(index); }
            std::string data(const Handle & node) const override { return node->text; }
            int rowCount(const Handle & parent) const override {
                (void)static_cast<int>(values->revision);
                return static_cast<int>((parent ? parent : values->root)->children.size());
            }
            void clicked(const Handle & node) override { std::cout << "clicked " << node->text << '\n'; }
            void dblclicked(const Handle & node) override { std::cout << "dblclicked " << node->text << '\n'; }
            void expanded(const Handle & node) override { std::cout << "expanded " << node->text << '\n'; }
            void collapsed(const Handle & node) override { std::cout << "collapsed " << node->text << '\n'; }
        };
    }
    void treeAdapter(Model & model) {
        if(!model.adapterTreeSource) model.adapterTreeSource = std::make_shared<HierarchyAdapter>(model.adapterTree);
        auto source = model.adapterTreeSource;
        for(auto column: CPPUI::VBox().layout({}, {}, 1)) {
            for(auto row: CPPUI::HBox()) {
                auto data = model.adapterTree;
                CPPUI::Button("Add item").click([data]() { data->addItem(); });
                CPPUI::Spacer();
            }
            for(auto row: CPPUI::HBox().layout({}, {}, 1)) {
                CPPUI::Tree(source).id("adapter-tree-first");
                CPPUI::Tree(source).expandAll().id("adapter-tree-second");
            }
        }
    }
}

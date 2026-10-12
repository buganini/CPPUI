#include "cppui/qt/tree.hpp"
#include "cppui/qt/application.hpp"
#include "cppui/qt/window.hpp"
#include "cppui/qt/layout.hpp"
#include <QAbstractItemModelTester>
#include <QPersistentModelIndex>
#include <sstream>

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    void settle() { QCoreApplication::processEvents(); QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete); }
    struct Item {
        CPPUI::State<std::string> text;
        std::weak_ptr<Item> parent;
        std::vector<std::shared_ptr<Item>> children;
        explicit Item(const std::string & text): text(text) {}
        static std::shared_ptr<Item> add(const std::shared_ptr<Item> & parent, const std::string & text) {
            auto child = std::make_shared<Item>(text); child->parent = parent; parent->children.push_back(child); return child;
        }
    };
    struct Adapter: CPPUI::BaseTreeAdapter<Item> {
        std::vector<Handle> roots{std::make_shared<Item>("First"), std::make_shared<Item>("Second"), std::make_shared<Item>("Third")};
        mutable CPPUI::State<int> revision{0};
        int clicks = 0, doubles = 0, expands = 0, collapses = 0;
        int invalid = 0;
        bool throwClick = false;
        std::function<void()> afterClick;
        Adapter() { Item::add(roots[0], "A"); Item::add(Item::add(roots[1], "B"), "Leaf"); }
        Handle parent(const Handle & node) const override { return node ? node->parent.lock() : Handle(); }
        Handle child(const Handle & parent, int row) const override {
            if(invalid == 1 && parent) return parent;
            if(invalid == 2) return {};
            return (parent ? parent->children : roots).at(row);
        }
        std::string data(const Handle & node) const override { return static_cast<std::string>(node->text); }
        int rowCount(const Handle & parent) const override {
            (void)static_cast<int>(revision);
            return invalid == 3 ? -1 : static_cast<int>((parent ? parent->children : roots).size());
        }
        void clicked(const Handle &) override { ++clicks; if(throwClick) throw std::runtime_error("adapter click failure"); if(afterClick) afterClick(); }
        void dblclicked(const Handle &) override { ++doubles; }
        void expanded(const Handle &) override { ++expands; }
        void collapsed(const Handle &) override { ++collapses; }
    };
    struct Root: CPPUI::Application {
        using Application::Application;
        std::shared_ptr<Adapter> source = std::make_shared<Adapter>();
        bool show = true, expanded = true, declarative = false;
        void content() override {
            for(auto window: CPPUI::Window()) for(auto row: CPPUI::HBox()) if(show) {
                if(declarative) {
                    for(auto tree: CPPUI::Tree().id("first")) CPPUI::TreeNode("Declarative");
                } else {
                    auto & first = CPPUI::Tree(source); first.id("first");
                    if(expanded) first.expandAll();
                    CPPUI::Tree(source).id("second");
                }
            }
        }
        CPPUI::Qt::Tree * tree(int index = 0) { return static_cast<CPPUI::Qt::Tree *>(children->front()->children->front()->children->at(index)); }
    };
}
int main(int argc, char ** argv) {
    QApplication qt(argc, argv);
    try {
        {
            Root root(argc, argv); root.sync(); settle();
            auto native = static_cast<QTreeView *>(root.tree()->ui); auto model = native->model();
            QAbstractItemModelTester tester(model, QAbstractItemModelTester::FailureReportingMode::Fatal);
            auto branch = model->index(1, 0); auto child = model->index(0, 0, branch); auto leaf = model->index(0, 0, child);
            require(model->rowCount() == 3 && child.parent() == branch && leaf.parent() == child && !branch.parent().isValid(),
                "adapter hierarchy or non-first parent rows are incorrect");
            require(child.internalPointer() != root.source->roots[1]->children[0].get(), "Qt indexes point directly at application data");
            require(native->isExpanded(branch) && native->isExpanded(child)
                && !static_cast<QTreeView *>(root.tree(1)->ui)->isExpanded(root.tree(1)->model->index(1, 0)), "independent expansion was lost");
            QPersistentModelIndex stableBranch(branch), stableChild(child), removedLeaf(leaf);
            native->setCurrentIndex(child); root.expanded = false;
            std::swap(root.source->roots[0], root.source->roots[1]); root.source->revision += 1; settle();
            require(root.tree()->ui == native && model == native->model() && stableBranch.row() == 0 && stableChild.parent() == stableBranch
                && native->currentIndex() == stableChild && native->isExpanded(stableChild), "adapter reorder lost indexes, selection or expansion");
            root.source->roots[0]->children[0]->text = std::string("Renamed"); settle();
            require(stableChild.data() == "Renamed" && root.tree(1)->model->index(0, 0, root.tree(1)->model->index(0, 0)).data() == "Renamed",
                "State text change did not update both adapter views");
            emit native->clicked(stableChild); emit native->doubleClicked(stableChild);
            const auto expansions = root.source->expands, collapses = root.source->collapses;
            native->collapse(stableChild); native->expand(stableChild);
            require(root.source->clicks == 1 && root.source->doubles == 1 && root.source->expands == expansions + 1
                && root.source->collapses == collapses + 1, "adapter callbacks duplicated or used old data");
            std::weak_ptr<Item> removed = root.source->roots[0]->children[0]->children[0];
            root.source->roots[0]->children[0]->children.clear(); root.source->revision += 1; settle();
            require(!removedLeaf.isValid() && stableChild.isValid() && removed.expired(), "removed data/index remained retained");
            for(int invalid = 1; invalid <= 3; ++invalid) {
                root.source->invalid = invalid; bool rejected = false;
                try { root.sync(); } catch(const std::invalid_argument &) { rejected = true; }
                require(rejected && root.tree()->ui == native && stableChild.isValid(), "invalid adapter hierarchy replaced the old tree");
            }
            root.source->invalid = 0;
            root.source->throwClick = true;
            std::ostringstream errors; auto old = std::cerr.rdbuf(errors.rdbuf());
            emit native->clicked(stableChild); std::cerr.rdbuf(old);
            require(errors.str().find("adapter click failure") != std::string::npos, "adapter exception escaped native dispatch");
            root.source->throwClick = false;
            std::weak_ptr<Adapter> oldSource = root.source;
            root.source = std::make_shared<Adapter>(); root.sync(); settle();
            require(oldSource.expired() && !stableChild.isValid() && model->rowCount() == 3, "source replacement retained the old adapter/indexes");
            root.declarative = true; root.sync(); settle();
            require(root.tree()->ui == native && model->index(0, 0).data() == "Declarative", "switch to declarative tree replaced native UI or kept adapter data");
            root.declarative = false; root.sync(); settle();
            oldSource = root.source;
            root.source->afterClick = [&]() { root.source.reset(); root.show = false; root.sync(); };
            emit native->clicked(model->index(0, 0));
            require(model->rowCount() == 0 && oldSource.expired(), "self-removing callback retained the tree adapter"); settle();
            for(int i = 0; i < 12; ++i) {
                root.source = std::make_shared<Adapter>(); root.show = true; root.sync();
                root.show = false; root.sync(); settle();
            }
            root.source = std::make_shared<Adapter>(); root.source->roots.clear(); root.show = true; root.sync();
            oldSource = root.source; root.source.reset();
            require(!oldSource.expired(), "empty mounted tree did not retain its adapter");
            root.show = false; root.sync(); settle(); require(oldSource.expired(), "empty removed tree retained its adapter");
            root.source = std::make_shared<Adapter>(); root.show = true; root.sync();
            auto declaration = root.tree(); delete static_cast<QTreeView *>(declaration->ui);
            require(!declaration->ui && !declaration->model, "native deletion left tree adapter observers dangling");
        }
        settle(); require(qt.topLevelWidgets().isEmpty(), "tree adapter teardown leaked native roots");
    } catch(const std::exception & error) { std::cerr << error.what() << '\n'; return 1; }
}

#include "cppui/qt/tree.hpp"
#include "cppui/qt/application.hpp"
#include "cppui/qt/window.hpp"
#include <QPersistentModelIndex>
#include <QItemSelectionModel>
#include <QAbstractItemModelTester>
#include <sstream>

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    void settle() {
        for(int i = 0; i < 4; ++i) QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
    class Items: public CPPUI::Qt::QtView {
        public:
        CPPUI::State<int> count{2};
        void content() override {
            for(int i = 0; i < count; ++i) CPPUI::TreeNode("dynamic " + std::to_string(i)).id(std::to_string(i));
        }
    };
    class Root: public CPPUI::Application {
        public:
        int generation = 0, clicks = 0, doubles = 0, expands = 0, collapses = 0;
        bool show = true, reverse = false, showChild = true, showDynamic = false, selfRemove = false, throwClick = false;
        int expansion = 1;
        CPPUI::Optional<bool> expandable;
        CPPUI::State<std::string> label{std::string("Root")};
        std::shared_ptr<int> capture = std::make_shared<int>(1);
        using Application::Application;
        void content() override {
            for(auto window: CPPUI::Window().size(400, 400)) if(show) {
                auto & tree = CPPUI::Tree();
                if(expansion > 0) tree.expandAll();
                if(expansion == 0) tree.collapseAll();
                if(expandable) tree.expandable(*expandable);
                for(auto scope: tree) {
                    const auto keys = reverse ? std::vector<std::string>{"tail", "root"} : std::vector<std::string>{"root", "tail"};
                    for(const auto & key: keys) {
                        if(key == "tail") CPPUI::TreeNode("Tail").id(key);
                        else for(auto root: CPPUI::TreeNode(label).id(key)) {
                            if(showChild) {
                                auto data = capture;
                                auto & child = CPPUI::TreeNode("child " + std::to_string(generation))
                                    .expand([this, data]() { expands += *data; })
                                    .collapse([this, data]() { collapses += *data; })
                                    .dblclick([this, data]() { doubles += *data; });
                                child.id("child");
                                child.click([this, data]() {
                                    clicks += *data;
                                    if(throwClick) throw std::runtime_error("expected tree callback failure");
                                    if(selfRemove) { show = false; sync(); }
                                });
                                for(auto nested: child) CPPUI::TreeNode("grandchild " + std::to_string(generation));
                            }
                            if(showDynamic) (new Items())->id("dynamic");
                        }
                    }
                }
            }
        }
        CPPUI::Qt::Tree * declaration() { return static_cast<CPPUI::Qt::Tree *>(children->front()->children->front()); }
        QTreeView * native() { return static_cast<QTreeView *>(declaration()->ui); }
        Items * items() {
            for(auto root: *declaration()->children) for(auto child: *root->children)
                if(auto view = dynamic_cast<Items *>(child)) return view;
            return nullptr;
        }
    };
}
int main(int argc, char ** argv) {
    QApplication qt(argc, argv);
    try {
        {
            Root root(argc, argv); root.sync(); settle();
            auto tree = root.native(); auto model = tree->model();
            QAbstractItemModelTester tester(model, QAbstractItemModelTester::FailureReportingMode::Fatal);
            auto rootIndex = model->index(0, 0);
            auto childIndex = model->index(0, 0, rootIndex);
            auto grandchild = model->index(0, 0, childIndex);
            require(model->data(rootIndex) == "Root" && model->parent(childIndex) == rootIndex && model->parent(grandchild) == childIndex,
                "tree hierarchy or parent indexes were wrong");
            require(tree->isHeaderHidden() && tree->isExpanded(rootIndex) && tree->isExpanded(childIndex), "initial expansion or hidden header was lost");
            require(!model->index(0, 1).isValid() && !model->index(-1, 0).isValid(), "tree accepted an invalid index");
            QPersistentModelIndex persistentRoot(rootIndex), persistentChild(childIndex), persistentGrandchild(grandchild);
            const auto nativeIdentity = childIndex.internalPointer();
            tree->setCurrentIndex(childIndex);
            tree->selectionModel()->select(childIndex, QItemSelectionModel::ClearAndSelect);
            root.expansion = -1; root.generation = 1; root.reverse = true; root.sync(); settle();
            require(root.native() == tree && tree->model() == model, "redraw replaced native tree/model");
            require(persistentRoot.row() == 1 && persistentChild.internalPointer() == nativeIdentity && persistentChild.data() == "child 1",
                "reorder lost stable identity or label changes");
            require(model->parent(persistentChild) == persistentRoot && model->parent(persistentGrandchild) == persistentChild,
                "reorder left incorrect parent rows");
            require(tree->currentIndex() == persistentChild && tree->selectionModel()->isSelected(persistentChild)
                && tree->isExpanded(persistentRoot) && tree->isExpanded(persistentChild), "reorder lost selection or expansion");
            std::weak_ptr<int> previousCapture = root.capture;
            root.capture = std::make_shared<int>(7); root.sync(); settle();
            require(previousCapture.expired(), "tree retained retired callback captures");
            emit tree->clicked(persistentChild); emit tree->doubleClicked(persistentChild);
            require(root.clicks == 7 && root.doubles == 7, "tree dispatched old or duplicate click callbacks");
            const int expansions = root.expands, collapses = root.collapses;
            tree->collapse(persistentChild); tree->expand(persistentChild);
            require(root.expands == expansions + 7 && root.collapses == collapses + 7, "node expansion callbacks did not use current data");
            root.label = std::string("changed"); settle(); require(persistentRoot.data() == "changed", "tree label binding did not redraw");
            root.expansion = 0; root.sync(); require(!tree->isExpanded(persistentRoot), "declared collapseAll did not apply");
            root.expansion = -1; root.declaration()->expandAll(); require(tree->isExpanded(persistentChild), "imperative expandAll did not apply");
            root.expandable = false; root.sync(); require(!tree->itemsExpandable(), "expandable(false) was ignored");
            root.expandable = {}; root.sync(); require(!tree->itemsExpandable(), "omitted imperative expandable state was unexpectedly reset");
            root.declaration()->expandable(true); require(tree->itemsExpandable(), "imperative expandability was not restored");
            root.throwClick = true;
            std::ostringstream errors; auto previous = std::cerr.rdbuf(errors.rdbuf());
            emit tree->clicked(persistentChild); std::cerr.rdbuf(previous);
            require(errors.str().find("expected tree callback failure") != std::string::npos, "tree callback exception escaped native dispatch");
            root.throwClick = false;
            root.showChild = false; root.sync(); settle();
            require(!persistentChild.isValid() && !persistentGrandchild.isValid() && persistentRoot.isValid(), "removed indexes survived or a retained ancestor was invalidated");
            root.showDynamic = true; root.sync(); settle();
            auto items = root.items(); require(items && model->rowCount(persistentRoot) == 2, "virtual tree view did not flatten its children");
            QPersistentModelIndex dynamicIndex(model->index(1, 0, persistentRoot));
            items->count = 0; items->sync(); settle();
            require(!dynamicIndex.isValid() && model->rowCount(persistentRoot) == 0, "independent empty subview did not update tree indexes");
            items->count = 3; items->sync(); settle();
            require(model->rowCount(persistentRoot) == 3 && model->index(2, 0, persistentRoot).data() == "dynamic 2", "independent subview growth was lost");
            items->count = 4; items->sync(); // Native model refresh is queued; remove the view before it runs.
            root.showDynamic = false; root.showChild = true; root.sync(); settle();
            require(model->rowCount(persistentRoot) == 1, "removed subview reached queued tree work");
            root.selfRemove = true;
            childIndex = model->index(0, 0, persistentRoot);
            emit tree->clicked(childIndex);
            require(!persistentRoot.isValid() && model->rowCount() == 0, "self-removal did not deactivate the model and indexes");
            const int calls = root.clicks; emit tree->clicked(childIndex);
            require(root.clicks == calls, "retired tree dispatched a late signal"); settle();
            for(int i = 0; i < 20; ++i) { root.show = true; root.sync(); root.show = false; root.sync(); settle(); }
            root.show = true; root.selfRemove = false; root.sync();
            auto declaration = root.declaration(); delete root.native();
            require(!declaration->ui && !declaration->model, "external tree deletion left stale native observers");
        }
        settle(); require(qt.topLevelWidgets().isEmpty(), "tree teardown retained native roots");
    } catch(const std::exception & error) { std::cerr << error.what() << '\n'; return 1; }
}

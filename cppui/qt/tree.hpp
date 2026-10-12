#ifndef CPPUI_QT_TREE
#define CPPUI_QT_TREE

#include "base.hpp"
#include "../binding.hpp"
#include "../tree_adapter.hpp"
#include <cstdint>
#include "../optional.hpp"
#include <QAbstractItemModel>
#include <QTreeView>
#include <QPointer>
#include <memory>
#include <unordered_set>

namespace CPPUI { namespace Qt {
    class Tree;
    class TreeNode;
    class TreeModel;
    namespace detail {
        // QModelIndex points here, never to a replaceable CPPUI declaration.
        // TreeModel owns records; the declaration is an invalidated observer.
        struct TreeItem {
            TreeModel * model;
            TreeNode * owner;
            TreeItem * parent = nullptr;
            int row = 0;
            QString text;
            std::vector<TreeItem *> children;
            std::function<void()> clicked, doubleClicked, expanded, collapsed;
            TreeItem(TreeModel * model, TreeNode * owner): model(model), owner(owner) {}
        };
        inline void treeError(const char * operation) {
            try { throw; }
            catch(const std::exception & error) { std::cerr << "CPPUI Tree " << operation << ": " << error.what() << '\n'; }
            catch(...) { std::cerr << "CPPUI Tree " << operation << ": unknown exception\n"; }
        }
    }
    class TreeNode: public Node {
        friend class TreeModel;
        friend class Tree;
        detail::TreeItem * item = nullptr;
        std::string text;
        std::function<void()> onDoubleClicked, onExpanded, onCollapsed;
        void releaseItem();
        public:
        explicit TreeNode(const std::string & text = {}): text(text) {}
        explicit TreeNode(Binding<std::string> text): TreeNode(text.get()) {}
        ~TreeNode() override { releaseItem(); }
        TreeNode & dblclick(std::function<void()> callback) { onDoubleClicked = std::move(callback); return *this; }
        TreeNode & expand(std::function<void()> callback) { onExpanded = std::move(callback); return *this; }
        TreeNode & collapse(std::function<void()> callback) { onCollapsed = std::move(callback); return *this; }
        void update(Node * previous) override;
        void destroy(bool) override { releaseItem(); }
        void releaseRetiredRefs() override { releaseItem(); Node::releaseRetiredRefs(); }
        void addChild(int, Node *) override;
        void removeChild(int, Node *) override;
        void * outer() override { return nullptr; }
        void * inner() override { return nullptr; }
    };
    class TreeModel: public QAbstractItemModel {
        friend class Tree;
        Tree * owner = nullptr;
        std::vector<std::unique_ptr<detail::TreeItem>> records;
        std::vector<detail::TreeItem *> roots;
        std::unordered_set<detail::TreeItem *> active;
        bool dirty = false, queued = false, publishing = false;
        struct Snapshot {
            detail::TreeItem * item, * parent;
            int row;
            QString text;
            std::vector<detail::TreeItem *> children;
            std::function<void()> clicked, doubleClicked, expanded, collapsed;
        };
        void collect(const std::vector<Node *> & nodes, detail::TreeItem * parent,
                     std::vector<detail::TreeItem *> & siblings, std::vector<Snapshot> & snapshots) {
            for(auto node: nodes) {
                if(node->is_virtual) { collect(*node->children, parent, siblings, snapshots); continue; }
                auto declaration = dynamic_cast<TreeNode *>(node);
                if(!declaration || !declaration->item || declaration->item->model != this)
                    throw std::logic_error("Tree requires TreeNode children");
                Snapshot snapshot{declaration->item, parent, static_cast<int>(siblings.size()), QString::fromStdString(declaration->text), {},
                    declaration->onClicked, declaration->onDoubleClicked, declaration->onExpanded, declaration->onCollapsed};
                siblings.push_back(snapshot.item);
                collect(*declaration->children, snapshot.item, snapshot.children, snapshots);
                snapshots.push_back(std::move(snapshot));
            }
        }
        detail::TreeItem * item(const QModelIndex & index) const {
            if(!owner || !index.isValid() || index.model() != this || index.column() != 0) return nullptr;
            auto value = static_cast<detail::TreeItem *>(index.internalPointer());
            return active.count(value) ? value : nullptr;
        }
        public:
        explicit TreeModel(QObject * parent): QAbstractItemModel(parent) {}
        ~TreeModel() override {
            for(const auto & record: records) if(record->owner) record->owner->item = nullptr;
        }
        detail::TreeItem * create(TreeNode * node) {
            records.emplace_back(new detail::TreeItem(this, node));
            return records.back().get();
        }
        void requestSync() {
            if(!owner) return;
            dirty = true;
            if(queued) return;
            queued = true;
            QMetaObject::invokeMethod(this, [this]() {
                queued = false;
                try { synchronize(); } catch(...) { detail::treeError("sync"); }
            }, ::Qt::QueuedConnection);
        }
        QModelIndex index(int row, int column, const QModelIndex & parent = {}) const override {
            if(!owner || row < 0 || column != 0) return {};
            auto parentItem = item(parent);
            if(parent.isValid() && !parentItem) return {};
            const auto & siblings = parentItem ? parentItem->children : roots;
            return row < static_cast<int>(siblings.size()) ? createIndex(row, column, siblings[row]) : QModelIndex();
        }
        QModelIndex parent(const QModelIndex & index) const override {
            auto child = item(index);
            return child && child->parent ? createIndex(child->parent->row, 0, child->parent) : QModelIndex();
        }
        int rowCount(const QModelIndex & parent = {}) const override {
            if(!owner) return 0;
            auto parentItem = item(parent);
            if(parent.isValid() && !parentItem) return 0;
            return static_cast<int>((parentItem ? parentItem->children : roots).size());
        }
        int columnCount(const QModelIndex & parent = {}) const override { return owner && (!parent.isValid() || item(parent)) ? 1 : 0; }
        QVariant data(const QModelIndex & index, int role = ::Qt::DisplayRole) const override {
            auto node = item(index);
            return node && role == ::Qt::DisplayRole ? QVariant(node->text) : QVariant();
        }
        ::Qt::ItemFlags flags(const QModelIndex & index) const override { return item(index) ? QAbstractItemModel::flags(index) : ::Qt::NoItemFlags; }
        void synchronize();
        void deactivate() {
            owner = nullptr; dirty = false;
            beginResetModel(); roots.clear(); active.clear();
            for(auto & record: records) {
                if(record->owner) record->owner->item = nullptr;
                record->owner = nullptr;
                record->clicked = {}; record->doubleClicked = {}; record->expanded = {}; record->collapsed = {};
            }
            // A publish may still have stack-local indexes pointing into the
            // pool. The native model owns these until that call unwinds.
            if(!publishing) records.clear();
            endResetModel();
        }
        void dispatch(const QModelIndex & index, int event) {
            const auto record = item(index);
            if(!record || !record->owner) return;
            auto invoke = event == 0 ? record->clicked : event == 1 ? record->doubleClicked
                : event == 2 ? record->expanded : record->collapsed;
            try { if(invoke) invoke(); } catch(...) { detail::treeError("callback"); }
        }
    };
    class Tree: public BaseWidget {
        Optional<bool> expansion, expandableValue;
        std::shared_ptr<void> adapterOwner;
        protected:
        void clearNative() override {
            if(model && model->owner == this) model->owner = nullptr;
            model = nullptr; BaseWidget::clearNative();
        }
        public:
        TreeModel * model = nullptr;
        explicit Tree(std::shared_ptr<void> adapter = {}): adapterOwner(std::move(adapter)) { layout_weight = 1; }
        Tree & expandAll() { if(ui) static_cast<QTreeView *>(ui)->expandAll(); else expansion = true; return *this; }
        Tree & collapseAll() { if(ui) static_cast<QTreeView *>(ui)->collapseAll(); else expansion = false; return *this; }
        Tree & expandable(bool value) { if(ui) static_cast<QTreeView *>(ui)->setItemsExpandable(value); else expandableValue = value; return *this; }
        void update(BaseWidget * previous) override {
            auto prev = static_cast<Tree *>(previous);
            if(prev && prev->ui) { ui = prev->ui; model = prev->model; }
            else {
                auto tree = new Widget<QTreeView>(); ui = tree;
                model = new TreeModel(tree); tree->setModel(model); tree->setHeaderHidden(true);
            }
            model->owner = this; model->requestSync();
            auto tree = static_cast<QTreeView *>(ui); auto ownedModel = model;
            connectSignal(QObject::connect(tree, &QTreeView::clicked, model, [ownedModel](const QModelIndex & index) { ownedModel->dispatch(index, 0); }));
            connectSignal(QObject::connect(tree, &QTreeView::doubleClicked, model, [ownedModel](const QModelIndex & index) { ownedModel->dispatch(index, 1); }));
            connectSignal(QObject::connect(tree, &QTreeView::expanded, model, [ownedModel](const QModelIndex & index) { ownedModel->dispatch(index, 2); }));
            connectSignal(QObject::connect(tree, &QTreeView::collapsed, model, [ownedModel](const QModelIndex & index) { ownedModel->dispatch(index, 3); }));
        }
        void addChild(int, Node *) override { if(model) model->requestSync(); }
        void removeChild(int, Node *) override { if(model) model->requestSync(); }
        void postSync() override {
            if(!model) return;
            QPointer<QTreeView> tree(static_cast<QTreeView *>(ui));
            const auto expand = expansion, enable = expandableValue;
            model->synchronize();
            if(tree && enable) tree->setItemsExpandable(*enable);
            if(tree && expand) { if(*expand) tree->expandAll(); else tree->collapseAll(); }
        }
        void destroy(bool direct) override {
            disconnectSignals(); if(model) model->deactivate(); BaseWidget::destroy(direct);
        }
    };
    inline void TreeModel::synchronize() {
        if(!owner || !dirty || publishing) return;
        std::vector<Snapshot> snapshots;
        std::vector<detail::TreeItem *> nextRoots;
        collect(*owner->children, nullptr, nextRoots, snapshots);
        QPointer<TreeModel> alive(this);
        publishing = true; dirty = false;
        emit layoutAboutToBeChanged();
        if(!alive) return;
        if(!owner) { publishing = false; emit layoutChanged(); return; }
        const auto previous = persistentIndexList();
        roots = std::move(nextRoots); active.clear();
        for(auto & snapshot: snapshots) {
            auto record = snapshot.item;
            record->parent = snapshot.parent; record->row = snapshot.row;
            record->text = std::move(snapshot.text); record->children = std::move(snapshot.children);
            record->clicked = std::move(snapshot.clicked); record->doubleClicked = std::move(snapshot.doubleClicked);
            record->expanded = std::move(snapshot.expanded); record->collapsed = std::move(snapshot.collapsed);
            active.insert(record);
        }
        QModelIndexList current;
        for(const auto & index: previous) {
            auto record = static_cast<detail::TreeItem *>(index.internalPointer());
            current.push_back(active.count(record) ? createIndex(record->row, 0, record) : QModelIndex());
        }
        changePersistentIndexList(previous, current);
        emit layoutChanged();
        if(!alive) return;
        publishing = false;
        records.erase(std::remove_if(records.begin(), records.end(), [this](const std::unique_ptr<detail::TreeItem> & record) {
            return !record->owner && !active.count(record.get());
        }), records.end());
    }
    inline void TreeNode::update(Node * previous) {
        auto tree = dynamic_cast<Tree *>(dom_parent);
        auto parentItem = dynamic_cast<TreeNode *>(dom_parent);
        auto model = tree ? tree->model : parentItem && parentItem->item ? parentItem->item->model : nullptr;
        if(!model) throw std::logic_error("TreeNode requires a Tree or TreeNode parent");
        auto prev = static_cast<TreeNode *>(previous);
        if(prev && prev->item && prev->item->model == model) { item = prev->item; prev->item = nullptr; item->owner = this; }
        else item = model->create(this);
        model->requestSync(); Node::update(previous);
    }
    inline void TreeNode::releaseItem() {
        if(item) {
            item->owner = nullptr;
            item->model->requestSync();
            item = nullptr;
        }
    }
    inline void TreeNode::addChild(int, Node *) { if(item) item->model->requestSync(); }
    inline void TreeNode::removeChild(int, Node *) { if(item) item->model->requestSync(); }
    namespace detail {
        template<class T> struct AdapterTreeItem {
            std::shared_ptr<T> value;
            std::string text;
            std::vector<AdapterTreeItem> children;
        };
        template<class T> std::vector<AdapterTreeItem<T>> readTreeAdapter(
                const std::shared_ptr<BaseTreeAdapter<T>> & adapter, const std::shared_ptr<T> & parent,
                std::unordered_set<const T *> & seen) {
            const int count = adapter->rowCount(parent);
            if(count < 0) throw std::invalid_argument("Tree adapter row count must be non-negative");
            std::vector<AdapterTreeItem<T>> result;
            for(int row = 0; row < count; ++row) {
                auto node = adapter->child(parent, row);
                if(!node || !seen.insert(node.get()).second)
                    throw std::invalid_argument("Tree adapter children must be non-null, unique and acyclic");
                if(parent && adapter->parent(node) != parent)
                    throw std::invalid_argument("Tree adapter parent does not match its child hierarchy");
                AdapterTreeItem<T> item{node, adapter->data(node), {}};
                item.children = readTreeAdapter(adapter, node, seen);
                result.push_back(std::move(item));
            }
            return result;
        }
        template<class T> void buildTreeAdapter(const std::shared_ptr<BaseTreeAdapter<T>> & adapter,
                                                const std::vector<AdapterTreeItem<T>> & items) {
            for(const auto & item: items) {
                auto value = item.value;
                auto & node = *new TreeNode(item.text);
                // Source and data identities are stable while both are owned.
                // QModelIndex still points to TreeModel's records, not these addresses.
                node.id(std::to_string(reinterpret_cast<std::uintptr_t>(adapter.get())) + ":"
                    + std::to_string(reinterpret_cast<std::uintptr_t>(value.get())));
                node.click([adapter, value]() { adapter->clicked(value); });
                node.dblclick([adapter, value]() { adapter->dblclicked(value); });
                node.expand([adapter, value]() { adapter->expanded(value); });
                node.collapse([adapter, value]() { adapter->collapsed(value); });
                auto scope = node.begin();
                buildTreeAdapter(adapter, item.children);
            }
        }
    }
}}
namespace CPPUI {
    inline Qt::Tree & Tree() { return *new Qt::Tree(); }
    template<class Adapter> inline Qt::Tree & Tree(std::shared_ptr<Adapter> source) {
        using T = typename Adapter::Value;
        std::shared_ptr<BaseTreeAdapter<T>> adapter = std::move(source);
        if(!adapter) throw std::invalid_argument("Tree adapter must not be null");
        // Snapshot application data during construction, before changing native
        // indexes; State reads are tracked and failing getters leave the old UI.
        std::unordered_set<const T *> seen;
        auto items = Qt::detail::readTreeAdapter(adapter, std::shared_ptr<T>(), seen);
        auto & tree = *new Qt::Tree(adapter);
        auto scope = tree.begin();
        Qt::detail::buildTreeAdapter(adapter, items);
        return tree;
    }
    inline Qt::TreeNode & TreeNode(const std::string & text = {}) { return *new Qt::TreeNode(text); }
    inline Qt::TreeNode & TreeNode(Binding<std::string> text) { return *new Qt::TreeNode(text); }
    inline Qt::TreeNode & TreeNode(State<std::string> & text) { return TreeNode(Binding<std::string>(text)); }
}
#endif

#ifndef CPPUI_WX_TREE
#define CPPUI_WX_TREE
#include "base.hpp"
#include "../binding.hpp"
#include "../tree_adapter.hpp"
#include <wx/dataview.h>
#include <unordered_set>
#include <cstdint>
namespace CPPUI { namespace Wx {
namespace detail {
    struct TreeItem {
        TreeItem * parent=nullptr;
        std::vector<TreeItem *> children;
        std::string text;
        int row=0;
        std::function<void()> clicked,doubleClicked,expanded,collapsed;
    };
}
class TreeNode:public Node {
public:
    std::shared_ptr<detail::TreeItem> item;
    std::string text;
    std::function<void()> onDoubleClicked,onExpanded,onCollapsed;
    explicit TreeNode(const std::string & text={}):text(text){}
    explicit TreeNode(Binding<std::string> text):TreeNode(text.get()){}
    TreeNode & dblclick(std::function<void()> callback){onDoubleClicked=std::move(callback);return *this;}
    TreeNode & expand(std::function<void()> callback){onExpanded=std::move(callback);return *this;}
    TreeNode & collapse(std::function<void()> callback){onCollapsed=std::move(callback);return *this;}
    void update(::CPPUI::Node * previous) override {auto prev=static_cast<TreeNode *>(previous);item=prev&&prev->item?prev->item:std::make_shared<detail::TreeItem>();::CPPUI::Node::update(previous);}
    void destroy(bool) override {item.reset();}
    void releaseRetiredRefs() override {item.reset();Node::releaseRetiredRefs();}
    void * inner() override{return nullptr;}
    void * outer() override{return nullptr;}
    void addChild(int,::CPPUI::Node *) override{}
    void removeChild(int,::CPPUI::Node *) override{}
};
class TreeModel:public wxDataViewModel {
    std::unordered_set<detail::TreeItem *> active;
public:
    std::vector<std::shared_ptr<detail::TreeItem>> records;
    std::vector<detail::TreeItem *> roots;
    bool syncing=false,enabled=true,programmatic=false,live=true;
    detail::TreeItem * item(const wxDataViewItem & id) const {auto p=static_cast<detail::TreeItem *>(id.GetID());return live&&active.count(p)?p:nullptr;}
    unsigned int GetColumnCount() const override{return 1;}
    wxString GetColumnType(unsigned int) const override{return "string";}
    void GetValue(wxVariant & value,const wxDataViewItem & id,unsigned int) const override {auto p=item(id);value=p?wxString::FromUTF8(p->text):wxString();}
    bool SetValue(const wxVariant &,const wxDataViewItem &,unsigned int) override{return false;}
    wxDataViewItem GetParent(const wxDataViewItem & id) const override{auto p=item(id);return wxDataViewItem(p?p->parent:nullptr);}
    bool IsContainer(const wxDataViewItem & id) const override{auto p=item(id);return !id.IsOk()||(p&&!p->children.empty());}
    unsigned int GetChildren(const wxDataViewItem & id,wxDataViewItemArray & result) const override {auto p=item(id);if(id.IsOk()&&!p)return 0;const auto & children=p?p->children:roots;for(auto child:children)result.Add(wxDataViewItem(child));return children.size();}
    bool HasDefaultCompare() const override{return true;}
    int Compare(const wxDataViewItem & a,const wxDataViewItem & b,unsigned int,bool ascending) const override {auto x=item(a),y=item(b);return x&&y&&x->parent==y->parent?(x->row-y->row)*(ascending?1:-1):0;}
    void collect(const std::vector<::CPPUI::Node *> & nodes,detail::TreeItem * parent,std::vector<detail::TreeItem *> & siblings){
        for(auto node:nodes){if(node->is_virtual){collect(*node->children,parent,siblings);continue;}
            auto declaration=dynamic_cast<TreeNode *>(node);if(!declaration||!declaration->item)throw std::logic_error("Tree requires TreeNode children");
            auto record=declaration->item;record->parent=parent;record->row=siblings.size();record->text=declaration->text;record->children.clear();record->clicked=declaration->onClicked;record->doubleClicked=declaration->onDoubleClicked;record->expanded=declaration->onExpanded;record->collapsed=declaration->onCollapsed;
            records.push_back(record);active.insert(record.get());siblings.push_back(record.get());collect(*node->children,record.get(),record->children);
        }
    }
    void refresh(const std::vector<::CPPUI::Node *> & nodes,wxDataViewCtrl * ui){
        std::unordered_set<detail::TreeItem *> expanded;for(auto & record:records)if(ui->IsExpanded(wxDataViewItem(record.get())))expanded.insert(record.get());
        auto selected=ui->GetSelection();auto previous=std::move(records);records.clear();active.clear();roots.clear();
        syncing=true;collect(nodes,nullptr,roots);Cleared();
        for(auto & record:records)if(expanded.count(record.get()))ui->Expand(wxDataViewItem(record.get()));
        if(item(selected))ui->Select(selected);ui->GetColumn(0)->SetWidth(wxCOL_WIDTH_AUTOSIZE);syncing=false;
    }
    void dispatch(const wxDataViewItem & id,int event){auto record=item(id);if(!record||syncing)return;auto callback=event==0?record->clicked:event==1?record->doubleClicked:event==2?record->expanded:record->collapsed;try{if(callback)callback();}catch(...){reportException("Tree callback");}}
    void deactivate(){live=false;syncing=true;roots.clear();active.clear();auto previous=std::move(records);records.clear();Cleared();}
    void all(wxDataViewCtrl * ui,bool expand){programmatic=true;auto snapshot=records;if(!expand)std::reverse(snapshot.begin(),snapshot.end());for(auto & record:snapshot){if(!live)break;if(item(wxDataViewItem(record.get()))&&!record->children.empty()){if(expand)ui->Expand(wxDataViewItem(record.get()));else ui->Collapse(wxDataViewItem(record.get()));}}programmatic=false;}
};
class Tree:public Widget {
    Optional<bool> expansion,enabled;
    std::shared_ptr<void> adapterOwner;
public:
    TreeModel * model=nullptr;
    explicit Tree(std::shared_ptr<void> adapter={}):adapterOwner(std::move(adapter)){is_terminal=false;layout_weight=1;expandX=expandY=3;}
    Tree & expandAll(){if(ui)model->all(static_cast<wxDataViewCtrl *>(ui),true);else expansion=true;return *this;}
    Tree & collapseAll(){if(ui)model->all(static_cast<wxDataViewCtrl *>(ui),false);else expansion=false;return *this;}
    Tree & expandable(bool value){if(model)model->enabled=value;else enabled=value;return *this;}
    void invalidateNative() override{model=nullptr;Widget::invalidateNative();}
    void updateWidget(Widget * previous) override {
        auto prev=static_cast<Tree *>(previous);if(prev){ui=prev->ui;model=prev->model;return;}
        auto tree=new wxDataViewCtrl(parentWindow(),wxID_ANY,wxDefaultPosition,wxDefaultSize,wxDV_NO_HEADER);ui=tree;
        model=new TreeModel();tree->AssociateModel(model);model->DecRef();tree->AppendTextColumn("",0,wxDATAVIEW_CELL_INERT,wxCOL_WIDTH_AUTOSIZE);auto owned=model;
        tree->Bind(wxEVT_LEFT_UP,[owned,tree](wxMouseEvent & e){wxDataViewItem item;wxDataViewColumn * column;tree->HitTest(e.GetPosition(),item,column);e.Skip();owned->dispatch(item,0);});
        tree->Bind(wxEVT_LEFT_DCLICK,[owned,tree](wxMouseEvent & e){wxDataViewItem item;wxDataViewColumn * column;tree->HitTest(e.GetPosition(),item,column);e.Skip();owned->dispatch(item,1);});
        auto request=[owned](wxDataViewEvent & e){if(owned->enabled||owned->programmatic||owned->syncing)e.Skip();else e.Veto();};
        tree->Bind(wxEVT_DATAVIEW_ITEM_EXPANDING,request);tree->Bind(wxEVT_DATAVIEW_ITEM_COLLAPSING,request);
        tree->Bind(wxEVT_DATAVIEW_ITEM_EXPANDED,[owned](wxDataViewEvent & e){e.Skip();owned->dispatch(e.GetItem(),2);});
        tree->Bind(wxEVT_DATAVIEW_ITEM_COLLAPSED,[owned](wxDataViewEvent & e){e.Skip();owned->dispatch(e.GetItem(),3);});
    }
    void addChild(int,::CPPUI::Node *) override{}
    void removeChild(int,::CPPUI::Node *) override{}
    void postSync() override {if(!model)return;auto owned=model;auto tree=static_cast<wxDataViewCtrl *>(ui);auto expand=expansion;auto enable=enabled;owned->refresh(*children,tree);if(enable)owned->enabled=*enable;if(expand)owned->all(tree,*expand);}
    void destroy(bool direct) override {if(model)model->deactivate();Widget::destroy(direct);model=nullptr;}
};
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
                // Native items point to model-owned records, not declaration addresses.
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
    inline Wx::Tree & Tree() { return *new Wx::Tree(); }
    template<class Adapter> inline Wx::Tree & Tree(std::shared_ptr<Adapter> source) {
        using T = typename Adapter::Value;
        std::shared_ptr<BaseTreeAdapter<T>> adapter = std::move(source);
        if(!adapter) throw std::invalid_argument("Tree adapter must not be null");
        // Snapshot application data during construction, before changing native
        // indexes; State reads are tracked and failing getters leave the old UI.
        std::unordered_set<const T *> seen;
        auto items = Wx::detail::readTreeAdapter(adapter, std::shared_ptr<T>(), seen);
        auto & tree = *new Wx::Tree(adapter);
        auto scope = tree.begin();
        Wx::detail::buildTreeAdapter(adapter, items);
        return tree;
    }
    inline Wx::TreeNode & TreeNode(const std::string & text = {}) { return *new Wx::TreeNode(text); }
    inline Wx::TreeNode & TreeNode(Binding<std::string> text) { return *new Wx::TreeNode(text); }
    inline Wx::TreeNode & TreeNode(State<std::string> & text) { return TreeNode(Binding<std::string>(text)); }
}
#endif

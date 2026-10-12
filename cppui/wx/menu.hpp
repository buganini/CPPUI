#ifndef CPPUI_WX_MENU
#define CPPUI_WX_MENU
#include "base.hpp"
#include <wx/menu.h>
namespace CPPUI { namespace Wx {
class MenuObject: public Node {
public:
    void * inner() override {return nullptr;}
    void observe(MenuObject * previous) {
        transfer(previous);
        if(!previous) {native->owner=wxGetTopLevelParent(parentWindow());native->owner->Bind(wxEVT_DESTROY,&Native::destroyed,native.get());}
    }
};
class MenuAction: public MenuObject {
    std::function<void()> callback;
public:
    wxMenuItem * ui=nullptr;
    std::string text;
    explicit MenuAction(const std::string & text):text(text){is_terminal=true;}
    MenuAction & trigger(std::function<void()> action){callback=std::move(action);return *this;}
    void * outer() override {return ui;}
    void invalidateNative() override {ui=nullptr;}
    void update(::CPPUI::Node * previous) override {
        auto prev=static_cast<MenuAction *>(previous);bool reuse=prev&&prev->ui;
        ui=reuse?prev->ui:new wxMenuItem(nullptr,wxWindow::NewControlId(),wxString::FromUTF8(text));
        ui->SetItemLabel(wxString::FromUTF8(text));observe(reuse?prev:nullptr);native->command=callback;
        if(!reuse)native->owner->Bind(wxEVT_MENU,&Native::clicked,native.get(),ui->GetId());
    }
    void releaseRetiredRefs() override {ui=nullptr;callback={};Node::releaseRetiredRefs();}
    void destroy(bool direct) override {if(native){native->node=nullptr;native->command={};}if(direct)delete ui;ui=nullptr;}
};
class Menu: public MenuObject {
    struct Item {::CPPUI::Node * node;wxMenuItem * item;};
    std::vector<Item> items;
public:
    wxMenu * ui=nullptr;
    std::string text;
    explicit Menu(const std::string & text):text(text){}
    void * outer() override{return ui;}
    void invalidateNative() override{ui=nullptr;items.clear();}
    void update(::CPPUI::Node * previous) override {
        auto prev=static_cast<Menu *>(previous);bool reuse=prev&&prev->ui;
        ui=reuse?prev->ui:new wxMenu();if(reuse)items=std::move(prev->items);observe(reuse?prev:nullptr);
    }
    void addChild(int index,::CPPUI::Node * child) override {
        wxMenuItem * item=nullptr;
        if(auto action=dynamic_cast<MenuAction *>(child))item=action->ui;
        else if(auto menu=dynamic_cast<Menu *>(child))item=new wxMenuItem(ui,wxWindow::NewControlId(),wxString::FromUTF8(menu->text),"",wxITEM_NORMAL,menu->ui);
        else throw std::logic_error("Menu accepts Menu or MenuAction");
        ui->Insert(index,item);items.insert(items.begin()+index,{child,item});
    }
    void removeChild(int index,::CPPUI::Node * child) override {
        if(!ui)return;auto item=items.at(index).item;ui->Remove(item);items.erase(items.begin()+index);
        if(dynamic_cast<Menu *>(child)){item->SetSubMenu(nullptr);delete item;}
    }
    void postSync() override {
        if(!ui)return;for(auto & item:items){item.node=item.node->getNode();
            auto action=dynamic_cast<MenuAction *>(item.node);auto menu=dynamic_cast<Menu *>(item.node);
            item.item->SetItemLabel(wxString::FromUTF8(action?action->text:menu->text));}
    }
    void releaseRetiredRefs() override {ui=nullptr;items.clear();Node::releaseRetiredRefs();}
    void destroy(bool direct) override {if(native)native->node=nullptr;if(direct)delete ui;ui=nullptr;items.clear();}
};
class MenuBar: public MenuObject {
    std::vector<Menu *> menus;
public:
    wxMenuBar * ui=nullptr;
    MenuBar(){is_outoforder=true;}
    void * outer() override{return ui;}
    void invalidateNative() override{ui=nullptr;menus.clear();}
    void update(::CPPUI::Node * previous) override {
        auto prev=static_cast<MenuBar *>(previous);bool reuse=prev&&prev->ui;
        ui=reuse?prev->ui:new wxMenuBar();if(reuse)menus=std::move(prev->menus);observe(reuse?prev:nullptr);
    }
    void addChild(int index,::CPPUI::Node * child) override {
        auto menu=dynamic_cast<Menu *>(child);if(!menu)throw std::logic_error("MenuBar accepts Menu declarations");
        ui->Insert(index,menu->ui,wxString::FromUTF8(menu->text));menus.insert(menus.begin()+index,menu);
    }
    void removeChild(int index,::CPPUI::Node *) override {if(ui){ui->Remove(index);menus.erase(menus.begin()+index);}}
    void postSync() override {if(ui)for(std::size_t i=0;i<menus.size();++i){menus[i]=static_cast<Menu *>(menus[i]->getNode());ui->SetMenuLabel(i,wxString::FromUTF8(menus[i]->text));}}
    void releaseRetiredRefs() override {ui=nullptr;menus.clear();Node::releaseRetiredRefs();}
    void destroy(bool direct) override {if(native)native->node=nullptr;if(direct)delete ui;ui=nullptr;menus.clear();}
};
}
inline Wx::MenuBar & MenuBar(){return *new Wx::MenuBar();}
inline Wx::Menu & Menu(const std::string & text){return *new Wx::Menu(text);}
inline Wx::MenuAction & MenuAction(const std::string & text){return *new Wx::MenuAction(text);}
}
#endif

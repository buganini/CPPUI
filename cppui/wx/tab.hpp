#ifndef CPPUI_WX_TAB
#define CPPUI_WX_TAB
#include "base.hpp"
#include "../side.hpp"
#include <wx/notebook.h>
namespace CPPUI { namespace Wx {
// PUI/wx uses a native page panel, allowing either a sizer or a widget as content.
class Tab: public SingleChild {
public:
    bool supportsVisibility() const override { return false; }
            std::string label;
    explicit Tab(const std::string & label):label(label){}
    void updateWidget(Widget * prev) override {ui=prev?prev->ui:new wxPanel(parentWindow());panel=ui;}
};
class Tabs: public Widget {
    std::vector<Tab *> pages;
    wxWindow * selected=nullptr;
    Side position;
    static long flags(Side position) {
        if(position==Side::Top)return wxNB_TOP;if(position==Side::Bottom)return wxNB_BOTTOM;
        if(position==Side::Right)return wxNB_RIGHT;if(position==Side::Left)return wxNB_LEFT;
        throw std::invalid_argument("Unknown tab position");
    }
public:
    explicit Tabs(Side position=Side::Top):position(position){flags(position);is_terminal=false;expandX=expandY=2;}
    bool refSelection() const override { return true; }
    bool refEmptySelection() const override { return true; }
    void refCommand(RefCommand,const OptionalText & key,bool) override {
        if(!ui || pages.empty()) return;
        int index=0;
        for(std::size_t i=0;i<pages.size();++i) {
            auto page=static_cast<Tab *>(pages[i]->getNode());
            if(key && (page->has_id && !page->_id.empty() ? page->_id : page->label)==*key) { index=i; break; }
        }
        static_cast<wxNotebook *>(ui)->SetSelection(index);
    }
    std::size_t typecode() const override {return Widget::typecode() ^ static_cast<std::size_t>(position);}
    void updateWidget(Widget * previous) override {
        auto prev=static_cast<Tabs *>(previous);ui=prev?prev->ui:new wxNotebook(parentWindow(),wxID_ANY,wxDefaultPosition,wxDefaultSize,flags(position));
        if(prev)pages=std::move(prev->pages);
    }
    void preSync() override {auto notebook=static_cast<wxNotebook *>(ui);selected=notebook->GetCurrentPage();}
    void addChild(int index,::CPPUI::Node * child) override {
        if(child->is_outoforder)return;
        auto tab=dynamic_cast<Tab *>(child);if(!tab)throw std::logic_error("wx Tabs children must be Tab declarations");
        static_cast<wxNotebook *>(ui)->InsertPage(index,tab->ui,wxString::FromUTF8(tab->label));pages.insert(pages.begin()+index,tab);
    }
    void removeChild(int index,::CPPUI::Node * child) override {
        if(child->is_outoforder || !ui)return;
        static_cast<wxNotebook *>(ui)->RemovePage(index);pages.erase(pages.begin()+index);
    }
    void postSync() override {
        if(!ui)return;auto notebook=static_cast<wxNotebook *>(ui);
        for(std::size_t i=0;i<pages.size();++i){pages[i]=static_cast<Tab *>(pages[i]->getNode());notebook->SetPageText(i,wxString::FromUTF8(pages[i]->label));if(pages[i]->ui==selected)notebook->ChangeSelection(i);}
        ui->Layout();
    }
};
}
inline Wx::Tabs & Tabs(Side position=Side::Top){return *new Wx::Tabs(position);}
inline Wx::Tab & Tab(const std::string & label){return *new Wx::Tab(label);}
}
#endif

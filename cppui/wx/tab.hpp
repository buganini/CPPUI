#ifndef CPPUI_WX_TAB
#define CPPUI_WX_TAB
#include "base.hpp"
#include <wx/notebook.h>
namespace CPPUI { namespace Wx {
// PUI/wx uses a native page panel, allowing either a sizer or a widget as content.
class Tab: public SingleChild {
public:
    std::string label;
    explicit Tab(const std::string & label):label(label){}
    void updateWidget(Widget * prev) override {ui=prev?prev->ui:new wxPanel(parentWindow());panel=ui;}
};
class Tabs: public Widget {
    std::vector<Tab *> pages;
    wxWindow * selected=nullptr;
    std::string position;
    static long flags(const std::string & position) {
        if(position=="n")return wxNB_TOP;if(position=="s")return wxNB_BOTTOM;
        if(position=="e")return wxNB_RIGHT;if(position=="w")return wxNB_LEFT;
        throw std::invalid_argument("Unknown tab position");
    }
public:
    explicit Tabs(const std::string & position="n"):position(position){flags(position);is_terminal=false;expandX=expandY=2;}
    std::size_t typecode() const override {return Widget::typecode() ^ std::hash<std::string>()(position);}
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
inline Wx::Tabs & Tabs(const std::string & position="n"){return *new Wx::Tabs(position);}
inline Wx::Tab & Tab(const std::string & label){return *new Wx::Tab(label);}
}
#endif

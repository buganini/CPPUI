#ifndef CPPUI_COOKBOOK_WX_BACKEND
#define CPPUI_COOKBOOK_WX_BACKEND
#include "cppui/wx/application.hpp"
#include "cppui/wx/window.hpp"
#include "cppui/wx/layout.hpp"
#include "cppui/wx/grid.hpp"
#include "cppui/wx/divider.hpp"
#include "cppui/wx/label.hpp"
#include "cppui/wx/button.hpp"
#include "cppui/wx/checkbox.hpp"
#include "cppui/wx/progressbar.hpp"
#include "cppui/wx/canvas.hpp"
#include "cppui/wx/tab.hpp"
#include "cppui/wx/table.hpp"
#include "cppui/wx/tree.hpp"
#include "cppui/wx/dialog.hpp"
#include "cppui/wx/image.hpp"
#include "cppui/wx/textfield.hpp"
#include "cppui/wx/radiobutton.hpp"
#include "cppui/wx/combobox.hpp"
#include "cppui/wx/scroll.hpp"
#include "cppui/wx/timeline.hpp"
#include "../recipes.hpp"
#include <wx/stopwatch.h>
namespace Cookbook { namespace Backend {
    inline bool viewSelected(CPPUI::Node &, const std::string &) { return false; }
    const char * const name="wxWidgets";
    inline void configureWindow(CPPUI::Context * node){static_cast<CPPUI::Wx::Window *>(node)->title("CPPUI Cookbook").maximize();}
    inline void menus(){
        for(auto bar:CPPUI::MenuBar()){
            for(auto menu:CPPUI::Menu("Menu")){
                CPPUI::MenuAction("Action").trigger([](){std::cout<<"action\n";});CPPUI::MenuAction("Action2").trigger([](){std::cout<<"action2\n";});
                for(auto sub:CPPUI::Menu("Menu2")){CPPUI::MenuAction("Action3").trigger([](){std::cout<<"action3\n";});CPPUI::MenuAction("Action4").trigger([](){std::cout<<"action4\n";});}
            }
            for(auto menu:CPPUI::Menu("Menu3")){CPPUI::MenuAction("Action5");CPPUI::MenuAction("Action6");}
        }
    }
    void configureRecipes(std::vector<Recipe> & pages);
    inline void mount(CPPUI::Application & app){app.sync();}
    inline void settle(){wxStopWatch clock;do{wxTheApp->Yield(true);wxTheApp->ProcessPendingEvents();wxTheApp->ProcessIdle();wxMilliSleep(1);}while(clock.Time()<25);}
    inline void command(wxWindow * window,wxEventType type){wxCommandEvent e(type,window->GetId());e.SetEventObject(window);window->ProcessWindowEvent(e);}
    inline bool clickButton(CPPUI::Node & node,const std::string & text){auto button=dynamic_cast<CPPUI::Wx::Button *>(&node);if(!button||button->text!=text)return false;command(button->ui,wxEVT_BUTTON);return true;}
    inline bool labelHasText(CPPUI::Node & node,const std::string & text){auto label=dynamic_cast<CPPUI::Wx::Label *>(&node);return label&&label->ui&&(label->selectable?static_cast<wxTextCtrl *>(label->ui)->GetValue():label->ui->GetLabel())==wxString::FromUTF8(text);}
    inline bool selectTab(CPPUI::Node & node,int index,const std::string & text){auto tabs=dynamic_cast<CPPUI::Wx::Tabs *>(&node);if(!tabs||!tabs->ui)return false;auto ui=static_cast<wxNotebook *>(tabs->ui);if(index<0||index>=int(ui->GetPageCount())||ui->GetPageText(index)!=wxString::FromUTF8(text))return false;ui->SetSelection(index);return ui->GetSelection()==index&&ui->GetPage(index)->IsShown();}
    inline bool toggleCheckbox(CPPUI::Node & node){auto box=dynamic_cast<CPPUI::Wx::Checkbox *>(&node);if(!box||!box->ui)return false;auto ui=static_cast<wxCheckBox *>(box->ui);ui->SetValue(!ui->GetValue());command(ui,wxEVT_CHECKBOX);return true;}
    inline bool selectRadio(CPPUI::Node & node){auto widget=dynamic_cast<CPPUI::Wx::Widget *>(&node);auto radio=widget?dynamic_cast<wxRadioButton *>(widget->ui):nullptr;if(!radio)return false;radio->SetValue(true);command(radio,wxEVT_RADIOBUTTON);return true;}
    inline bool selectCombo(CPPUI::Node & node,int index){auto combo=dynamic_cast<CPPUI::Wx::ComboBox *>(&node);if(!combo||!combo->ui)return false;auto ui=static_cast<wxComboBox *>(combo->ui);ui->SetSelection(index);command(ui,wxEVT_COMBOBOX);return true;}
    inline bool editTextField(CPPUI::Node & node,const std::string & text,bool commit){auto field=dynamic_cast<CPPUI::Wx::TextField *>(&node);if(!field||!field->ui)return false;auto ui=static_cast<wxTextCtrl *>(field->ui);ui->SetValue(wxString::FromUTF8(text));if(commit)command(ui,wxEVT_TEXT_ENTER);return true;}
    inline bool atScrollEnd(CPPUI::Node & node){auto scroll=dynamic_cast<CPPUI::Wx::Scroll *>(&node);if(!scroll||!scroll->ui)return false;auto ui=static_cast<wxScrolledWindow *>(scroll->ui);int x,y;ui->GetViewStart(&x,&y);int max=std::max(0,ui->GetVirtualSize().y-ui->GetClientSize().y);return max>0&&y>=max-1;}
    inline wxDataViewItem treeIndex(CPPUI::Wx::Tree & tree,const std::vector<int> & path){wxDataViewItem item;for(int row:path){wxDataViewItemArray children;tree.model->GetChildren(item,children);if(row<0||row>=int(children.size()))return {};item=children[row];}return item;}
    inline int treeChildCount(CPPUI::Node & node,const std::vector<int> & path){auto tree=dynamic_cast<CPPUI::Wx::Tree *>(&node);if(!tree||!tree->model)return -1;wxDataViewItemArray children;auto item=treeIndex(*tree,path);if(!path.empty()&&!item.IsOk())return -1;return tree->model->GetChildren(item,children);}
    inline bool treeExpanded(CPPUI::Node & node,const std::vector<int> & path){auto tree=dynamic_cast<CPPUI::Wx::Tree *>(&node);return tree&&tree->model&&static_cast<wxDataViewCtrl *>(tree->ui)->IsExpanded(treeIndex(*tree,path));}
    inline std::string tableText(CPPUI::Node & node,int row,int col){auto table=dynamic_cast<CPPUI::Wx::Table *>(&node);return table&&table->model?table->model->GetValue(row,col).ToStdString(wxConvUTF8):std::string();}
    inline bool editTable(CPPUI::Node & node,int row,int col,const std::string & value){auto table=dynamic_cast<CPPUI::Wx::Table *>(&node);return table&&table->model&&table->model->edit(row,col,value);}
    class PromptAnswer:public wxTimer {
        std::string text;bool accepted;
    public:
        PromptAnswer(const std::string & text,bool accepted):text(text),accepted(accepted){Start(5);}
        void Notify() override{for(auto window:wxTopLevelWindows)if(auto dialog=dynamic_cast<wxTextEntryDialog *>(window))if(dialog->IsModal()){Stop();dialog->SetValue(wxString::FromUTF8(text));dialog->EndModal(accepted?wxID_OK:wxID_CANCEL);delete this;return;}}
    };
    inline void answerPrompt(const std::string & text,bool accepted){new PromptAnswer(text,accepted);}
    inline bool modalVisible(CPPUI::Node &){return false;}
    inline bool selectPage(CPPUI::Node & node){auto link=dynamic_cast<CPPUI::Wx::Label *>(&node);if(!link||!link->ui)return false;
        for(auto parent=link->ui->GetParent();parent;parent=parent->GetParent())if(auto scroll=dynamic_cast<wxScrolledWindow *>(parent)){int x,y;scroll->GetViewStart(&x,&y);auto point=scroll->ScreenToClient(link->ui->ClientToScreen(wxPoint(0,0)));scroll->Scroll(x,std::max(0,y+point.y));}
        wxMouseEvent event(wxEVT_LEFT_DOWN);event.SetEventObject(link->ui);link->ui->ProcessWindowEvent(event);return true;}
    inline void prepareLayoutCheck(CPPUI::Node & root){auto frame=static_cast<wxFrame *>(find(root,"cookbook")->outer());if(frame->GetTitle()!="CPPUI Cookbook")throw std::runtime_error("cookbook window title missing");frame->Maximize(false);frame->SetSize(800,600);settle();}
    inline bool pageVisible(CPPUI::Node & root,const std::string & key){auto scroll=static_cast<wxScrolledWindow *>(find(root,"menu-scroller")->outer());auto label=static_cast<wxWindow *>(find(root,"page:"+key)->outer());auto center=scroll->ScreenToClient(label->ClientToScreen(wxPoint(label->GetSize().x/2,label->GetSize().y/2)));return wxRect(wxPoint(0,0),scroll->GetClientSize()).Contains(center);}
    inline void checkLayout(CPPUI::Node & root){auto frame=static_cast<wxFrame *>(find(root,"cookbook")->outer());auto sidebar=static_cast<CPPUI::Wx::Layout *>(find(root,"menu-container"))->sizer();auto result=static_cast<CPPUI::Wx::Layout *>(find(root,"result"))->sizer();auto scroll=static_cast<wxScrolledWindow *>(find(root,"menu-scroller")->outer());
        if(sidebar->GetPosition().x+sidebar->GetSize().x>result->GetPosition().x||result->GetSize().x<=sidebar->GetSize().x||result->GetSize().y<200)throw std::runtime_error("cookbook must retain a sidebar beside an expanding result pane");
        if(scroll->GetVirtualSize().y<=scroll->GetClientSize().y)throw std::runtime_error("cookbook page list must scroll");
        auto bar=frame->GetMenuBar();if(!bar||bar->GetMenuCount()!=2||bar->GetMenu(0)->GetMenuItemCount()!=3||bar->GetMenu(1)->GetMenuItemCount()!=2)throw std::runtime_error("reference menus missing");
        if(find(root,"code"))throw std::runtime_error("source code pane must be omitted");
    }
}}
#endif

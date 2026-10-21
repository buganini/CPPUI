#include "../cookbook_backend.hpp"
#include <stdexcept>
#include <wx/stopwatch.h>

namespace Cookbook { namespace TestBackend {
    bool viewSelected(CPPUI::Node & node, const std::string & key) {
        auto views = dynamic_cast<Views *>(&node);
        if(!views || !views->ui) return false;
        const int index = static_cast<wxSimplebook *>(views->ui)->GetSelection();
        return key.empty() ? index == 0 : index > 0 && views->children->at(index - 1)->_id == key;
    }
    bool wheelCanvas(CPPUI::Node & node,int delta){
        auto canvas=dynamic_cast<CPPUI::Wx::Canvas *>(&node);if(!canvas||!canvas->ui)return false;
        auto widget=canvas->ui;widget->Refresh();widget->Update();
        wxMouseEvent event(wxEVT_MOUSEWHEEL);event.SetEventObject(widget);
        event.SetPosition(wxPoint(widget->GetClientSize().x/2,100));event.m_wheelRotation=delta;event.m_wheelDelta=120;
        widget->ProcessWindowEvent(event);return true;
    }
    void settle(){wxStopWatch clock;do{wxTheApp->Yield(true);wxTheApp->ProcessPendingEvents();wxTheApp->ProcessIdle();wxMilliSleep(1);}while(clock.Time()<25);}
    void command(wxWindow * window,wxEventType type){wxCommandEvent e(type,window->GetId());e.SetEventObject(window);window->ProcessWindowEvent(e);}
    bool clickButton(CPPUI::Node & node,const std::string & text){auto button=dynamic_cast<CPPUI::Wx::Button *>(&node);if(!button||button->text!=text)return false;command(button->ui,wxEVT_BUTTON);return true;}
    bool labelHasText(CPPUI::Node & node,const std::string & text) {
        auto label=dynamic_cast<CPPUI::Wx::Label *>(&node);
        if(!label || !label->ui) return false;
        const auto value=label->selectable ? static_cast<wxTextCtrl *>(label->ui)->GetValue()
            : dynamic_cast<CPPUI::Wx::Text *>(label) ? static_cast<wxStaticText *>(label->ui)->GetLabelText() : label->ui->GetLabel();
        return value==wxString::FromUTF8(text);
    }
    bool selectTab(CPPUI::Node & node,int index,const std::string & text){auto tabs=dynamic_cast<CPPUI::Wx::Tabs *>(&node);if(!tabs||!tabs->ui)return false;auto ui=static_cast<wxNotebook *>(tabs->ui);if(index<0||index>=int(ui->GetPageCount())||ui->GetPageText(index)!=wxString::FromUTF8(text))return false;ui->SetSelection(index);return ui->GetSelection()==index&&ui->GetPage(index)->IsShown();}
    bool toggleCheckbox(CPPUI::Node & node){auto box=dynamic_cast<CPPUI::Wx::Checkbox *>(&node);if(!box||!box->ui)return false;auto ui=static_cast<wxCheckBox *>(box->ui);ui->SetValue(!ui->GetValue());command(ui,wxEVT_CHECKBOX);return true;}
    bool selectRadio(CPPUI::Node & node){auto widget=dynamic_cast<CPPUI::Wx::Widget *>(&node);auto radio=widget?dynamic_cast<wxRadioButton *>(widget->ui):nullptr;if(!radio)return false;radio->SetValue(true);command(radio,wxEVT_RADIOBUTTON);return true;}
    bool selectCombo(CPPUI::Node & node,int index){auto combo=dynamic_cast<CPPUI::Wx::ComboBox *>(&node);if(!combo||!combo->ui)return false;auto ui=static_cast<wxComboBox *>(combo->ui);ui->SetSelection(index);command(ui,wxEVT_COMBOBOX);return true;}
    bool typeCombo(CPPUI::Node & node,const std::string & text){auto combo=dynamic_cast<CPPUI::Wx::ComboBox *>(&node);if(!combo||!combo->ui)return false;static_cast<wxComboBox *>(combo->ui)->SetValue(wxString::FromUTF8(text));return true;}
    bool editTextField(CPPUI::Node & node,const std::string & text,bool commit){auto field=dynamic_cast<CPPUI::Wx::TextField *>(&node);if(!field||!field->ui)return false;auto ui=static_cast<wxTextCtrl *>(field->ui);ui->SetValue(wxString::FromUTF8(text));if(commit)command(ui,wxEVT_TEXT_ENTER);return true;}
    bool atScrollEnd(CPPUI::Node & node){auto scroll=dynamic_cast<CPPUI::Wx::Scroll *>(&node);if(!scroll||!scroll->ui)return false;auto ui=static_cast<wxScrolledWindow *>(scroll->ui);int x,y;ui->GetViewStart(&x,&y);int max=std::max(0,ui->GetVirtualSize().y-ui->GetClientSize().y);return max>0&&y>=max-1;}
    wxDataViewItem treeIndex(CPPUI::Wx::Tree & tree,const std::vector<int> & path){wxDataViewItem item;for(int row:path){wxDataViewItemArray children;tree.model->GetChildren(item,children);if(row<0||row>=int(children.size()))return {};item=children[row];}return item;}
    int treeChildCount(CPPUI::Node & node,const std::vector<int> & path){auto tree=dynamic_cast<CPPUI::Wx::Tree *>(&node);if(!tree||!tree->model)return -1;wxDataViewItemArray children;auto item=treeIndex(*tree,path);if(!path.empty()&&!item.IsOk())return -1;return tree->model->GetChildren(item,children);}
    bool treeExpanded(CPPUI::Node & node,const std::vector<int> & path){auto tree=dynamic_cast<CPPUI::Wx::Tree *>(&node);return tree&&tree->model&&static_cast<wxDataViewCtrl *>(tree->ui)->IsExpanded(treeIndex(*tree,path));}
    std::string tableText(CPPUI::Node & node,int row,int col){auto table=dynamic_cast<CPPUI::Wx::Table *>(&node);return table&&table->model?table->model->GetValue(row,col).ToStdString(wxConvUTF8):std::string();}
    bool editTable(CPPUI::Node & node,int row,int col,const std::string & value){auto table=dynamic_cast<CPPUI::Wx::Table *>(&node);return table&&table->model&&table->model->edit(row,col,value);}
    class PromptAnswer:public wxTimer {
        std::string text;bool accepted;
    public:
        PromptAnswer(const std::string & text,bool accepted):text(text),accepted(accepted){Start(5);}
        void Notify() override{for(auto window:wxTopLevelWindows)if(auto dialog=dynamic_cast<wxTextEntryDialog *>(window))if(dialog->IsModal()){Stop();dialog->SetValue(wxString::FromUTF8(text));dialog->EndModal(accepted?wxID_OK:wxID_CANCEL);delete this;return;}}
    };
    void answerPrompt(const std::string & text,bool accepted){new PromptAnswer(text,accepted);}
    bool modalVisible(CPPUI::Node &){return false;}
    bool pageActive(CPPUI::Node & node){return static_cast<wxWindow *>(node.outer())->GetFont().GetWeight()==wxFONTWEIGHT_BOLD;}
    bool selectPage(CPPUI::Node & node){auto link=dynamic_cast<CPPUI::Wx::Label *>(&node);if(!link||!link->ui)return false;
        for(auto parent=link->ui->GetParent();parent;parent=parent->GetParent())if(auto scroll=dynamic_cast<wxScrolledWindow *>(parent)){int x,y;scroll->GetViewStart(&x,&y);auto point=scroll->ScreenToClient(link->ui->ClientToScreen(wxPoint(0,0)));scroll->Scroll(x,std::max(0,y+point.y));}
        wxMouseEvent event(wxEVT_LEFT_DOWN);event.SetEventObject(link->ui);link->ui->ProcessWindowEvent(event);return true;}
    void prepareLayoutCheck(CPPUI::Node & root){auto frame=static_cast<wxFrame *>(find(root,"cookbook")->outer());
        if(frame->IsMaximized() || frame->GetSize()!=wxSize(1024,768) || frame->GetTitle()!=wxString::FromUTF8(title))
            throw std::runtime_error("cookbook should open at 1024x768 without maximizing, with its window title");
        checkLayout(root);
        frame->SetSize(800,600);
        // The window manager may deliver resize after the first idle
        // cycle. Wait for the native client size before checking child layout.
        wxStopWatch clock;
        do {
            settle(); frame->Layout();
            auto panel=static_cast<CPPUI::Wx::Window *>(find(root,"cookbook"))->panel;
            panel->Layout();
            auto result=static_cast<CPPUI::Wx::Layout *>(find(root,"result"))->sizer();
            if(result->GetSize().x>206 && result->GetSize().y>=200) break;
        } while(clock.Time()<1000);
    }
    void resizeWindow(CPPUI::Node & root,int width,int height) {
        static_cast<wxFrame *>(find(root,"cookbook")->outer())->SetClientSize(width,height);
    }
    void resizeCodePane(CPPUI::Node & root,int width) {
        auto host=static_cast<wxWindow *>(find(root,"example-panes")->outer());
        auto panes=static_cast<wxSplitterWindow *>(host->GetSizer()->GetItem(std::size_t(0))->GetWindow());
        panes->SetSashPosition(width);panes->UpdateSize();
    }
    bool pageVisible(CPPUI::Node & root,const std::string & key){auto scroll=static_cast<wxScrolledWindow *>(find(root,"menu-scroller")->outer());auto label=static_cast<wxWindow *>(find(root,"page:"+key)->outer());auto center=scroll->ScreenToClient(label->ClientToScreen(wxPoint(label->GetSize().x/2,label->GetSize().y/2)));return wxRect(wxPoint(0,0),scroll->GetClientSize()).Contains(center);}
    void checkLayout(CPPUI::Node & root){auto frame=static_cast<wxFrame *>(find(root,"cookbook")->outer());auto sidebar=static_cast<CPPUI::Wx::Layout *>(find(root,"menu-container"))->sizer();auto result=static_cast<CPPUI::Wx::Layout *>(find(root,"result"))->sizer();auto scroll=static_cast<wxScrolledWindow *>(find(root,"menu-scroller")->outer());
        auto code=static_cast<CPPUI::Wx::Layout *>(find(root,"code"))->sizer();
        auto host=static_cast<wxWindow *>(find(root,"example-panes")->outer());
        auto panes=dynamic_cast<wxSplitterWindow *>(host->GetSizer()->GetItem(std::size_t(0))->GetWindow());
        const auto codeOrigin=code->GetContainingWindow()->ClientToScreen(code->GetPosition());
        const auto resultOrigin=result->GetContainingWindow()->ClientToScreen(result->GetPosition());
        const auto sidebarOrigin=sidebar->GetContainingWindow()->ClientToScreen(sidebar->GetPosition());
        if(!panes || !panes->IsSplit() || panes->GetSplitMode()!=wxSPLIT_VERTICAL
           || sidebarOrigin.x+sidebar->GetSize().x>codeOrigin.x || codeOrigin.x+code->GetSize().x>resultOrigin.x
           || code->GetSize().x<100 || result->GetSize().x<150 || result->GetSize().y<200)
            throw std::runtime_error("cookbook Code/Result geometry: code="+std::to_string(code->GetSize().x)+
                "; result="+std::to_string(result->GetSize().x)+"x"+std::to_string(result->GetSize().y)+
                "; sidebar end="+std::to_string(sidebarOrigin.x+sidebar->GetSize().x)+
                "; code start="+std::to_string(codeOrigin.x)+"; result start="+std::to_string(resultOrigin.x));
        if(scroll->GetVirtualSize().y<=scroll->GetClientSize().y)throw std::runtime_error("cookbook page list must scroll");
        auto bar=frame->GetMenuBar();if(!bar||bar->GetMenuCount()!=2||bar->GetMenu(0)->GetMenuItemCount()!=3||bar->GetMenu(1)->GetMenuItemCount()!=2)throw std::runtime_error("reference menus missing");
        if(auto node=find(root,"example-splitter")) {
            auto host=static_cast<wxWindow *>(node->outer());
            auto outer=dynamic_cast<wxSplitterWindow *>(host->GetSizer()->GetItem(std::size_t(0))->GetWindow());
            auto innerHost=static_cast<wxWindow *>(find(root,"splitter-horizontal")->outer());
            auto inner=dynamic_cast<wxSplitterWindow *>(innerHost->GetSizer()->GetItem(std::size_t(0))->GetWindow());
            auto tail=inner ? dynamic_cast<wxSplitterWindow *>(inner->GetWindow2()) : nullptr;
            if(!outer || !outer->IsSplit() || outer->GetSplitMode()!=wxSPLIT_HORIZONTAL
               || !inner || !tail || !inner->IsSplit() || !tail->IsSplit()
               || inner->GetSplitMode()!=wxSPLIT_VERTICAL || tail->GetSplitMode()!=wxSPLIT_VERTICAL
               || inner->GetWindow1()->GetSize().x<=0 || tail->GetWindow2()->GetSize().x<=0)
                throw std::runtime_error("Splitter recipe must mount three horizontal panes inside a vertical split");
        }
    }
}}

#include "backend.hpp"
#include <iostream>
#include <stdexcept>
#if defined(CPPUI_COOKBOOK_QT)
#include <QTimer>
#elif defined(CPPUI_COOKBOOK_WX)
#include <wx/stopwatch.h>
#elif CPPUI_LVGL_SDL2
#include <SDL.h>
#elif CPPUI_LVGL_X11
#include <X11/Xlib.h>
#endif

namespace Cookbook { namespace Backend {
#if defined(CPPUI_COOKBOOK_LVGL) && CPPUI_LVGL_SDL2
    const std::string name = std::string(Native::backendName) + " SDL2";
#elif defined(CPPUI_COOKBOOK_LVGL) && CPPUI_LVGL_X11
    const std::string name = std::string(Native::backendName) + " X11";
#else
    const std::string name = Native::backendName;
#endif
    const std::string title = "Cookbook (" + name + ")";

    void configureWindow(CPPUI::Context * node) {
#if !defined(CPPUI_COOKBOOK_LVGL)
        static_cast<Native::Window *>(node)->title(title).maximize();
#else
        (void)node;
#endif
    }

    void menus() {
#if !defined(CPPUI_COOKBOOK_LVGL)
        for(auto bar: CPPUI::MenuBar()) {
            for(auto menu: CPPUI::Menu("Menu")) {
                CPPUI::MenuAction("Action").trigger([]() { std::cout << "action\n"; });
                CPPUI::MenuAction("Action2").trigger([]() { std::cout << "action2\n"; });
                for(auto submenu: CPPUI::Menu("Menu2")) {
                    CPPUI::MenuAction("Action3").trigger([]() { std::cout << "action3\n"; });
                    CPPUI::MenuAction("Action4").trigger([]() { std::cout << "action4\n"; });
                }
            }
            for(auto menu: CPPUI::Menu("Menu3")) {
                CPPUI::MenuAction("Action5");
                CPPUI::MenuAction("Action6");
            }
        }
#endif
    }

    void mount(CPPUI::Application & app) {
#if defined(CPPUI_COOKBOOK_LVGL)
        app.redraw();
#else
        app.sync();
#endif
    }

    void configureRecipes(std::vector<Recipe> & pages) {
        struct Implementation {
            const char * key;
            void (*render)(Model &);
            const char * remaining;
        };
        const Implementation implementations[] = {
            {"stack", stack, ""},
            {"grid", grid, ""},
            {"table", table, ""},
            {"table_adapter", tableAdapter, ""},
            {"progressbar", progressbar, ""},
            {"checkbox", checkbox, ""},
            {"heavycanvas", heavycanvas, ""},
            {"canvas", canvas, ""},
            {"tab", tab, ""},
            {"views", views, ""},
            {"timeline", timeline, ""},
            {"combobox", combobox, ""},
            {"radiobutton", radiobutton, ""},
            {"maskedfield", maskedfield, ""},
            {"image", image, ""},
            {"textfield", textfield, "String models; dynamic numeric conversion and typed input/change payloads pending"},
#if defined(CPPUI_COOKBOOK_LVGL)
            {"text", [](Model & model) {
                CPPUI::Text("HTML and Markdown formatting is not yet implemented; raw source is shown below.");
                text(model);
            }, "Plain multiline text implemented; HTML and Markdown display raw source; selection not yet implemented"},
#else
            {"tree", tree, ""},
            {"tree_adapter", treeAdapter, ""},
            {"splitter", splitter, ""},
            {"drop", drop, ""},
            {"dialog", dialog, ""},
            {"scroll", scroll, ""},
#if defined(CPPUI_COOKBOOK_QT)
            {"text", text, ""},
            {"mdi", mdi, ""},
            {"modal", modal, ""},
#else
            {"text", text, "Plain multiline text and HTML implemented; Markdown displays raw source"},
            {"mdi", nullptr, "MDI requires a native MDI parent frame"},
            {"modal", nullptr, "Nonblocking application-modal ownership policy needed"},
#endif
#endif
        };
        for(auto & page: pages) {
            for(const auto & implementation: implementations) {
                if(std::string(page.key) != implementation.key) continue;
                page.render = implementation.render;
                page.remaining = implementation.remaining;
                break;
            }
        }
    }

#if defined(CPPUI_COOKBOOK_QT)
    bool viewSelected(CPPUI::Node & node, const std::string & key) {
        auto views = dynamic_cast<Views *>(&node);
        if(!views || !views->ui) return false;
        const int index = static_cast<QStackedWidget *>(views->ui)->currentIndex();
        return key.empty() ? index == 0 : index > 0 && views->children->at(index - 1)->_id == key;
    }
    bool wheelCanvas(CPPUI::Node & node, int delta) {
        auto canvas = dynamic_cast<CPPUI::Qt::Canvas *>(&node);
        if(!canvas || !canvas->ui) return false;
        auto widget = static_cast<QWidget *>(canvas->ui);
        widget->grab(); // Paint once so the recipe has the current plot bounds.
        const QPointF point(widget->width()/2.0, 100);
        QWheelEvent event(point, widget->mapToGlobal(point), {}, QPoint(0, delta),
                          ::Qt::NoButton, ::Qt::NoModifier, ::Qt::NoScrollPhase, false);
        QCoreApplication::sendEvent(widget, &event); return true;
    }
    void settle() {
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
    QModelIndex treeIndex(CPPUI::Qt::Tree & tree, const std::vector<int> & path) {
        QModelIndex index;
        for(int row: path) {
            index = tree.model->index(row, 0, index);
            if(!index.isValid()) return {};
        }
        return index;
    }
    int treeChildCount(CPPUI::Node & node, const std::vector<int> & path) {
        auto tree = dynamic_cast<CPPUI::Qt::Tree *>(&node);
        if(!tree || !tree->model) return -1;
        const auto index = treeIndex(*tree, path);
        return path.empty() || index.isValid() ? tree->model->rowCount(index) : -1;
    }
    bool treeExpanded(CPPUI::Node & node, const std::vector<int> & path) {
        auto tree = dynamic_cast<CPPUI::Qt::Tree *>(&node);
        return tree && tree->model && static_cast<QTreeView *>(tree->ui)->isExpanded(treeIndex(*tree, path));
    }
    std::string tableText(CPPUI::Node & node, int row, int column) {
        auto table = dynamic_cast<CPPUI::Qt::Table *>(&node);
        return table && table->model ? table->model->data(table->model->index(row, column)).toString().toStdString() : std::string();
    }
    bool editTable(CPPUI::Node & node, int row, int column, const std::string & value) {
        auto table = dynamic_cast<CPPUI::Qt::Table *>(&node);
        return table && table->model && table->model->setData(table->model->index(row, column), QString::fromStdString(value));
    }
    bool atScrollEnd(CPPUI::Node & node) {
        auto scroll = dynamic_cast<CPPUI::Qt::Scroll *>(&node);
        if(!scroll || !scroll->ui) return false;
        auto bar = static_cast<QScrollArea *>(scroll->ui)->verticalScrollBar();
        return bar->maximum() > 0 && bar->value() == bar->maximum();
    }
    bool selectCombo(CPPUI::Node & node, int index) {
        auto combo = dynamic_cast<CPPUI::Qt::ComboBox *>(&node);
        if(!combo || !combo->ui) return false;
        static_cast<QComboBox *>(combo->ui)->setCurrentIndex(index); return true;
    }
    bool selectRadio(CPPUI::Node & node) {
        auto widget = dynamic_cast<CPPUI::Qt::BaseWidget *>(&node);
        auto radio = widget ? qobject_cast<QRadioButton *>(widget->ui) : nullptr;
        if(!radio) return false;
        radio->click(); return true;
    }
    bool editTextField(CPPUI::Node & node, const std::string & text, bool commit) {
        auto editor = dynamic_cast<CPPUI::Qt::TextField *>(&node);
        if(!editor || !editor->ui) return false;
        auto native = static_cast<QLineEdit *>(editor->ui);
        native->setText(QString::fromStdString(text));
        if(commit) native->editingFinished();
        return true;
    }
    bool clickButton(CPPUI::Node & node, const std::string & text) {
        auto button = dynamic_cast<CPPUI::Qt::Button *>(&node);
        if(!button || button->text != text) return false;
        static_cast<QPushButton *>(button->outer())->click();
        return true;
    }
    bool labelHasText(CPPUI::Node & node, const std::string & text) {
        auto widget = dynamic_cast<CPPUI::Qt::BaseWidget *>(&node);
        auto label = widget ? qobject_cast<QLabel *>(widget->ui) : nullptr;
        return label && label->text().toStdString() == text;
    }
    bool selectTab(CPPUI::Node & node, int index, const std::string & label) {
        auto tabs = dynamic_cast<CPPUI::Qt::Tabs *>(&node);
        if(!tabs) return false;
        auto widget = static_cast<QTabWidget *>(tabs->outer());
        if(index < 0 || index >= widget->count() || widget->tabText(index).toStdString() != label) return false;
        widget->setCurrentIndex(index);
        return widget->currentIndex() == index && widget->currentWidget()->isVisible();
    }
    bool toggleCheckbox(CPPUI::Node & node) {
        auto checkbox = dynamic_cast<CPPUI::Qt::Checkbox *>(&node);
        if(!checkbox || !checkbox->ui) return false;
        static_cast<QCheckBox *>(checkbox->ui)->click();
        return true;
    }
    void answerPrompt(const std::string & text, bool accepted) {
        QTimer::singleShot(0, qApp, [text, accepted]() {
            if(auto dialog = qobject_cast<QInputDialog *>(QApplication::activeModalWidget())) {
                dialog->setTextValue(QString::fromStdString(text));
                accepted ? dialog->accept() : dialog->reject();
            }
        });
    }
    bool modalVisible(CPPUI::Node & node) {
        auto modal = dynamic_cast<CPPUI::Qt::Modal *>(&node);
        return modal && modal->ui && modal->ui->isVisible()
            && static_cast<QDialog *>(modal->ui)->isModal();
    }
    bool selectPage(CPPUI::Node & node) {
        auto link = dynamic_cast<CPPUI::Qt::Label *>(&node);
        if(!link) return false;
        auto label = static_cast<QWidget *>(link->outer());
        for(auto parent = label->parentWidget(); parent; parent = parent->parentWidget()) {
            if(auto scroll = qobject_cast<QScrollArea *>(parent)) scroll->ensureWidgetVisible(label);
        }
        QMouseEvent event(QEvent::MouseButtonPress, QPointF(2, 2), QPointF(2, 2),
                          ::Qt::LeftButton, ::Qt::NoButton, ::Qt::NoModifier);
        QCoreApplication::sendEvent(label, &event);
        return true;
    }
    void prepareLayoutCheck(CPPUI::Node & root) {
        auto window = static_cast<QMainWindow *>(find(root, "cookbook")->outer());
        if(!window->isMaximized() || window->windowTitle() != QString::fromStdString(title)) {
            throw std::runtime_error("cookbook should open maximized with its window title");
        }
        // Force an overflowing list independently of the host's screen size.
        window->showNormal();
        window->resize(800, 600);
        settle();
    }
    bool pageVisible(CPPUI::Node & root, const std::string & key) {
        auto scroll = static_cast<QScrollArea *>(find(root, "menu-scroller")->outer());
        auto label = static_cast<QWidget *>(find(root, "page:" + key)->outer());
        return scroll->viewport()->rect().contains(label->mapTo(scroll->viewport(), label->rect().center()));
    }
    void checkLayout(CPPUI::Node & root) {
        auto window = static_cast<QMainWindow *>(find(root, "cookbook")->outer());
        auto sidebar = static_cast<QWidget *>(find(root, "menu-container")->outer());
        auto result = static_cast<QWidget *>(find(root, "result")->outer());
        auto scroll = static_cast<QScrollArea *>(find(root, "menu-scroller")->outer());
        if(sidebar->geometry().right() >= result->geometry().left()
           || result->width() <= sidebar->width() || result->height() < 200) {
            throw std::runtime_error("cookbook must retain a sidebar beside an expanding result pane");
        }
        if(scroll->verticalScrollBar()->maximum() <= 0 || !scroll->widget()
           || scroll->widget()->findChildren<QLabel *>().size() != static_cast<int>(recipes().size())) {
            throw std::runtime_error("cookbook page list must scroll and include every recipe");
        }
        const auto menus = window->menuBar()->actions();
        if(menus.size() != 2 || menus[0]->text() != "Menu" || menus[1]->text() != "Menu3"
           || menus[0]->menu()->actions().size() != 3
           || menus[0]->menu()->actions()[2]->menu()->actions().size() != 2
           || menus[1]->menu()->actions().size() != 2) {
            throw std::runtime_error("reference menus are missing or duplicated");
        }
        if(find(root, "code")) throw std::runtime_error("source code pane must be omitted");
    }
#elif defined(CPPUI_COOKBOOK_WX)
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
    bool labelHasText(CPPUI::Node & node,const std::string & text){auto label=dynamic_cast<CPPUI::Wx::Label *>(&node);return label&&label->ui&&(label->selectable?static_cast<wxTextCtrl *>(label->ui)->GetValue():label->ui->GetLabel())==wxString::FromUTF8(text);}
    bool selectTab(CPPUI::Node & node,int index,const std::string & text){auto tabs=dynamic_cast<CPPUI::Wx::Tabs *>(&node);if(!tabs||!tabs->ui)return false;auto ui=static_cast<wxNotebook *>(tabs->ui);if(index<0||index>=int(ui->GetPageCount())||ui->GetPageText(index)!=wxString::FromUTF8(text))return false;ui->SetSelection(index);return ui->GetSelection()==index&&ui->GetPage(index)->IsShown();}
    bool toggleCheckbox(CPPUI::Node & node){auto box=dynamic_cast<CPPUI::Wx::Checkbox *>(&node);if(!box||!box->ui)return false;auto ui=static_cast<wxCheckBox *>(box->ui);ui->SetValue(!ui->GetValue());command(ui,wxEVT_CHECKBOX);return true;}
    bool selectRadio(CPPUI::Node & node){auto widget=dynamic_cast<CPPUI::Wx::Widget *>(&node);auto radio=widget?dynamic_cast<wxRadioButton *>(widget->ui):nullptr;if(!radio)return false;radio->SetValue(true);command(radio,wxEVT_RADIOBUTTON);return true;}
    bool selectCombo(CPPUI::Node & node,int index){auto combo=dynamic_cast<CPPUI::Wx::ComboBox *>(&node);if(!combo||!combo->ui)return false;auto ui=static_cast<wxComboBox *>(combo->ui);ui->SetSelection(index);command(ui,wxEVT_COMBOBOX);return true;}
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
    bool selectPage(CPPUI::Node & node){auto link=dynamic_cast<CPPUI::Wx::Label *>(&node);if(!link||!link->ui)return false;
        for(auto parent=link->ui->GetParent();parent;parent=parent->GetParent())if(auto scroll=dynamic_cast<wxScrolledWindow *>(parent)){int x,y;scroll->GetViewStart(&x,&y);auto point=scroll->ScreenToClient(link->ui->ClientToScreen(wxPoint(0,0)));scroll->Scroll(x,std::max(0,y+point.y));}
        wxMouseEvent event(wxEVT_LEFT_DOWN);event.SetEventObject(link->ui);link->ui->ProcessWindowEvent(event);return true;}
    void prepareLayoutCheck(CPPUI::Node & root){auto frame=static_cast<wxFrame *>(find(root,"cookbook")->outer());if(frame->GetTitle()!=wxString::FromUTF8(title))throw std::runtime_error("cookbook window title missing");frame->Maximize(false);frame->SetSize(800,600);
        // The window manager may deliver unmaximize/resize after the first idle
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
    bool pageVisible(CPPUI::Node & root,const std::string & key){auto scroll=static_cast<wxScrolledWindow *>(find(root,"menu-scroller")->outer());auto label=static_cast<wxWindow *>(find(root,"page:"+key)->outer());auto center=scroll->ScreenToClient(label->ClientToScreen(wxPoint(label->GetSize().x/2,label->GetSize().y/2)));return wxRect(wxPoint(0,0),scroll->GetClientSize()).Contains(center);}
    void checkLayout(CPPUI::Node & root){auto frame=static_cast<wxFrame *>(find(root,"cookbook")->outer());auto sidebar=static_cast<CPPUI::Wx::Layout *>(find(root,"menu-container"))->sizer();auto result=static_cast<CPPUI::Wx::Layout *>(find(root,"result"))->sizer();auto scroll=static_cast<wxScrolledWindow *>(find(root,"menu-scroller")->outer());
        if(sidebar->GetPosition().x+sidebar->GetSize().x>result->GetPosition().x||result->GetSize().x<=sidebar->GetSize().x||result->GetSize().y<200)throw std::runtime_error("cookbook layout: frame="+std::to_string(frame->GetSize().x)+"x"+std::to_string(frame->GetSize().y)+"; panel="+std::to_string(static_cast<CPPUI::Wx::Window *>(find(root,"cookbook"))->panel->GetSize().x)+"x"+std::to_string(static_cast<CPPUI::Wx::Window *>(find(root,"cookbook"))->panel->GetSize().y)+"; sidebar x="+std::to_string(sidebar->GetPosition().x)+" width="+std::to_string(sidebar->GetSize().x)+"; result x="+std::to_string(result->GetPosition().x)+" width="+std::to_string(result->GetSize().x)+" height="+std::to_string(result->GetSize().y));
        if(scroll->GetVirtualSize().y<=scroll->GetClientSize().y)throw std::runtime_error("cookbook page list must scroll");
        auto bar=frame->GetMenuBar();if(!bar||bar->GetMenuCount()!=2||bar->GetMenu(0)->GetMenuItemCount()!=3||bar->GetMenu(1)->GetMenuItemCount()!=2)throw std::runtime_error("reference menus missing");
        if(find(root,"code"))throw std::runtime_error("source code pane must be omitted");
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
#elif defined(CPPUI_COOKBOOK_LVGL)
    bool viewSelected(CPPUI::Node & node, const std::string & key) {
        auto views = dynamic_cast<Views *>(&node);
        if(!views || !views->ui) return false;
        bool found = key.empty();
        for(auto child: *views->children) {
            const bool visible = !lv_obj_is_hidden(static_cast<lv_obj_t *>(child->outer()));
            if(visible != (!key.empty() && child->_id == key)) return false;
            if(visible) found = true;
        }
        return found;
    }
    int treeChildCount(CPPUI::Node &, const std::vector<int> &) { return -1; }
    bool treeExpanded(CPPUI::Node &, const std::vector<int> &) { return false; }
    std::string tableText(CPPUI::Node & node,int row,int column) {
        auto table=dynamic_cast<CPPUI::LVGL::Table *>(&node);
        return table ? table->cellText(row,column) : std::string();
    }
    bool editTable(CPPUI::Node & node,int row,int column,const std::string & value) {
        auto table=dynamic_cast<CPPUI::LVGL::Table *>(&node);
        return table && table->edit(row,column,value);
    }
    bool atScrollEnd(CPPUI::Node &) { return false; }
    bool selectCombo(CPPUI::Node & node, int index) {
        auto combo = dynamic_cast<CPPUI::LVGL::ComboBox *>(&node);
        if(!combo || !combo->ui || index < 0) return false;
        auto object = static_cast<lv_obj_t *>(combo->ui);
        auto dropdown = lv_obj_check_type(object, &lv_dropdown_class) ? object : lv_obj_get_child(object, 1);
        if(dropdown) lv_dropdown_open(dropdown);
        if(!dropdown || static_cast<unsigned>(index) >= lv_dropdown_get_option_count(dropdown)) return false;
        lv_dropdown_set_selected(dropdown, index);
        lv_dropdown_close(dropdown);
        lv_obj_send_event(dropdown, LV_EVENT_VALUE_CHANGED, nullptr);
        return true;
    }
    bool wheelCanvas(CPPUI::Node & node, int delta) {
        auto canvas = dynamic_cast<CPPUI::LVGL::Canvas *>(&node);
        if(!canvas || !canvas->ui) return false;
        auto object = static_cast<lv_obj_t *>(canvas->ui);
        lv_obj_update_layout(object);
        lv_area_t area; lv_obj_get_coords(object, &area);
        const int x = (area.x1 + area.x2)/2, y = area.y1 + 40;
        auto display = lv_obj_get_display(object);
        // Feed the actual driver queue so smoke tests cover SDL's pointer wheel
        // and X11's separate encoder, including the native input bridge.
#if LV_USE_SDL
        const auto window = SDL_GetWindowID(lv_sdl_window_get_window(display));
        SDL_Event event{}; event.type = SDL_WINDOWEVENT;
        event.window.windowID = window; event.window.event = SDL_WINDOWEVENT_ENTER;
        SDL_PushEvent(&event);
        event = SDL_Event{}; event.type = SDL_MOUSEMOTION;
        event.motion.windowID = window; event.motion.x = x; event.motion.y = y;
        SDL_PushEvent(&event); settle();
        event = SDL_Event{}; event.type = SDL_MOUSEWHEEL;
        event.wheel.windowID = window; event.wheel.y = delta/120;
        SDL_PushEvent(&event);
#elif LV_USE_X11
        auto connection = static_cast<_x11_user_hdr_t *>(lv_display_get_driver_data(display))->display;
        XEvent event{}; event.type = MotionNotify; event.xmotion.x = x; event.xmotion.y = y;
        XPutBackEvent(connection, &event);
        event = XEvent{}; event.type = EnterNotify; XPutBackEvent(connection, &event);
        settle(); settle(); // Publish motion before the independent wheel device reads.
        event = XEvent{}; event.type = ButtonPress; event.xbutton.button = delta > 0 ? Button4 : Button5;
        XPutBackEvent(connection, &event);
#endif
        settle(); settle();
        return true;
    }
    bool editTextField(CPPUI::Node & node, const std::string & text, bool commit) {
        auto field = dynamic_cast<CPPUI::LVGL::TextField *>(&node);
        if(!field || !field->ui) return false;
        auto object = static_cast<lv_obj_t *>(field->ui);
        auto group = static_cast<lv_group_t *>(lv_obj_get_group(object));
        if(!group) return false;
        lv_group_focus_obj(object);
        lv_textarea_set_text(object, text.c_str());
        if(commit) lv_group_send_data(group, LV_KEY_ENTER);
        return true;
    }
    void settle() {
        // Finish layout before waiting so size/style changes cannot postpone the frame.
        lv_obj_update_layout(lv_screen_active());
        const auto start = lv_tick_get();
        // Wait for the next refresh using the backend's elapsed-time clock.
        lv_delay_ms(LV_DEF_REFR_PERIOD + 1);
        while(lv_tick_elaps(start) <= LV_DEF_REFR_PERIOD) lv_delay_ms(1);
        lv_timer_handler();
    }
    bool clickButton(CPPUI::Node & node, const std::string & text) {
        auto button = dynamic_cast<CPPUI::LVGL::Button *>(&node);
        if(!button || button->text != text) return false;
        lv_obj_send_event(static_cast<lv_obj_t *>(button->outer()), LV_EVENT_CLICKED, nullptr);
        return true;
    }
    bool labelHasText(CPPUI::Node & node, const std::string & text) {
        auto label = dynamic_cast<CPPUI::LVGL::Label *>(&node);
        auto multiline = dynamic_cast<CPPUI::LVGL::Text *>(&node);
        return (label || multiline) && node.outer()
            && text == lv_label_get_text(static_cast<lv_obj_t *>(node.outer()));
    }
    bool selectTab(CPPUI::Node & node, int index, const std::string & text) {
        auto tabs = dynamic_cast<CPPUI::LVGL::Tabs *>(&node);
        if(!tabs || !tabs->ui) return false;
        auto object = static_cast<lv_obj_t *>(tabs->ui);
        auto button = lv_tabview_get_tab_button(object, index);
        if(!button) return false;
        lv_obj_send_event(button, LV_EVENT_CLICKED, nullptr);
        auto page = lv_obj_get_child(lv_tabview_get_content(object), index);
        auto label = lv_obj_get_child(button, 0);
        return lv_tabview_get_tab_active(object) == static_cast<uint32_t>(index)
            && label && text == lv_label_get_text(label) && lv_obj_is_visible(page);
    }
    bool toggleCheckbox(CPPUI::Node & node) {
        auto checkbox = dynamic_cast<CPPUI::LVGL::Checkbox *>(&node);
        if(!checkbox || !checkbox->ui) return false;
        lv_obj_send_event(static_cast<lv_obj_t *>(checkbox->ui), LV_EVENT_RELEASED, nullptr);
        return true;
    }
    bool selectRadio(CPPUI::Node & node) {
        auto radio = dynamic_cast<CPPUI::LVGL::Node *>(&node);
        auto object = radio ? static_cast<lv_obj_t *>(radio->ui) : nullptr;
        if(!object || !lv_obj_is_radio_button(object)) return false;
        if(lv_obj_send_event(object, LV_EVENT_RELEASED, nullptr) == LV_RESULT_OK)
            lv_obj_send_event(object, LV_EVENT_CLICKED, nullptr);
        return true;
    }
    void answerPrompt(const std::string &, bool) {}
    bool modalVisible(CPPUI::Node &) { return false; }
    bool selectPage(CPPUI::Node & node) {
        auto link = dynamic_cast<CPPUI::LVGL::Label *>(&node);
        if(!link) return false;
        auto label = static_cast<lv_obj_t *>(link->outer());
        lv_obj_scroll_to_view_recursive(label, LV_ANIM_OFF);
        lv_obj_send_event(label, LV_EVENT_CLICKED, nullptr);
        return true;
    }
    void prepareLayoutCheck(CPPUI::Node &) {}
    bool pageVisible(CPPUI::Node & root, const std::string & key) {
        lv_obj_update_layout(lv_screen_active());
        lv_area_t viewport, label;
        lv_obj_get_coords(static_cast<lv_obj_t *>(find(root, "menu-scroller")->outer()), &viewport);
        lv_obj_get_coords(static_cast<lv_obj_t *>(find(root, "page:" + key)->outer()), &label);
        return label.y1 >= viewport.y1 && label.y2 <= viewport.y2;
    }
    void checkLayout(CPPUI::Node & root) {
        lv_obj_update_layout(lv_screen_active());
        bool keyboardReady = false;
        for(auto input = lv_indev_get_next(nullptr); input; input = lv_indev_get_next(input)) {
            if(lv_indev_get_type(input) == LV_INDEV_TYPE_KEYPAD && lv_group_get_default()
               && lv_indev_get_group(input) == lv_group_get_default()) keyboardReady = true;
        }
        if(!keyboardReady) throw std::runtime_error("cookbook keyboard must be attached to the default focus group");
#if LV_USE_X11
        lv_indev_t * pointer = nullptr;
        for(auto input = lv_indev_get_next(nullptr); input; input = lv_indev_get_next(input)) {
            if(lv_indev_get_type(input) == LV_INDEV_TYPE_POINTER) pointer = input;
        }
        if(!pointer || lv_indev_get_cursor(pointer)) {
            throw std::runtime_error("X11 must retain pointer input without an LVGL software cursor");
        }
#endif
        auto sidebar = static_cast<lv_obj_t *>(find(root, "menu-container")->outer());
        auto result = static_cast<lv_obj_t *>(find(root, "result")->outer());
        auto scroll = static_cast<lv_obj_t *>(find(root, "menu-scroller")->outer());
        auto list = static_cast<lv_obj_t *>(find(root, "menu-list")->outer());
        if(lv_obj_get_x(sidebar) + lv_obj_get_width(sidebar) > lv_obj_get_x(result)
           || lv_obj_get_width(result) <= lv_obj_get_width(sidebar) || lv_obj_get_height(result) < 200) {
            throw std::runtime_error("cookbook must retain a sidebar beside an expanding result pane");
        }
        if(lv_obj_get_height(list) <= lv_obj_get_content_height(scroll)
           || lv_obj_get_child_count(list) != recipes().size() + 1
           || lv_obj_get_width(list) < 100
           || lv_obj_get_width(lv_obj_get_child(list, 0)) < 100
           || lv_obj_get_height(lv_obj_get_child(list, 0)) > 40) {
            throw std::runtime_error("cookbook page list must scroll and include every recipe");
        }
        if(find(root, "code")) throw std::runtime_error("source code pane must be omitted");
        if(auto row = find(root, "spacer-row")) {
            auto box = static_cast<lv_obj_t *>(row->outer());
            auto gap = static_cast<lv_obj_t *>(find(root, "row-gap")->outer());
            lv_area_t content, end;
            lv_obj_get_content_coords(box, &content);
            lv_obj_get_coords(static_cast<lv_obj_t *>(find(root, "row-end")->outer()), &end);
            if(lv_obj_get_width(gap) <= lv_obj_get_content_width(box)/2 || end.x2 != content.x2)
                throw std::runtime_error("horizontal spacer must push Column 2 to the row's far edge");
        }
        if(auto column = find(root, "spacer-column")) {
            auto box = static_cast<lv_obj_t *>(column->outer());
            auto gap = static_cast<lv_obj_t *>(find(root, "column-gap")->outer());
            lv_area_t content, end;
            lv_obj_get_content_coords(box, &content);
            lv_obj_get_coords(gap, &end);
            if(lv_obj_get_height(gap) <= lv_obj_get_content_height(box)/2 || end.y2 != content.y2)
                throw std::runtime_error("vertical spacer must fill the remaining column height");
        }
        if(auto grid = find(root, "example-grid")) {
            auto object = static_cast<lv_obj_t *>(grid->outer());
            auto a = static_cast<lv_obj_t *>(find(root, "grid-0-0")->outer());
            auto b = static_cast<lv_obj_t *>(find(root, "grid-1-0")->outer());
            auto c = static_cast<lv_obj_t *>(find(root, "grid-0-1")->outer());
            auto d = static_cast<lv_obj_t *>(find(root, "grid-2-1")->outer());
            if(lv_obj_get_x(a) != 0 || lv_obj_get_y(a) != 0 || lv_obj_get_y(c) != 0
               || lv_obj_get_x(b) != lv_obj_get_x(a) || lv_obj_get_x(c) != lv_obj_get_width(a) + 6
               || lv_obj_get_x(d) != lv_obj_get_x(c) || lv_obj_get_y(b) != lv_obj_get_height(a) + 6
               || lv_obj_get_y(d) != lv_obj_get_y(b) + lv_obj_get_height(b) + 6
               || lv_obj_get_style_grid_cell_row_span(c, LV_PART_MAIN) != 2
               || lv_obj_get_height(object) != lv_obj_get_y(d) + lv_obj_get_height(d))
                throw std::runtime_error("Grid recipe must retain its two columns, three rows and spanning cell");
        }
        if(auto stack = find(root, "example-stack")) {
            auto object = static_cast<lv_obj_t *>(stack->outer());
            auto bottom = static_cast<lv_obj_t *>(find(root, "stack-bottom")->outer());
            auto top = static_cast<lv_obj_t *>(find(root, "stack-top")->outer());
            lv_area_t a, b, content;
            lv_obj_get_coords(bottom, &a); lv_obj_get_coords(top, &b);
            lv_obj_get_content_coords(object, &content);
            if(a.x1 != b.x1 || a.y1 != b.y1 || a.x1 != content.x1 || a.y1 != content.y1
               || lv_obj_get_height(object) != lv_obj_get_height(bottom)
               || lv_obj_get_height(bottom) < lv_obj_get_height(top)
               || lv_obj_get_height(top) != lv_obj_get_style_text_font(top, LV_PART_MAIN)->line_height
               || lv_obj_get_child(object, 1) != top)
                throw std::runtime_error("Stack recipe must overlap natural-height labels with the last layer on top");
        }
        if(auto image = find(root, "example-image")) {
            auto object = static_cast<lv_obj_t *>(image->outer());
            auto pixels = static_cast<const lv_draw_buf_t *>(lv_image_get_src(object));
            if(!pixels || pixels->header.w != 400 || pixels->header.h != 339
               || lv_obj_get_width(object) != 400 || lv_obj_get_height(object) != 339)
                throw std::runtime_error("Image recipe must decode the windmill and preserve its aspect ratio");
        }
        if(auto canvas = find(root, "example-canvas")) {
            auto object = static_cast<lv_obj_t *>(canvas->outer());
            auto buffer = lv_canvas_get_draw_buf(object);
            if(lv_obj_get_width(object) != 100 || lv_obj_get_height(object) != 100
               || !buffer || buffer->header.w != 100 || buffer->header.h != 100) {
                throw std::runtime_error("canvas recipe must paint at its fixed 100x100 size");
            }
            const auto pixel = lv_canvas_get_px(object, 15, 15);
            if(pixel.alpha != 255 || pixel.red != 0xcc || pixel.green != 0xcc || pixel.blue != 0xcc) {
                throw std::runtime_error("canvas recipe did not paint its rectangle");
            }
        }
        if(auto canvas = find(root, "heavy-canvas")) {
            auto object = static_cast<lv_obj_t *>(canvas->outer());
            auto buffer = lv_canvas_get_draw_buf(object);
            if(!buffer || buffer->header.w < 320 || buffer->header.h != 720)
                throw std::runtime_error("HeavyCanvas must paint a usable chart at its requested height");
            if(lv_canvas_get_px(object, 70, 24).alpha == 0)
                throw std::runtime_error("HeavyCanvas did not render its price grid");
        }
    }
#endif
}}

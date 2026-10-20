#ifndef CPPUI_WX_COMPLETION
#define CPPUI_WX_COMPLETION

#include <wx/wx.h>
#include <wx/combobox.h>
#include <wx/popupwin.h>
#include <wx/weakref.h>
#include <functional>
#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace CPPUI { namespace Wx {
// A non-grabbing popup: typing stays in the retained native ComboBox.
class ComboCompletion: public wxEvtHandler, public std::enable_shared_from_this<ComboCompletion> {
    wxWeakRef<wxComboBox> combo;
    wxWeakRef<wxPopupWindow> popup;
    wxWeakRef<wxListBox> rows;
    std::vector<wxWeakRef<wxWindow>> ancestors;
    std::vector<std::string> signature;
    bool enabled = false, requested = false, pending = false;
    std::function<void(int)> selected;
    void geometry(wxEvent & event) {
        event.Skip();
        if(pending || !shown()) return;
        pending = true;
        CallAfter([this]() { pending = false; refresh(); });
    }
    void resized(wxSizeEvent & event) { geometry(event); }
    void moved(wxMoveEvent & event) { geometry(event); }
    void visibility(wxShowEvent & event) { if(!event.IsShown()) hide(); event.Skip(); }
    void idle(wxIdleEvent & event) { if(shown()) refresh(); event.Skip(); }
    void dropdown(wxCommandEvent & event) { hide(); event.Skip(); }
    void blur(wxFocusEvent & event) {
        event.Skip();
        CallAfter([this]() {
            auto focus = wxWindow::FindFocus();
            if(!combo || (focus != combo.get() && focus != rows.get()
                && (!focus || !combo->IsDescendant(focus)))) hide();
        });
    }
    void clicked(wxMouseEvent &) { accept(); }
    void key(wxKeyEvent & event) {
        auto keep = shared_from_this();
        if(!shown()) { event.Skip(); return; }
        const int code = event.GetKeyCode();
        if(code == WXK_ESCAPE || code == WXK_TAB) {
            hide(); if(code == WXK_TAB) event.Skip();
        } else if((code == WXK_UP || code == WXK_DOWN) && rows->GetCount()) {
            const int count = rows->GetCount(), current = rows->GetSelection();
            const int step = code == WXK_DOWN ? 1 : -1;
            rows->SetSelection(current < 0 ? (step > 0 ? 0 : count - 1) : (current + step + count) % count);
        } else if(code == WXK_RETURN && rows->GetSelection() != wxNOT_FOUND) accept();
        else event.Skip();
    }
public:
    explicit ComboCompletion(wxComboBox * control): combo(control) {
        popup = new wxPopupWindow(control);
        rows = new wxListBox(popup.get(), wxID_ANY);
        auto layout = new wxBoxSizer(wxVERTICAL); layout->Add(rows.get(), 1, wxEXPAND); popup->SetSizer(layout);
        rows->Bind(wxEVT_LEFT_UP, &ComboCompletion::clicked, this);
        rows->Bind(wxEVT_KILL_FOCUS, &ComboCompletion::blur, this);
        combo->Bind(wxEVT_KEY_DOWN, &ComboCompletion::key, this);
        combo->Bind(wxEVT_KILL_FOCUS, &ComboCompletion::blur, this);
        combo->Bind(wxEVT_COMBOBOX_DROPDOWN, &ComboCompletion::dropdown, this);
        combo->Bind(wxEVT_IDLE, &ComboCompletion::idle, this);
        for(wxWindow * window = control; window; window = window->GetParent()) {
            ancestors.emplace_back(window);
            window->Bind(wxEVT_MOVE, &ComboCompletion::moved, this);
            window->Bind(wxEVT_SIZE, &ComboCompletion::resized, this);
            window->Bind(wxEVT_SHOW, &ComboCompletion::visibility, this);
            if(window->IsTopLevel()) break;
        }
    }
    ~ComboCompletion() override {
        if(wxTheApp) wxTheApp->RemovePendingEventHandler(this);
        DeletePendingEvents();
        if(combo) {
            combo->Unbind(wxEVT_KEY_DOWN, &ComboCompletion::key, this);
            combo->Unbind(wxEVT_KILL_FOCUS, &ComboCompletion::blur, this);
            combo->Unbind(wxEVT_COMBOBOX_DROPDOWN, &ComboCompletion::dropdown, this);
            combo->Unbind(wxEVT_IDLE, &ComboCompletion::idle, this);
        }
        for(auto & window: ancestors) if(window) {
            window->Unbind(wxEVT_MOVE, &ComboCompletion::moved, this);
            window->Unbind(wxEVT_SIZE, &ComboCompletion::resized, this);
            window->Unbind(wxEVT_SHOW, &ComboCompletion::visibility, this);
        }
        if(rows) {
            rows->Unbind(wxEVT_LEFT_UP, &ComboCompletion::clicked, this);
            rows->Unbind(wxEVT_KILL_FOCUS, &ComboCompletion::blur, this);
        }
        if(popup && !popup->IsBeingDeleted()) { popup->Hide(); popup->Destroy(); }
    }
    bool shown() const { return popup && popup->IsShown(); }
    void hide() { requested = false; if(popup) popup->Hide(); }
    void configure(bool active, const wxArrayString & labels, const std::vector<std::string> & keys,
                   std::function<void(int)> callback) {
        enabled = active; selected = std::move(callback);
        if(rows && (signature != keys || rows->GetStrings() != labels)) {
            rows->Set(labels); signature = keys;
        }
        refresh();
    }
    void request() { requested = true; refresh(); }
    void refresh() {
        if(!combo || !popup || !rows || !enabled || !combo->IsEnabled() || !combo->IsShownOnScreen()) { hide(); return; }
        if(!requested) return;
        if(!rows->GetCount()) { popup->Hide(); return; }
        if(rows->GetFont() != combo->GetFont()) rows->SetFont(combo->GetFont());
        wxSize size(combo->GetSize().x, std::max(24, int(std::min(8U, rows->GetCount())) * (rows->GetCharHeight() + 8) + 4));
        if(popup->GetSize() != size) popup->SetSize(size);
        popup->Position(combo->ClientToScreen(wxPoint(0, 0)), wxSize(0, combo->GetSize().y));
        if(!shown()) popup->Show();
    }
    void accept() {
        auto keep = shared_from_this();
        if(!rows || !shown()) return;
        int index = rows->GetSelection(); auto callback = selected;
        if(index == wxNOT_FOUND || index >= int(rows->GetCount())) return;
        hide();
        if(callback) try { callback(index); }
        catch(const std::exception & error) { std::cerr << "CPPUI ComboBox completion: " << error.what() << '\n'; }
        catch(...) { std::cerr << "CPPUI ComboBox completion: unknown exception\n"; }
        if(combo && !combo->IsBeingDeleted()) { combo->SetFocus(); combo->SetInsertionPointEnd(); }
    }
};
}}
#endif

#include "cppui/wx/application.hpp"
#include "cppui/wx/window.hpp"
#include "cppui/wx/layout.hpp"
#include "cppui/wx/combobox.hpp"
#include <stdexcept>

namespace {
void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
void pump() { for(int i = 0; i < 6; ++i) { wxTheApp->Yield(true); wxTheApp->ProcessPendingEvents(); wxTheApp->ProcessIdle(); } }
void key(wxWindow * window, int code) { wxKeyEvent event(wxEVT_KEY_DOWN); event.m_keyCode = code; window->ProcessWindowEvent(event); pump(); }
class Root: public CPPUI::Application {
public:
    using Application::Application;
    CPPUI::State<std::string> value{std::string()}, query{std::string("not the display")};
    bool show = true, visible = true, enabled = true, expand = true, filtered = false, empty = false;
    int inputs = 0, changes = 0;
    std::function<void()> action;
    void content() override {
        auto text = static_cast<std::string>(query);
        for(auto window: CPPUI::Window().size(500, 300)) for(auto panel: CPPUI::VBox()) if(show) {
            for(auto combo: CPPUI::ComboBox(true, value).editModel(query).autoexpand(expand).disabled(!enabled)
                    .input([this]() { ++inputs; if(action) action(); }).change([this]() { ++changes; }).visible(visible)) {
                if(!empty) {
                    CPPUI::ComboBoxItem("Amsterdam", "AMS");
                    if(!filtered || text.empty()) CPPUI::ComboBoxItem("Tokyo", "HND");
                }
            }
        }
    }
    wxComboBox * combo() { return static_cast<wxComboBox *>(children->front()->children->front()->children->front()->outer()); }
};
wxPopupWindow * popup(wxWindow * parent) {
    for(auto child: parent->GetChildren()) if(auto popup = dynamic_cast<wxPopupWindow *>(child)) return popup;
    return nullptr;
}
}
int main(int argc, char ** argv) {
    try {
        Root root(argc, argv); root.sync(); pump();
        auto combo = root.combo(); auto suggestions = popup(combo);
        require(suggestions && combo->GetValue().empty(), "popup missing or edit model drove display");
        combo->SetFocus(); root.filtered = true;
        for(auto text: {"t", "tai", "tai tok"}) { combo->SetValue(text); pump(); }
        require(root.combo() == combo && combo->GetValue() == "tai tok" && root.inputs == 3 && root.changes == 3
            && static_cast<std::string>(root.query) == "tai tok" && static_cast<std::string>(root.value) == "tai tok",
            "reactive typing lost draft, identity, input binding or callback count");
        auto rows = static_cast<wxListBox *>(suggestions->GetChildren().front());
        require(suggestions->IsShown() && rows->GetCount() == 1 && rows->GetString(0) == "Amsterdam",
            "application candidates were filtered again or popup did not stay open");
        combo->SetSelection(1, 3); root.sync(); pump(); long start, end; combo->GetSelection(&start, &end);
        require(start == 1 && end == 3, "redraw lost editor selection");
        auto frame = static_cast<wxWindow *>(root.children->front()->outer());
        frame->Move(frame->GetPosition() + wxPoint(20, 20)); pump();
        require(suggestions->GetPosition() == combo->ClientToScreen(wxPoint(0, combo->GetSize().y)), "popup detached after ancestor move");
        key(combo, WXK_DOWN); key(combo, WXK_RETURN);
        require(combo->GetValue() == "Amsterdam" && static_cast<std::string>(root.value) == "AMS"
            && static_cast<std::string>(root.query) == "tai tok" && root.changes == 4 && root.inputs == 3,
            "acceptance failed or emitted input/duplicate change");
        combo->SetValue("AMS"); pump(); root.sync(); pump();
        require(combo->GetValue() == "AMS" && combo->GetSelection() == wxNOT_FOUND, "typed value unexpectedly resolved to item label");
        key(combo, WXK_ESCAPE);
        require(!suggestions->IsShown() && combo->GetValue() == "AMS", "Escape changed draft or failed to dismiss");
        root.expand = false; root.sync(); combo->SetValue("manual"); pump();
        require(!suggestions->IsShown() && root.combo() == combo, "autoexpand off opened suggestions or remounted editor");
        root.expand = true; root.empty = true; root.sync(); combo->SetValue("empty"); pump();
        require(!suggestions->IsShown(), "empty candidates opened popup");
        root.empty = false; root.sync(); pump();
        require(suggestions->IsShown(), "repopulating active query did not refresh candidates");
        root.enabled = false; root.sync(); pump(); require(!suggestions->IsShown(), "disabled popup remained open");
        root.enabled = true; root.sync(); combo->SetValue("hide"); pump();
        root.visible = false; root.sync(); pump(); require(!suggestions->IsShown(), "hidden popup remained open");
        root.visible = true; root.sync(); combo->SetValue("delete"); pump();
        wxWeakRef<wxWindow> oldCombo(combo), oldPopup(suggestions);
        root.action = [&]() { root.show = false; root.sync(); };
        combo->SetValue("remove"); pump();
        require(!oldCombo && !oldPopup, "input callback removal retained editor or popup");
    } catch(const std::exception & error) { std::cerr << error.what() << '\n'; return 1; }
}

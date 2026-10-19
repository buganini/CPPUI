#include "backend.hpp"
#include <wx/stopwatch.h>

namespace ExampleTest {
    void settle() {
        wxStopWatch clock;
        do {
            wxTheApp->Yield(true);
            wxTheApp->ProcessPendingEvents();
            wxTheApp->ProcessIdle();
            wxMilliSleep(1);
        } while(clock.Time() < 25);
    }
    void command(wxWindow * window, wxEventType type) {
        wxCommandEvent event(type, window->GetId());
        event.SetEventObject(window);
        window->ProcessWindowEvent(event);
    }
    bool clickButton(CPPUI::Node & node, const std::string & text) {
        auto button = dynamic_cast<CPPUI::Wx::Button *>(&node);
        if(!button || button->text != text) return false;
        command(button->ui, wxEVT_BUTTON);
        return true;
    }
    bool labelHasText(CPPUI::Node & node, const std::string & text) {
        auto label = dynamic_cast<CPPUI::Wx::Label *>(&node);
        return label && label->ui && label->ui->GetLabel() == wxString::FromUTF8(text);
    }
    bool toggleCheckbox(CPPUI::Node & node) {
        auto checkbox = dynamic_cast<CPPUI::Wx::Checkbox *>(&node);
        if(!checkbox || !checkbox->ui) return false;
        auto widget = static_cast<wxCheckBox *>(checkbox->ui);
        widget->SetValue(!widget->GetValue());
        command(widget, wxEVT_CHECKBOX);
        return true;
    }
}

#ifndef CPPUI_WX_CHECKBOX
#define CPPUI_WX_CHECKBOX
#include "base.hpp"
#include "../binding.hpp"
namespace CPPUI { namespace Wx {
class Checkbox: public Widget {
    Binding<bool> model;
    bool checked;
public:
    std::string text;
    Checkbox(const std::string & text, Binding<bool> model): model(model), checked(model.get()), text(text) {}
    void updateWidget(Widget * prev) override {
        ui = prev ? prev->ui : new wxCheckBox(parentWindow(), wxID_ANY, wxString::FromUTF8(text));
        ui->SetLabel(wxString::FromUTF8(text)); static_cast<wxCheckBox *>(ui)->SetValue(checked);
    }
    void bindEvents() override { ui->Bind(wxEVT_CHECKBOX, &Native::clicked, native.get()); }
    void updateEvents() override {
        auto binding = model; auto callback = onClicked; auto object = native.get();
        native->command = [binding, callback, object]() mutable {
            if(!object->window) return;
            bool value = static_cast<wxCheckBox *>(object->window)->GetValue();
            binding.set(value); Node::commitFocused(); if(callback) callback();
        };
    }
};
}
inline Wx::Checkbox & Checkbox(const std::string & text, Binding<bool> model) { return *new Wx::Checkbox(text, model); }
}
#endif

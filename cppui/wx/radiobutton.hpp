#ifndef CPPUI_WX_RADIOBUTTON
#define CPPUI_WX_RADIOBUTTON
#include "base.hpp"
#include "../binding.hpp"
namespace CPPUI { namespace Wx {
template<class T> class RadioButton: public Widget {
    std::string text;
    T value;
    Binding<T> model;
    bool checked;
public:
    using Value = T;
    RadioButton(const std::string & text, const T & value, Binding<T> model): text(text), value(value), model(model), checked(model.get() == value) {}
    void updateWidget(Widget * prev) override {
        ui = prev ? prev->ui : new wxRadioButton(parentWindow(), wxID_ANY, wxString::FromUTF8(text), wxDefaultPosition, wxDefaultSize, wxRB_SINGLE);
        ui->SetLabel(wxString::FromUTF8(text)); static_cast<wxRadioButton *>(ui)->SetValue(checked);
    }
    void bindEvents() override { ui->Bind(wxEVT_RADIOBUTTON, &Native::clicked, native.get()); }
    void updateEvents() override {
        auto binding = model; auto selected = value; auto callback = onClicked;
        native->command = [binding, selected, callback]() mutable { binding.set(selected); Node::commitFocused(); if(callback) callback(); };
    }
};
}
template<class T> inline Wx::RadioButton<T> & RadioButton(const std::string & text, const typename Wx::RadioButton<T>::Value & value, Binding<T> model) { return *new Wx::RadioButton<T>(text, value, model); }
template<class T> inline Wx::RadioButton<T> & RadioButton(const std::string & text, const typename Wx::RadioButton<T>::Value & value, State<T> & model) { return *new Wx::RadioButton<T>(text, value, Binding<T>(model)); }
}
#endif

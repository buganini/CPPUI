#ifndef CPPUI_WX_TEXTEDIT
#define CPPUI_WX_TEXTEDIT
#include "base.hpp"
#include "../binding.hpp"
namespace CPPUI { namespace Wx {
class TextEdit: public Widget {
    struct EditState { bool editing = false, committing = false; OptionalText value; };
    std::shared_ptr<EditState> state;
    Binding<std::string> model;
    Optional<Binding<std::string>> editModel;
    std::string value, editValue, placeholderText;
protected:
    bool isReadonly=false,wrapText=true;
private:
    std::function<void()> onInput, onChange;
public:
    explicit TextEdit(Binding<std::string> model): model(model), value(model.get()) { expandX=expandY=4; }
    TextEdit(Binding<std::string> model, Binding<std::string> edit): TextEdit(model) { editModel = edit; editValue = edit.get(); }
    TextEdit & input(std::function<void()> callback) { onInput = std::move(callback); return *this; }
    TextEdit & change(std::function<void()> callback) { onChange = std::move(callback); return *this; }
    TextEdit & placeholder(const std::string & text) { placeholderText = text; return *this; }
    TextEdit & readonly(bool value=true) {isReadonly=value;return *this;}
    TextEdit & wrap(bool value=true) {wrapText=value;return *this;}
    void updateWidget(Widget * previous) override {
        auto prev = static_cast<TextEdit *>(previous);
        state = prev ? prev->state : std::make_shared<EditState>();
        ui = prev ? prev->ui : new wxTextCtrl(parentWindow(), wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | (wrapText ? 0 : wxTE_DONTWRAP));
        if(prev && prev->wrapText!=wrapText) ui->SetWindowStyleFlag(wxTE_MULTILINE | (wrapText ? 0 : wxTE_DONTWRAP));
        static_cast<wxTextCtrl *>(ui)->SetEditable(!isReadonly);
        if(isReadonly) state->editing=false;
        static_cast<wxTextCtrl *>(ui)->SetHint(wxString::FromUTF8(placeholderText));
        if(!state->editing && static_cast<wxTextCtrl *>(ui)->GetValue().ToStdString(wxConvUTF8)!=value) {
            static_cast<wxTextCtrl *>(ui)->ChangeValue(wxString::FromUTF8(value)); state->value = value;
        }
        if(editModel && !state->editing && editValue != value) editModel->set(value);
    }
    void bindEvents() override {
        ui->Bind(wxEVT_TEXT, &Native::typed, native.get());
        ui->Bind(commitEvent(), &Native::commit, native.get());
        ui->Bind(wxEVT_KILL_FOCUS, &Native::blur, native.get());
    }
    void updateEvents() override {
        auto object = native.get(); auto edit = state; auto binding = model; auto draft = editModel;
        auto input = onInput, change = onChange;const bool readonly=isReadonly;
        native->input = [object, edit, draft, input, readonly]() mutable {
            if(!object->window || readonly) return;
            edit->editing = true;
            const auto text = static_cast<wxTextCtrl *>(object->window)->GetValue().ToStdString(wxConvUTF8);
            if(draft) draft->set(text); if(input) input();
        };
        native->finish = [object, edit, binding, draft, change, readonly]() mutable {
            if(!object->window || readonly || !edit->editing || edit->committing) return;
            edit->editing = false; edit->committing = true;
            const auto text = static_cast<wxTextCtrl *>(object->window)->GetValue().ToStdString(wxConvUTF8);
            try {
                binding.set(text);
                auto committed = binding.get();
                if(draft) draft->set(committed);
                if(object->window && object->node) { static_cast<wxTextCtrl *>(object->window)->ChangeValue(wxString::FromUTF8(committed)); edit->value = committed; }
                if(change) change();
            } catch(...) { edit->committing = false; throw; }
            edit->committing = false;
        };
    }
    void commit() override { auto keep = native; auto callback = keep ? keep->finish : std::function<void()>(); if(callback) callback(); }
};
}
inline Wx::TextEdit & TextEdit(Binding<std::string> model) { return *new Wx::TextEdit(model); }
inline Wx::TextEdit & TextEdit(Binding<std::string> model,Binding<std::string> edit) { return *new Wx::TextEdit(model,edit); }
}
#endif

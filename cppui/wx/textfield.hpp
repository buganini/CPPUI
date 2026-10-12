#ifndef CPPUI_WX_TEXTFIELD
#define CPPUI_WX_TEXTFIELD
#include "base.hpp"
#include "../binding.hpp"
namespace CPPUI { namespace Wx {
class TextField: public Widget {
    struct EditState { bool editing = false, committing = false; OptionalText value; };
    std::shared_ptr<EditState> state;
    Binding<std::string> model;
    Optional<Binding<std::string>> editModel;
    std::string value, editValue;
    std::function<void()> onInput, onChange;
public:
    explicit TextField(Binding<std::string> model): model(model), value(model.get()) { layout_width = 50; }
    TextField(Binding<std::string> model, Binding<std::string> edit): TextField(model) { editModel = edit; editValue = edit.get(); }
    TextField & input(std::function<void()> callback) { onInput = std::move(callback); return *this; }
    TextField & change(std::function<void()> callback) { onChange = std::move(callback); return *this; }
    void updateWidget(Widget * previous) override {
        auto prev = static_cast<TextField *>(previous);
        state = prev ? prev->state : std::make_shared<EditState>();
        ui = prev ? prev->ui : new wxTextCtrl(parentWindow(), wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
        if(!state->editing && (!state->value || *state->value != value)) {
            static_cast<wxTextCtrl *>(ui)->ChangeValue(wxString::FromUTF8(value)); state->value = value;
        }
        if(editModel && !state->editing && editValue != value) editModel->set(value);
    }
    void bindEvents() override {
        ui->Bind(wxEVT_TEXT, &Native::typed, native.get());
        ui->Bind(wxEVT_TEXT_ENTER, &Native::commit, native.get());
        ui->Bind(commitEvent(), &Native::commit, native.get());
        ui->Bind(wxEVT_KILL_FOCUS, &Native::blur, native.get());
    }
    void updateEvents() override {
        auto object = native.get(); auto edit = state; auto binding = model; auto draft = editModel;
        auto input = onInput, change = onChange;
        native->input = [object, edit, draft, input]() mutable {
            if(!object->window) return;
            edit->editing = true;
            const auto text = static_cast<wxTextCtrl *>(object->window)->GetValue().ToStdString(wxConvUTF8);
            if(draft) draft->set(text); if(input) input();
        };
        native->finish = [object, edit, binding, draft, change]() mutable {
            if(!object->window || !edit->editing || edit->committing) return;
            edit->editing = false; edit->committing = true;
            const auto text = static_cast<wxTextCtrl *>(object->window)->GetValue().ToStdString(wxConvUTF8);
            try {
                binding.set(text); if(draft) draft->set(text);
                auto committed = binding.get();
                if(object->window && object->node) { static_cast<wxTextCtrl *>(object->window)->ChangeValue(wxString::FromUTF8(committed)); edit->value = committed; }
                if(change) change();
            } catch(...) { edit->committing = false; throw; }
            edit->committing = false;
        };
    }
    void commit() override { auto keep = native; auto callback = keep ? keep->finish : std::function<void()>(); if(callback) callback(); }
};
}
inline Wx::TextField & TextField(Binding<std::string> model) { return *new Wx::TextField(model); }
inline Wx::TextField & TextField(Binding<std::string> model, Binding<std::string> edit) { return *new Wx::TextField(model, edit); }
}
#endif

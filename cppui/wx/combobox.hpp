#ifndef CPPUI_WX_COMBOBOX
#define CPPUI_WX_COMBOBOX
#include "base.hpp"
#include "completion.hpp"
#include "../binding.hpp"
namespace CPPUI { namespace Wx {
class ComboBoxItem: public ::CPPUI::Node {
public:
    std::string text, value;
    ComboBoxItem(const std::string & text, const std::string & value): text(text), value(value) { id(text); is_terminal = true; }
    void destroy(bool) override {}
    void postUpdate() override;
};
class ComboBox: public Widget {
    struct Choice { std::string text, value, key; };
    struct Selection {
        OptionalText key; std::string text; std::vector<Choice> items;
        bool applying = false, editing = false, committing = false;
        std::shared_ptr<ComboCompletion> completion;
    };
    std::shared_ptr<Selection> selection;
    bool editable;
    bool isDisabled = false;
    bool expand = true;
    Optional<Binding<int>> indexModel;
    Optional<Binding<std::string>> textModel;
    Optional<Binding<std::string>> editBinding;
    int indexValue = -1;
    std::string textValue;
    std::function<void()> onChange;
    std::function<void()> onInput;
    static void collect(const std::vector<::CPPUI::Node *> * children, std::vector<Choice> & items, const std::string & prefix = {}) {
        for(auto node: *children) {
            if(node->is_virtual) collect(node->children, items, prefix + node->_id + "/");
            else if(auto item = dynamic_cast<ComboBoxItem *>(node)) items.push_back({item->text, item->value, prefix + item->_id});
        }
    }
public:
    ComboBox(bool editable, Optional<Binding<int>> index = {}, Optional<Binding<std::string>> text = {}): editable(editable), indexModel(index), textModel(text) {
        is_terminal = false;
        if(index) indexValue = index->get(); if(text) textValue = text->get();
    }
    std::size_t typecode() const override { return Widget::typecode() ^ (editable ? 0x7123U : 0U); }
    ComboBox & change(std::function<void()> callback) { onChange = std::move(callback); return *this; }
    ComboBox & input(std::function<void()> callback) { onInput = std::move(callback); return *this; }
    ComboBox & editModel(Binding<std::string> model) { editBinding = std::move(model); return *this; }
    ComboBox & autoexpand(bool value = true) { expand = value; return *this; }
    ComboBox & disabled(bool value = true) { isDisabled = value; return *this; }
    void updateWidget(Widget * previous) override {
        auto prev = static_cast<ComboBox *>(previous);
        selection = prev ? prev->selection : std::make_shared<Selection>();
        ui = prev ? prev->ui : new wxComboBox(parentWindow(), wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, nullptr, editable ? 0 : wxCB_READONLY);
        if(editable && !selection->completion) selection->completion = std::make_shared<ComboCompletion>(static_cast<wxComboBox *>(ui));
        if(selection->completion && (isDisabled || !expand)) selection->completion->hide();
        ui->Enable(!isDisabled);
    }
    void bindEvents() override {
        ui->Bind(wxEVT_COMBOBOX, &Native::clicked, native.get());
        ui->Bind(wxEVT_TEXT, &Native::typed, native.get());
    }
    void updateEvents() override {
        auto state = selection; auto object = native.get(); auto index = indexModel; auto text = textModel; auto callback = onChange;
        native->command = [state, object, index, text, callback]() mutable {
            if(state->applying || !object->window) return;
            int i = static_cast<wxComboBox *>(object->window)->GetSelection();
            std::string value;
            state->editing = false;
            if(state->completion) state->completion->hide();
            if(i >= 0 && i < static_cast<int>(state->items.size())) { auto item = state->items[i]; state->key = item.key; state->text = item.text; value = item.value; }
            else { state->key.reset(); state->text.clear(); }
            if(index) index->set(i); if(text) text->set(value); if(callback) callback();
        };
        auto draft = editBinding; auto input = onInput;
        if(editable) native->input = [state, object, index, text, callback, draft, input]() mutable {
            if(state->applying || state->committing || !object->window || !object->window->IsEnabled()) return;
            auto combo = static_cast<wxComboBox *>(object->window);
            auto value = combo->GetValue().ToStdString(wxConvUTF8); int i = combo->GetSelection();
            if(i >= 0 && i < static_cast<int>(state->items.size()) && value == state->items[i].text) return;
            state->key.reset(); state->text = value; state->editing = true;
            state->committing = true;
            if(state->completion) state->completion->request();
            try {
                if(index) index->set(-1); if(text) text->set(value);
                if(draft) draft->set(value); if(input) input(); if(callback) callback();
            } catch(...) { state->committing = false; throw; }
            state->committing = false;
            if(state->completion && object->node) state->completion->refresh();
        }; else native->input = {};
    }
    void destroy(bool direct) override {
        if(selection && selection->completion) selection->completion->hide();
        Widget::destroy(direct);
    }
    void postSync() override {
        if(!ui) return;
        std::vector<Choice> items; collect(children, items); selection->items = items;
        auto combo = static_cast<wxComboBox *>(ui);
        int index = -1; std::string text;
        if(indexModel) {
            if(indexValue >= 0 && indexValue < static_cast<int>(items.size())) index = indexValue;
            else if(editable) text = textModel ? textValue : selection->text;
        } else if(textModel) {
            text = textValue;
            if(!editable || !selection->editing || text != selection->text)
                for(std::size_t i = 0; i < items.size(); ++i) if(items[i].value == textValue) { index = i; break; }
        } else {
            text = editable ? selection->text : std::string();
            for(std::size_t i = 0; selection->key && i < items.size(); ++i) if(items[i].key == *selection->key) { index = i; break; }
        }
        if(index >= 0) text = items[index].text;
        wxArrayString labels; for(auto & item: items) labels.Add(wxString::FromUTF8(item.text));
        const auto oldText = combo->GetValue();
        const long cursor = combo->GetInsertionPoint();
        long start = 0, end = 0; if(editable) combo->GetSelection(&start, &end);
        selection->applying = true;
        if(combo->GetStrings() != labels) combo->Set(labels);
        if(selection->committing) { index = -1; text = selection->text; }
        if(combo->GetSelection() != index || combo->GetValue() != wxString::FromUTF8(text)) {
            combo->SetSelection(index); if(editable) combo->ChangeValue(wxString::FromUTF8(text));
        }
        if(editable && oldText == wxString::FromUTF8(text)) {
            if(start != end) combo->SetSelection(start, end); else combo->SetInsertionPoint(cursor);
        }
        selection->applying = false;
        if(selection->completion) {
            std::vector<std::string> keys;
            for(const auto & item: items) keys.push_back(item.key + '\0' + item.value);
            std::weak_ptr<Native> weak = native;
            selection->completion->configure(expand && !isDisabled, labels, keys, [weak](int index) {
                auto object = weak.lock();
                if(!object || !object->node || !object->window || !object->window->IsEnabled()) return;
                static_cast<wxComboBox *>(object->window)->SetSelection(index);
                wxCommandEvent event(wxEVT_COMBOBOX); object->clicked(event);
            });
        }
    }
};
inline void ComboBoxItem::postUpdate() { if(auto combo = dynamic_cast<ComboBox *>(dom_parent)) combo->postSync(); }
}
inline Wx::ComboBox & ComboBox(bool editable = false) { return *new Wx::ComboBox(editable); }
inline Wx::ComboBox & ComboBox(bool editable, Binding<int> index) { return *new Wx::ComboBox(editable, index); }
inline Wx::ComboBox & ComboBox(bool editable, Binding<std::string> text) { return *new Wx::ComboBox(editable, {}, text); }
inline Wx::ComboBox & ComboBox(bool editable, Binding<int> index, Binding<std::string> text) { return *new Wx::ComboBox(editable, index, text); }
inline Wx::ComboBoxItem & ComboBoxItem(const std::string & text) { return *new Wx::ComboBoxItem(text, text); }
inline Wx::ComboBoxItem & ComboBoxItem(const std::string & text, const std::string & value) { return *new Wx::ComboBoxItem(text, value); }
}
#endif

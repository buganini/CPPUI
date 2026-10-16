#ifndef CPPUI_LVGL_COMBOBOX
#define CPPUI_LVGL_COMBOBOX

#include "base.hpp"
#include "../binding.hpp"

namespace CPPUI { namespace LVGL {
    class ComboBoxItem: public ::CPPUI::Node {
        public:
        std::string text, value;
        ComboBoxItem(const std::string & text, const std::string & value): text(text), value(value) {
            id(text); is_terminal = true;
        }
        void destroy(bool) override {}
        void postUpdate() override;
        std::vector<std::string> identity() const {
            std::vector<std::string> key;
            for(auto node = static_cast<const ::CPPUI::Node *>(this); node && node != dom_parent; node = node->parent) {
                key.push_back(std::to_string(node->typecode()) + (node->has_id ? ":id:" + node->_id : ":none"));
                if(node == node->parent) break;
            }
            return key;
        }
    };
    class ComboBox: public Node {
        struct Choice { std::string text, value; std::vector<std::string> key; };
        struct Runtime: std::enable_shared_from_this<Runtime> {
            ComboBox * current = nullptr;
            lv_obj_t * object = nullptr, * dropdown = nullptr, * editor = nullptr;
            std::vector<Choice> items;
            std::vector<int> visible;
            std::vector<std::string> key;
            std::string freeText, options;
            int selected = -1;
            bool applying = false, committing = false, filtering = false, openingSuggestions = false;
            ~Runtime() {
                for(auto child: {object, dropdown, editor})
                    if(child) lv_obj_remove_event_cb_with_user_data(child, event, this);
            }
            static void event(lv_event_t * event) {
                auto self = static_cast<Runtime *>(lv_event_get_user_data(event))->shared_from_this();
                auto target = lv_event_get_target_obj(event);
                const auto code = lv_event_get_code(event);
                if(code == LV_EVENT_DELETE) {
                    if(target == self->object) { self->object = nullptr; self->current = nullptr; }
                    if(target == self->dropdown) self->dropdown = nullptr;
                    if(target == self->editor) self->editor = nullptr;
                    return;
                }
                if(self->applying || !self->current || self->current->isDisabled) return;
                try {
                    if(code == LV_EVENT_READY && target == self->dropdown && !self->openingSuggestions) {
                        self->filtering = false;
                        self->current->postSync();
                    } else if(code == LV_EVENT_VALUE_CHANGED) {
                        if(target == self->editor) self->current->typed();
                        else if(target == self->dropdown) self->current->chosen();
                    } else if(target == self->dropdown && self->selected < 0 && !lv_dropdown_is_open(target)
                              && (code == LV_EVENT_RELEASED || (code == LV_EVENT_KEY && lv_event_get_key(event) == LV_KEY_ENTER))) {
                        // LVGL suppresses VALUE_CHANGED when keyboard Enter
                        // accepts its initial option 0; our initial state is empty.
                        self->current->chosen();
                    }
                } catch(const std::exception & error) { std::fprintf(stderr, "CPPUI LVGL ComboBox: %s\n", error.what()); }
                catch(...) { std::fprintf(stderr, "CPPUI LVGL ComboBox: unknown exception\n"); }
            }
        };
        std::shared_ptr<Runtime> runtime;
        bool editable, isDisabled = false;
        Optional<Binding<int>> indexModel;
        Optional<Binding<std::string>> textModel;
        int indexValue = -1;
        std::string textValue;
        std::function<void()> onChange;
        void retire() { if(runtime && runtime->current == this) runtime->current = nullptr; }
        static bool startsWith(const std::string & label, const std::string & prefix) {
            if(label.size() < prefix.size()) return false;
            for(std::size_t i = 0; i < prefix.size(); ++i) {
                auto lower = [](unsigned char c) { return c >= 'A' && c <= 'Z' ? c + ('a'-'A') : c; };
                if(lower(label[i]) != lower(prefix[i])) return false;
            }
            return true;
        }
        void changed(int index, const std::string & value) {
            // Bindings can synchronously redraw/remove the declaration. Retain
            // only copied bindings/callbacks across the first application write.
            auto indices = indexModel; auto texts = textModel; auto invoke = onChange;
            auto state = runtime;
            state->committing = true;
            try {
                if(indices) indices->set(index);
                if(texts) texts->set(value);
                if(invoke) invoke();
            } catch(...) { state->committing = false; throw; }
            state->committing = false;
            if(state->current && state->object) state->current->postSync();
        }
        void chosen() {
            auto state = runtime;
            if(!state->dropdown) return;
            const auto position = lv_dropdown_get_selected(state->dropdown);
            if(position >= state->visible.size()) return;
            const int index = state->visible[position];
            const auto item = state->items[index];
            state->key = item.key; state->freeText = item.text; state->selected = index;
            // Update unbound controls too; native text must be current before writes.
            display(index, item.text);
            changed(index, item.value);
        }
        void typed() {
            auto state = runtime;
            const std::string value = lv_textarea_get_text(state->editor);
            state->key.clear(); state->freeText = value; state->selected = -1;
            state->filtering = true;
            display(-1, value);
            state->openingSuggestions = true;
            if(!value.empty() && !state->visible.empty()) lv_dropdown_open(state->dropdown);
            else lv_dropdown_close(state->dropdown);
            state->openingSuggestions = false;
            changed(-1, value);
        }
        void display(int selected, const std::string & text) {
            auto state = runtime;
            if(!state || !state->dropdown) return;
            state->applying = true;
            state->selected = selected;
            if(selected >= 0 || text != state->freeText) state->filtering = false;
            std::string options;
            state->visible.clear();
            int nativeIndex = 0;
            for(std::size_t i = 0; i < state->items.size(); ++i) {
                const auto & item = state->items[i];
                if(state->filtering && !text.empty() && !startsWith(item.text, text)) continue;
                if(!state->visible.empty()) options += '\n';
                // LVGL uses newline as its option separator; keep one item per row.
                auto label = item.text;
                std::replace(label.begin(), label.end(), '\n', ' ');
                options += label;
                if(static_cast<int>(i) == selected) nativeIndex = state->visible.size();
                state->visible.push_back(i);
            }
            if(state->options != options) {
                // LVGL 9.6's popup label borrows the option buffer, but
                // set_options frees that buffer without rebinding the label.
                auto label = lv_obj_get_child(lv_dropdown_get_list(state->dropdown), 0);
                lv_label_set_text_static(label, "");
                lv_dropdown_set_options(state->dropdown, options.c_str()); state->options = options;
                lv_label_set_text_static(label, lv_dropdown_get_options(state->dropdown));
            }
            lv_dropdown_set_selected(state->dropdown, nativeIndex);
            lv_dropdown_set_selected_highlight(state->dropdown, selected >= 0);
            lv_dropdown_set_text(state->dropdown, editable || selected < 0 ? "" : nullptr);
            lv_obj_set_state(state->dropdown, LV_STATE_DISABLED, isDisabled || state->items.empty());
            if(state->editor) {
                lv_obj_set_state(state->editor, LV_STATE_DISABLED, isDisabled);
                if(text != lv_textarea_get_text(state->editor)) lv_textarea_set_text(state->editor, text.c_str());
            }
            if(isDisabled || state->visible.empty()) lv_dropdown_close(state->dropdown);
            else if(lv_dropdown_is_open(state->dropdown)) lv_dropdown_open(state->dropdown);
            state->applying = false;
        }
        public:
        ComboBox(bool editable, Optional<Binding<int>> index = {}, Optional<Binding<std::string>> text = {})
            : editable(editable), indexModel(index), textModel(text) {
            if(index) indexValue = index->get();
            if(text) textValue = text->get();
        }
        std::size_t typecode() const override { return Node::typecode() ^ (editable ? 0x7123U : 0U); }
        ComboBox & change(std::function<void()> callback) { onChange = std::move(callback); return *this; }
        ComboBox & disabled(bool value = true) { isDisabled = value; return *this; }
        ~ComboBox() override { retire(); }
        void destroy(bool direct) override { retire(); Node::destroy(direct); }
        void update(Node * previous) override {
            auto prev = static_cast<ComboBox *>(previous);
            if(prev && prev->ui) { ui = prev->ui; runtime = prev->runtime; }
            else {
                runtime = std::make_shared<Runtime>();
                auto parentObject = static_cast<lv_obj_t *>(parent->inner());
                if(editable) {
                    runtime->object = lv_obj_create(parentObject);
                    lv_obj_set_size(runtime->object, 240, LV_SIZE_CONTENT);
                    lv_obj_set_style_pad_all(runtime->object, 0, 0);
                    lv_obj_set_style_border_width(runtime->object, 0, 0);
                    lv_obj_set_style_bg_opa(runtime->object, LV_OPA_TRANSP, 0);
                    lv_obj_set_scrollable(runtime->object, false);
                    lv_obj_set_flex_flow(runtime->object, LV_FLEX_FLOW_ROW);
                    lv_obj_set_style_pad_column(runtime->object, 0, 0);
                    runtime->editor = lv_textarea_create(runtime->object);
                    lv_textarea_set_one_line(runtime->editor, true);
                    lv_obj_set_width(runtime->editor, 0); lv_obj_set_flex_grow(runtime->editor, 1);
                    runtime->dropdown = lv_dropdown_create(runtime->object);
                    lv_obj_set_width(runtime->dropdown, 40);
                    lv_obj_add_event_cb(runtime->editor, Runtime::event, LV_EVENT_ALL, runtime.get());
                    lv_obj_add_event_cb(runtime->object, Runtime::event, LV_EVENT_DELETE, runtime.get());
                } else runtime->object = runtime->dropdown = lv_dropdown_create(parentObject);
                lv_dropdown_set_options(runtime->dropdown, "");
                lv_obj_add_event_cb(runtime->dropdown, Runtime::event, LV_EVENT_ALL, runtime.get());
                ui = runtime->object;
            }
            runtime->current = this;
        }
        void addChild(int index, ::CPPUI::Node * child) override {
            if(auto item = dynamic_cast<ComboBoxItem *>(child))
                runtime->items.insert(runtime->items.begin() + index, {item->text, item->value, item->identity()});
        }
        void removeChild(int index, ::CPPUI::Node *) override {
            if(index >= 0 && index < static_cast<int>(runtime->items.size())) {
                runtime->items.erase(runtime->items.begin() + index); postSync();
            }
        }
        void updateItem(ComboBoxItem * item) {
            int index = 0;
            if(findDomOffsetForNode(item, &index) && index < static_cast<int>(runtime->items.size())) {
                runtime->items[index] = {item->text, item->value, item->identity()}; postSync();
            }
        }
        void postSync() override {
            if(!runtime || !ui || runtime->committing) return;
            const auto & items = runtime->items;
            int index = -1; std::string text;
            if(indexModel) {
                if(indexValue >= 0 && indexValue < static_cast<int>(items.size())) index = indexValue;
                else if(editable) text = textModel ? textValue : runtime->freeText;
            } else if(textModel) {
                text = textValue;
                for(std::size_t i = 0; i < items.size(); ++i) if(items[i].value == textValue) { index = i; break; }
            } else {
                if(editable) text = runtime->freeText;
                for(std::size_t i = 0; !runtime->key.empty() && i < items.size(); ++i)
                    if(items[i].key == runtime->key) { index = i; break; }
            }
            if(index >= 0) text = items[index].text;
            display(index, text);
        }
    };
    inline void ComboBoxItem::postUpdate() { if(auto combo = dynamic_cast<ComboBox *>(dom_parent)) combo->updateItem(this); }
}}
namespace CPPUI {
    inline LVGL::ComboBox & ComboBox(bool editable = false) { return *new LVGL::ComboBox(editable); }
    inline LVGL::ComboBox & ComboBox(bool editable, Binding<int> index) { return *new LVGL::ComboBox(editable, index); }
    inline LVGL::ComboBox & ComboBox(bool editable, Binding<std::string> text) { return *new LVGL::ComboBox(editable, {}, text); }
    inline LVGL::ComboBox & ComboBox(bool editable, Binding<int> index, Binding<std::string> text) { return *new LVGL::ComboBox(editable, index, text); }
    inline LVGL::ComboBoxItem & ComboBoxItem(const std::string & text) { return *new LVGL::ComboBoxItem(text, text); }
    inline LVGL::ComboBoxItem & ComboBoxItem(const std::string & text, const std::string & value) { return *new LVGL::ComboBoxItem(text, value); }
}
#endif

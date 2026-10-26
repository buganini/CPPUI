#ifndef CPPUI_LVGL_TEXTEDIT
#define CPPUI_LVGL_TEXTEDIT

#include "base.hpp"
#include "../binding.hpp"

namespace CPPUI { namespace LVGL {
    class TextEdit: public Node {
        struct Runtime {
            TextEdit * current = nullptr;
            lv_obj_t * object = nullptr;
            bool editing = false, committing = false, updating = false;
            bool pending = false, syncingDraft = false;
            OptionalText modelValue;
            void cancel() {
                if(pending) lv_async_call_cancel(syncDraft, this);
                pending = false;
            }
            static void deleted(lv_event_t * event) {
                auto state = static_cast<Runtime *>(lv_event_get_user_data(event));
                state->object = nullptr;
                state->cancel();
            }
            ~Runtime() {
                cancel();
                if(object) lv_obj_remove_event_cb_with_user_data(object, deleted, this);
            }
            static void syncDraft(void * data) {
                auto state = static_cast<Runtime *>(data);
                state->pending = false;
                if(!state->current || !state->object || state->editing || state->committing) return;
                auto keep = state->current->runtime;
                auto draft = state->current->editModel;
                const auto value = state->current->value;
                if(!draft) return;
                // State writes redraw synchronously: defer this until native
                // reconciliation is over, and retain no declaration across it.
                state->syncingDraft = true;
                try { draft->set(value); }
                catch(const std::exception & error) { std::fprintf(stderr, "CPPUI LVGL TextEdit draft: %s\n", error.what()); }
                catch(...) { std::fprintf(stderr, "CPPUI LVGL TextEdit draft: unknown exception\n"); }
                state->syncingDraft = false;
            }
            void setText(const std::string & text) {
                updating = true;
                lv_textarea_set_text(object, text.c_str());
                updating = false;
            }
        };
        std::shared_ptr<Runtime> runtime;
        Binding<std::string> model;
        Optional<Binding<std::string>> editModel;
        std::string value, editValue, placeholderText;
        protected:
        bool isReadonly=false,wrapText=true;
        private:
        std::function<void()> onInput, onChange;
        void retire() {
            if(runtime && runtime->current == this) {
                runtime->current = nullptr;
                runtime->cancel();
            }
        }
        protected:
        void applyWrap() {
            if(!ui) return;
            auto label=lv_textarea_get_label(static_cast<lv_obj_t *>(ui));
            lv_label_set_long_mode(label,wrapText ? LV_LABEL_LONG_MODE_WRAP : LV_LABEL_LONG_MODE_CLIP);
            lv_obj_set_width(label,wrapText ? lv_obj_get_content_width(static_cast<lv_obj_t *>(ui)) : LV_SIZE_CONTENT);
        }
        void nativeEvent(lv_event_t * event) override {
            const auto code = lv_event_get_code(event);
            if(code==LV_EVENT_INSERT && isReadonly && runtime && !runtime->updating) {
                lv_textarea_set_insert_replace(static_cast<lv_obj_t *>(ui),"");return;
            }
            if(code==LV_EVENT_SIZE_CHANGED) applyWrap();
            if(code == LV_EVENT_VALUE_CHANGED) {
                auto state = runtime;
                if(state->updating || !state->current || isReadonly) return;
                applyWrap();
                auto draft = editModel;
                auto invoke = onInput;
                const std::string text = lv_textarea_get_text(state->object);
                state->editing = true;
                state->cancel();
                if(draft) draft->set(text);
                if(state->current && state->object && invoke) invoke();
            } else if(code == LV_EVENT_DEFOCUSED) commit();
            else Node::nativeEvent(event);
        }
        public:
        explicit TextEdit(Binding<std::string> model): model(model), value(model.get()) { is_terminal = true; expandX=expandY=4; }
        TextEdit(Binding<std::string> model, Binding<std::string> editModel): TextEdit(model) {
            this->editModel = editModel;
            editValue = editModel.get();
        }
        TextEdit & input(std::function<void()> callback) { onInput = std::move(callback); return *this; }
        TextEdit & change(std::function<void()> callback) { onChange = std::move(callback); return *this; }
        TextEdit & placeholder(const std::string & text) { placeholderText = text; return *this; }
        TextEdit & readonly(bool value=true) {isReadonly=value;return *this;}
        TextEdit & wrap(bool value=true) {wrapText=value;return *this;}
        ~TextEdit() override { retire(); }
        void destroy(bool direct) override { retire(); Node::destroy(direct); }
        void update(Node * previous) override {
            auto prev = static_cast<TextEdit *>(previous);
            if(prev && prev->ui) {
                ui = prev->ui;
                runtime = prev->runtime;
            } else {
                ui = lv_textarea_create(static_cast<lv_obj_t *>(parent->inner()));
                lv_textarea_set_one_line(static_cast<lv_obj_t *>(ui), false);
                runtime = std::make_shared<Runtime>();
                runtime->object = static_cast<lv_obj_t *>(ui);
                lv_obj_add_event_cb(runtime->object, Runtime::deleted, LV_EVENT_DELETE, runtime.get());
            }
            runtime->current = this;
            if(isReadonly) runtime->editing=false;
            applyWrap();
            lv_textarea_set_placeholder_text(runtime->object, placeholderText.c_str());
            if(!runtime->editing && !runtime->committing) {
                if(std::string(lv_textarea_get_text(runtime->object))!=value) {
                    runtime->setText(value);
                    runtime->modelValue = value;
                }
                runtime->cancel();
                if(editModel && editValue != value && !runtime->syncingDraft)
                    runtime->pending = lv_async_call(Runtime::syncDraft, runtime.get()) == LV_RESULT_OK;
            }
        }
        void commit() {
            auto state = runtime;
            if(isReadonly || !state || !state->current || !state->object || !state->editing || state->committing) return;
            auto binding = model;
            auto draft = editModel;
            auto invoke = onChange;
            const std::string text = lv_textarea_get_text(state->object);
            state->committing = true;
            state->cancel();
            try {
                binding.set(text);
                if(state->current && state->object) {
                    const auto committed = binding.get();
                    if(state->current && state->object) {
                        state->setText(committed);
                        state->modelValue = committed;
                        state->editing = false;
                        if(draft) draft->set(committed);
                        if(state->current && state->object && invoke) invoke();
                    }
                }
            } catch(...) { state->committing = false; throw; }
            state->committing = false;
        }
    };
}}
namespace CPPUI {
    inline LVGL::TextEdit & TextEdit(Binding<std::string> model) { return *new LVGL::TextEdit(model); }
    inline LVGL::TextEdit & TextEdit(Binding<std::string> model,Binding<std::string> edit) { return *new LVGL::TextEdit(model,edit); }
}
#endif

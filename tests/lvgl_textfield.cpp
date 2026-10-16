#include "cppui/lvgl/textfield.hpp"
#include "cppui/lvgl/window.hpp"
#include "cppui/lvgl/layout.hpp"
#include "cppui/lvgl/button.hpp"
#include "cppui/lvgl/views.hpp"
#include <stdexcept>

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    void settle() { for(int i = 0; i < 3; ++i) { lv_tick_inc(1); lv_timer_handler(); } }
    std::string text(lv_obj_t * object) { return lv_textarea_get_text(object); }
    void key(lv_obj_t * object, uint32_t value) { lv_obj_send_event(object, LV_EVENT_KEY, &value); }
    void deleted(lv_event_t * event) { ++*static_cast<int *>(lv_event_get_user_data(event)); }
    class Root: public CPPUI::LVGL::View {
        public:
        CPPUI::State<std::string> first{std::string("first")}, second{std::string("second")}, draft{std::string()};
        bool other = false, show = true, failGet = false, throwSet = false, throwDraft = false, withDraft = true;
        int writes = 0, inputs = 0, changes = 0, version = 1, draftWrites = 0;
        std::function<void()> onInput, onCommit, onSet, onDraft;
        std::weak_ptr<int> capture;
        std::string seen;
        std::string placeholder = "Enter text";
        bool setPlaceholder = true, setDisabled = true, masked = false;
        CPPUI::State<bool> disabled{false};
        void content() override {
            for(auto window: CPPUI::Window()) for(auto column: CPPUI::VBox()) {
                if(show) {
                    auto token = std::make_shared<int>(1); capture = token;
                    auto state = other ? &second : &first;
                    CPPUI::Binding<std::string> binding([this, state]() {
                        if(failGet) throw std::runtime_error("expected getter failure");
                        return static_cast<std::string>(*state);
                    }, [this, state, token](const std::string & value) {
                        ++writes;
                        if(throwSet) throw std::runtime_error("expected setter failure");
                        auto invoke = onSet;
                        if(value != "reject") *state = value == "normalize" ? "NORMALIZED" : value;
                        if(invoke) invoke();
                        require(*token == 1, "active binding lost its captures");
                    });
                    CPPUI::Binding<std::string> edit([this]() { return static_cast<std::string>(draft); },
                        [this](const std::string & value) {
                            ++draftWrites;
                            if(throwDraft) throw std::runtime_error("expected draft failure");
                            auto invoke = onDraft; draft = value; if(invoke) invoke();
                        });
                    const auto current = version;
                    auto & field = masked
                        ? (withDraft ? CPPUI::MaskedField(binding, edit) : CPPUI::MaskedField(binding))
                        : (withDraft ? CPPUI::TextField(binding, edit) : CPPUI::TextField(binding));
                    if(setPlaceholder) field.placeholder(std::string(placeholder));
                    field.input([this, current]() {
                        inputs += current; auto invoke = onInput; if(invoke) invoke();
                    }).change([this, current]() {
                        changes += current; auto invoke = onCommit; if(invoke) invoke();
                    }).id("field");
                }
                auto & button = CPPUI::Button("read model");
                if(setDisabled) button.disabled(disabled);
                button.click([this]() { seen = static_cast<std::string>(other ? second : first); });
            }
        }
        CPPUI::LVGL::TextField * field() { return static_cast<CPPUI::LVGL::TextField *>(children->front()->children->front()->children->front()); }
        lv_obj_t * editor() { return static_cast<lv_obj_t *>(field()->ui); }
        lv_obj_t * button() { return static_cast<lv_obj_t *>(children->front()->children->front()->children->back()->ui); }
    };
    class Pages: public CPPUI::LVGL::View {
        public:
        CPPUI::State<std::string> model{std::string("saved")};
        CPPUI::State<CPPUI::OptionalText> selection{CPPUI::OptionalText("editor")};
        void content() override {
            for(auto window: CPPUI::Window()) for(auto pages: CPPUI::Views(selection)) {
                CPPUI::TextField(model).id("editor");
                CPPUI::Button("other page").id("other");
            }
        }
        lv_obj_t * editor() { return static_cast<lv_obj_t *>(children->front()->children->front()->children->front()->children->front()->ui); }
    };
}
int main() {
    lv_init();
    auto display = lv_display_create(800, 600);
    auto group = lv_group_create(); lv_group_set_default(group);
    int result = 0;
    try {
        for(bool masked: {false, true}) {
            Root root; root.masked = masked; root.redraw(); settle();
            auto editor = root.editor();
            require(lv_textarea_get_password_mode(editor) == masked, "wrong masking mode");
            require(std::string(lv_textarea_get_placeholder_text(editor)) == "Enter text", "TextField lost its placeholder");
            auto button = root.button();
            for(int i = 0; i < 4; ++i) {
                root.setDisabled = i != 3;
                root.disabled = i != 1; settle();
                require(root.button() == button
                    && lv_obj_has_state(button, LV_STATE_DISABLED) == (root.setDisabled && static_cast<bool>(root.disabled)),
                    "Button disabled update/default reset remounted it or left stale native state");
            }
            require(lv_textarea_get_one_line(editor) && text(editor) == "first" && static_cast<std::string>(root.draft) == "first"
                && root.draftWrites == 1 && root.writes == 0 && root.inputs == 0, "initial model/draft sync emitted input or repeated writes");
            auto capture = root.capture;
            lv_group_focus_obj(editor);
            lv_group_send_data(group, '!'); settle();
            require(text(editor) == "first!" && static_cast<std::string>(root.first) == "first"
                && static_cast<std::string>(root.draft) == "first!" && root.inputs == 1 && root.writes == 0 && capture.expired(),
                "keyboard input did not update only the draft or retire old callbacks");
            const auto cursor = lv_textarea_get_cursor_pos(editor);
            for(int i = 0; i < 4; ++i) {
                root.placeholder = i == 1 ? "" : "Updated hint";
                root.setPlaceholder = i != 3;
                root.redraw(); settle();
                const auto hint = lv_textarea_get_placeholder_text(editor);
                require(root.editor() == editor && std::string(hint ? hint : "") == (root.setPlaceholder ? root.placeholder : "")
                    && text(editor) == "first!" && lv_textarea_get_cursor_pos(editor) == cursor,
                    "placeholder update/default reset changed native identity, draft or cursor");
                require(static_cast<std::string>(root.first) == "first" && static_cast<std::string>(root.draft) == "first!"
                    && root.inputs == 1 && root.writes == 0 && root.changes == 0,
                    "placeholder update changed the model or emitted feedback");
            }
            root.first = std::string("outside"); root.version = 2; root.redraw(); settle();
            require(root.editor() == editor && text(editor) == "first!" && lv_textarea_get_cursor_pos(editor) == cursor,
                "redraw overwrote an active draft/cursor");
            lv_group_send_data(group, LV_KEY_ENTER); settle();
            require(static_cast<std::string>(root.first) == "first!" && root.writes == 1 && root.changes == 2 && root.inputs == 1,
                "Enter commit used an old callback or caused input feedback");
            lv_obj_send_event(editor, LV_EVENT_DEFOCUSED, nullptr);
            lv_obj_send_event(editor, LV_EVENT_READY, nullptr);
            require(root.writes == 1, "Enter followed by blur committed twice");
            root.first = std::string("programmatic"); settle();
            require(text(editor) == "programmatic" && static_cast<std::string>(root.draft) == "programmatic"
                && root.inputs == 1 && root.writes == 1, "programmatic update emitted user input or left stale draft");
            lv_textarea_set_text(editor, "new binding"); root.other = true; root.redraw();
            lv_obj_send_event(editor, LV_EVENT_READY, nullptr); settle();
            require(static_cast<std::string>(root.second) == "new binding" && static_cast<std::string>(root.first) == "programmatic",
                "commit used a retired model");
            lv_textarea_set_text(editor, "reject"); key(editor, LV_KEY_ENTER); settle();
            require(text(editor) == "new binding" && static_cast<std::string>(root.draft) == "new binding", "rejected input did not restore committed model/draft");
            lv_textarea_set_text(editor, "normalize"); key(editor, LV_KEY_ENTER); settle();
            require(text(editor) == "NORMALIZED" && static_cast<std::string>(root.second) == "NORMALIZED"
                && static_cast<std::string>(root.draft) == "NORMALIZED", "normalized model was not read back");
            root.failGet = true;
            bool failed = false; try { root.redraw(); } catch(const std::runtime_error &) { failed = true; }
            require(failed && root.editor() == editor, "getter failure replaced mounted content");
            root.failGet = false; root.redraw();
            root.throwSet = true; lv_textarea_set_text(editor, "throws"); key(editor, LV_KEY_ENTER); settle();
            root.second = std::string("outside after failure"); settle();
            require(text(editor) == "throws", "throwing setter lost the editable draft");
            root.throwSet = false; key(editor, LV_KEY_ENTER); settle();
            require(static_cast<std::string>(root.second) == "throws", "failed commit could not be retried");
            lv_group_focus_obj(editor); lv_textarea_set_text(editor, "before button");
            lv_group_focus_obj(root.button());
            lv_obj_send_event(root.button(), LV_EVENT_CLICKED, nullptr); settle();
            require(root.seen == "before button", "focus loss did not commit before button callback");
            root.onInput = []() { throw std::runtime_error("expected input callback failure"); };
            root.onCommit = []() { throw std::runtime_error("expected change callback failure"); };
            root.redraw(); lv_textarea_set_text(editor, "callback failure"); key(editor, LV_KEY_ENTER); settle();
            require(static_cast<std::string>(root.second) == "callback failure", "callback exception escaped native boundary or lost commit");
            root.onInput = {}; root.onCommit = {}; root.throwDraft = true; root.redraw();
            lv_textarea_set_text(editor, "draft failure"); key(editor, LV_KEY_ENTER); settle();
            root.throwDraft = false; root.redraw(); settle();
            require(static_cast<std::string>(root.draft) == "draft failure", "draft setter failure could not recover");
            int removed = 0; lv_obj_add_event_cb(editor, deleted, LV_EVENT_DELETE, &removed);
            root.onCommit = [&]() { root.show = false; root.redraw(); }; root.redraw();
            lv_textarea_set_text(editor, "remove"); key(editor, LV_KEY_ENTER); settle();
            require(removed == 1 && root.capture.expired(), "commit callback removal retained native data/captures");
            root.onCommit = {}; root.show = true; root.redraw(); settle(); editor = root.editor();
            root.onInput = [&]() { root.show = false; root.redraw(); }; root.redraw();
            lv_textarea_set_text(editor, "remove on input"); settle();
            require(root.capture.expired(), "input callback removal retained captures");
            root.onInput = {}; root.show = true; root.redraw(); settle();
            root.onSet = [&]() { root.show = false; root.redraw(); }; root.redraw();
            editor = root.editor(); lv_textarea_set_text(editor, "remove in setter"); key(editor, LV_KEY_ENTER); settle();
            require(root.capture.expired(), "model setter removal retained captures");
        }
        {
            Root root; root.withDraft = false; root.redraw();
            auto editor = root.editor(); lv_textarea_set_text(editor, "no draft binding");
            key(editor, LV_KEY_ENTER); settle();
            require(static_cast<std::string>(root.first) == "no draft binding" && root.draftWrites == 0, "model-only field did not commit");
        }
        {
            Pages root; root.redraw(); auto editor = root.editor();
            lv_textarea_set_text(editor, "unfinished page");
            root.selection = CPPUI::OptionalText("other"); root.model = std::string("external");
            root.selection = CPPUI::OptionalText("editor"); settle();
            require(root.editor() == editor && text(editor) == "unfinished page", "page switching lost native editor or unfinished draft");
            key(editor, LV_KEY_ENTER);
            require(static_cast<std::string>(root.model) == "unfinished page", "retained page could not commit");
        }
        for(int mode = 0; mode < 4; ++mode) {
            std::unique_ptr<Root> root(new Root()); root->redraw(); settle();
            auto action = [&]() { root.reset(); };
            if(mode == 0) root->onInput = action;
            if(mode == 1) root->onCommit = action;
            if(mode == 2) root->onSet = action;
            if(mode == 3) root->onDraft = action;
            root->redraw(); auto editor = root->editor();
            lv_textarea_set_text(editor, "delete owner");
            if(root) key(editor, LV_KEY_ENTER);
            settle(); require(!root, "active text event could not destroy owner");
        }
        {
            std::unique_ptr<Root> root(new Root());
            root->onDraft = [&]() { root.reset(); };
            root->redraw(); settle();
            require(!root, "deferred draft synchronization could not destroy owner");
        }
        {
            Root root; root.redraw();
            auto declaration = root.field(); lv_obj_delete(root.editor()); settle();
            require(!declaration->ui && root.draftWrites == 0, "native deletion left stale access or queued draft write");
            root.redraw(); settle();
            require(text(root.editor()) == "first", "externally deleted editor could not remount");
        }
        {
            Root root; root.redraw(); root.show = false; root.redraw(); settle();
            require(root.draftWrites == 0 && root.capture.expired(), "retirement left a pending draft write");
        }
        require(lv_obj_get_child_count(lv_screen_active()) == 0 && lv_group_get_obj_count(group) == 0,
            "TextField teardown retained native content or focus objects");
        {
            Root root; root.redraw(); lv_display_delete(display); settle();
            require(!root.field()->ui && root.draftWrites == 0, "display teardown retained native access or draft task");
        }
    } catch(const std::exception & error) { std::cerr << error.what() << '\n'; result = 1; }
    lv_group_delete(group);
    lv_deinit();
    return result;
}

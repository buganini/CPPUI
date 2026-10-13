#include "cppui/lvgl/application.hpp"
#include "cppui/lvgl/window.hpp"
#include "cppui/lvgl/label.hpp"
#include "cppui/lvgl/button.hpp"
#include "cppui/lvgl/layout.hpp"
#include "cppui/state.hpp"
#include <memory>
#include <stdexcept>

namespace {
    int alive = 0;
    int retired = 0;
    bool validTransfer = true;
    class Label: public CPPUI::LVGL::Label {
        public:
        explicit Label(const std::string & text): CPPUI::LVGL::Label(text) { ++alive; }
        ~Label() override { --alive; }
        void releaseRetiredRefs() override {
            // Internal lookup is valid while reconciliation still owns both
            // declarations, before the predecessor is physically reclaimed.
            auto current = getNode();
            validTransfer = validTransfer && current != this && current->outer() == outer()
                && current->parent != parent;
            ++retired;
            CPPUI::LVGL::Label::releaseRetiredRefs();
        }
    };
    class Root: public CPPUI::Application {
        public:
        using Application::Application;
        bool show = true;
        int value = 0;
        void content() override {
            for(auto window: CPPUI::Window()) {
                if(show) (new Label(std::to_string(value)))->id("value");
            }
        }
    };
    void require(bool value, const char * message) {
        if(!value) throw std::runtime_error(message);
    }
    CPPUI::Node * find(CPPUI::Node & node, const std::string & id) {
        if(node.has_id && node._id == id) return &node;
        for(auto child: *node.children) if(auto result = find(*child, id)) return result;
        return nullptr;
    }
    void counted(lv_event_t * event) { ++*static_cast<int *>(lv_event_get_user_data(event)); }

    uint32_t loopTick = 0;
    unsigned loopDelays = 0;
    uint32_t readLoopTick() { return loopTick; }
    void advanceLoopTick(uint32_t milliseconds) {
        require(++loopDelays <= 10, "application loop failed to return after native close");
        require(milliseconds <= LV_DEF_REFR_PERIOD, "application slept for an invalid timer delay");
        loopTick += milliseconds;
    }
    void closeDisplay(lv_timer_t * timer) {
        lv_display_delete(static_cast<lv_display_t *>(lv_timer_get_user_data(timer)));
    }
    void applicationClose(int argc, char ** argv) {
        while(auto display = lv_display_get_next(nullptr)) lv_display_delete(display);
        lv_tick_set_cb(readLoopTick);
        lv_delay_set_cb(advanceLoopTick);
        {
            Root root(argc, argv);
            root.start();
            require(loopDelays == 0, "application slept with no display");
        }
        {
            auto display = lv_display_create(800, 600);
            Root root(argc, argv);
            root.redraw();
            auto window = root.children->front();
            auto label = window->children->front();
            auto timer = lv_timer_create(closeDisplay, 1, display);
            lv_timer_set_repeat_count(timer, 1);
            lv_timer_ready(timer);
            root.start();
            require(!lv_display_get_next(nullptr) && !window->ui && !label->ui,
                    "native close did not invalidate the mounted application");
            require(loopDelays == 0, "application slept after its last display closed");
        }
        require(alive == 0, "native close retained application declarations");
        {
            auto first = lv_display_create(800, 600);
            auto last = lv_display_create(800, 600);
            // These displays test loop ownership without a renderer.
            lv_timer_pause(lv_display_get_refr_timer(first));
            lv_timer_pause(lv_display_get_refr_timer(last));
            auto finalClose = lv_timer_create(closeDisplay, 5, last);
            lv_timer_set_repeat_count(finalClose, 1);
            auto firstClose = lv_timer_create(closeDisplay, 1, first);
            lv_timer_set_repeat_count(firstClose, 1);
            lv_timer_ready(firstClose);
            Root root(argc, argv);
            root.start();
            require(!lv_display_get_next(nullptr) && loopTick >= 5 && loopDelays > 0,
                    "application stopped while another display was still open");
        }
        lv_delay_set_cb(nullptr);
        lv_tick_set_cb(nullptr);
    }

    class ClickRoot: public CPPUI::LVGL::View {
        public:
        bool show = true;
        bool fail = false;
        int value = 0;
        int clicked = 0;
        std::weak_ptr<int> capture;
        std::function<void()> action;
        void content() override {
            for(auto window: CPPUI::Window()) {
                for(auto column: CPPUI::VBox().id("column")) {
                    if(show) {
                        auto token = std::make_shared<int>(value);
                        capture = token;
                        CPPUI::Button(std::to_string(value)).id("button").click([this, token]() {
                            clicked = *token;
                            auto invoke = action;
                            if(invoke) invoke();
                            // The capture must remain alive after synchronous removal.
                            require(*token == clicked, "active callback capture was corrupted");
                        });
                    }
                    if(fail) throw std::runtime_error("builder failure");
                }
            }
        }
        CPPUI::LVGL::Button * button() { return static_cast<CPPUI::LVGL::Button *>(find(*this, "button")); }
    };
    void callbacksAndDeletion() {
        int clicks = 0, deletes = 0;
        {
            ClickRoot root;
            root.redraw();
            auto object = static_cast<lv_obj_t *>(root.button()->ui);
            lv_obj_add_event_cb(object, counted, LV_EVENT_CLICKED, &clicks);
            lv_obj_add_event_cb(object, counted, LV_EVENT_DELETE, &deletes);
            auto oldCapture = root.capture;
            root.value = 7;
            root.redraw();
            require(oldCapture.expired(), "redraw retained the old button callback");
            require(root.button()->ui == object, "button redraw remounted native object");
            lv_obj_send_event(object, LV_EVENT_CLICKED, nullptr);
            require(root.clicked == 7 && clicks == 1, "redraw lost current or external callback");
            root.action = [&root]() { root.value = 8; root.redraw(); };
            lv_obj_send_event(object, LV_EVENT_CLICKED, nullptr);
            require(root.clicked == 7 && clicks == 2, "active callback changed during redraw");
            root.action = {};
            lv_obj_send_event(object, LV_EVENT_CLICKED, nullptr);
            require(root.clicked == 8 && clicks == 3, "next click used a retired callback");
            root.action = []() { throw std::runtime_error("expected click failure"); };
            lv_obj_send_event(object, LV_EVENT_CLICKED, nullptr);
            require(clicks == 4, "click exception escaped the native boundary");
            root.action = {};
            oldCapture = root.capture;
            root.fail = true;
            try { root.redraw(); require(false, "builder did not fail"); }
            catch(const std::runtime_error & error) { require(std::string(error.what()) == "builder failure", "wrong builder failure"); }
            require(root.button()->ui == object && !oldCapture.expired(), "builder failure discarded mounted tree");
            root.fail = false;
            root.redraw();
            require(oldCapture.expired(), "recovery retained previous callback");
            root.action = [&root]() { root.redraw(); root.show = false; root.redraw(); };
            oldCapture = root.capture;
            lv_obj_send_event(object, LV_EVENT_CLICKED, nullptr);
            require(!root.button() && oldCapture.expired() && deletes == 1,
                    "callback self-removal retained declarations/captures/native objects");
            root.action = {};
            root.show = true;
            root.redraw();
            // Secondary native handles must also observe destruction.
            lv_obj_delete(root.button()->label);
            require(root.button()->label == nullptr, "deleted native label remained accessible");
            root.redraw();
            require(root.button()->label && std::string(lv_label_get_text(root.button()->label)) == "8",
                    "retained button did not restore its label");
            auto column = find(root, "column");
            lv_obj_delete(static_cast<lv_obj_t *>(column->ui));
            require(!column->ui && !root.button()->ui && !root.button()->label,
                    "parent deletion did not invalidate every native observer");
            root.show = false;
            // Safe destruction after external deletion; no remount contract implied.
        }
        require(lv_obj_get_child_count(lv_screen_active()) == 0, "standalone view retained native content");
        {
            ClickRoot root;
            root.redraw();
            auto display = lv_display_get_default();
            lv_display_delete(display);
            require(!root.children->front()->ui && !root.button()->ui && !root.button()->label,
                    "display deletion left dangling native observers");
        }
        lv_display_create(800, 600);
        for(int i = 0; i != 10; ++i) {
            ClickRoot root;
            root.redraw();
            auto capture = root.capture;
            root.show = false;
            root.redraw();
            require(capture.expired(), "removal retained captures");
            root.show = true;
            root.redraw();
        }
        require(lv_obj_get_child_count(lv_screen_active()) == 0, "repeated views retained native objects");
    }

    struct Model {
        CPPUI::State<int> left{1}, right{1}, nested{1};
        CPPUI::State<bool> show{true};
        bool reversed = false;
    };
    class Labels: public CPPUI::LVGL::View {
        CPPUI::State<int> & count;
        std::string prefix;
        public:
        Labels(CPPUI::State<int> & count, const std::string & prefix): count(count), prefix(prefix) { ++alive; }
        ~Labels() override { --alive; }
        void content() override {
            for(int i = 0; i < count; ++i) new Label(prefix + std::to_string(i));
        }
    };
    class Middle: public CPPUI::LVGL::View {
        Model & model;
        public:
        explicit Middle(Model & model): model(model) { ++alive; }
        ~Middle() override { --alive; }
        void content() override { new Label("middle"); (new Labels(model.nested, "nested"))->id("nested"); }
    };
    class VirtualRoot: public CPPUI::LVGL::View {
        Model & model;
        void left() { (new Labels(model.left, "left"))->id("left"); }
        void right() { (new Labels(model.right, "right"))->id("right"); }
        public:
        explicit VirtualRoot(Model & model): model(model) {}
        void content() override {
            for(auto window: CPPUI::Window()) for(auto column: CPPUI::VBox().id("column")) {
                if(model.reversed) right(); else left();
                if(model.show) (new Middle(model))->id("middle");
                if(model.reversed) left(); else right();
                new Label("tail");
            }
        }
        lv_obj_t * column() { return static_cast<lv_obj_t *>(find(*this, "column")->ui); }
        void check(std::initializer_list<const char *> expected) {
            require(lv_obj_get_child_count(column()) == expected.size(), "wrong native child count");
            int index = 0;
            for(auto text: expected) require(std::string(lv_label_get_text(lv_obj_get_child(column(), index++))) == text,
                                             "wrong virtual child order");
        }
    };
    void virtualViews() {
        Model model;
        {
            VirtualRoot root(model);
            root.redraw();
            root.check({"left0", "middle", "nested0", "right0", "tail"});
            auto left = lv_obj_get_child(root.column(), 0);
            model.left = 2;
            model.nested = 2;
            root.check({"left0", "left1", "middle", "nested0", "nested1", "right0", "tail"});
            root.redraw();
            for(int i = 0; i != 10; ++i) {
                model.reversed = true;
                root.redraw();
                root.check({"right0", "middle", "nested0", "nested1", "left0", "left1", "tail"});
                require(lv_obj_get_child(root.column(), 4) == left, "keyed view move replaced native identity");
                model.reversed = false;
                root.redraw();
                model.show = false;
                root.check({"left0", "left1", "right0", "tail"});
                model.show = true;
            }
            model.left = 0;
            model.right = 0;
            model.nested = 0;
            root.check({"middle", "tail"});
            model.show = false;
            root.check({"tail"});
            model.left = 1;
            root.check({"left0", "tail"});
        }
        require(alive == 0 && lv_obj_get_child_count(lv_screen_active()) == 0,
                "standalone nested view teardown leaked declarations or native children");
        model.left = 3; // Removed views must no longer subscribe.
    }
}
int main(int argc, char ** argv) {
    lv_init();
    lv_display_create(800, 600);
    int result = 0;
    try {
        {
            Root root(argc, argv);
            root.redraw();
            auto native = static_cast<lv_obj_t *>(root.children->front()->children->front()->outer());
            for(int i = 1; i <= 20; ++i) {
                root.value = i;
                root.redraw();
                require(validTransfer, "backend update omitted the common retirement transfer");
                require(alive == 1 && retired == i, "retired declarations were retained or deleted twice");
                require(root.children->front()->children->front()->outer() == native
                    && std::string(lv_label_get_text(native)) == std::to_string(i),
                    "retirement did not preserve and update the native label");
            }
            root.show = false;
            root.redraw();
            require(alive == 0 && lv_obj_get_child_count(lv_screen_active()) == 0,
                    "removal retained a declaration or its native object");
            root.show = true;
            root.redraw();
        }
        require(alive == 0 && lv_obj_get_child_count(lv_screen_active()) == 0,
                "application teardown retained declarations or native content");
        callbacksAndDeletion();
        virtualViews();
        applicationClose(argc, argv);
    } catch(const std::exception & error) {
        std::cerr << error.what() << '\n';
        result = 1;
    }
    lv_deinit();
    return result;
}

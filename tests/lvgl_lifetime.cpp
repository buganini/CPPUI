#include "cppui/lvgl/application.hpp"
#include "cppui/lvgl/window.hpp"
#include "cppui/lvgl/label.hpp"
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
    } catch(const std::exception & error) {
        std::cerr << error.what() << '\n';
        result = 1;
    }
    lv_deinit();
    return result;
}

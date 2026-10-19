#ifndef CPPUI_EXAMPLE_VIRTUAL_NODES
#define CPPUI_EXAMPLE_VIRTUAL_NODES
#include "cppui/backend.hpp"

namespace Examples { namespace VirtualNodes {
    struct Model {
        CPPUI::State<int> count1{1};
        CPPUI::State<bool> show2{true};
        CPPUI::State<int> count2{1};
        CPPUI::State<int> count2sub{1};
        CPPUI::State<int> count3{1};
    };
    inline Model & model() {
        static Model value;
        return value;
    }

    class View1: public CPPUI::Backend::View {
        void content() override {
            for(int i = 0; i < model().count1; ++i) CPPUI::Label("View1-" + std::to_string(i));
        }
    };
    class View2Sub: public CPPUI::Backend::View {
        void content() override {
            for(auto row: CPPUI::HBox()) {
                CPPUI::Label("View2Sub n=" + std::to_string(model().count2sub));
                CPPUI::Button("-").click([]() { model().count2sub -= 1; });
                CPPUI::Button("+").click([]() { model().count2sub += 1; });
            }
            for(int i = 0; i < model().count2sub; ++i) CPPUI::Label("View2Sub-" + std::to_string(i));
        }
    };
    class View2: public CPPUI::Backend::View {
        void content() override {
            for(int i = 0; i < model().count2; ++i) CPPUI::Label("View2-" + std::to_string(i));
            CPPUI::SubView<View2Sub>();
        }
    };
    class View3: public CPPUI::Backend::View {
        void content() override {
            for(int i = 0; i < model().count3; ++i) CPPUI::Label("View3-" + std::to_string(i));
        }
    };
    class App: public CPPUI::Application {
        public:
        using Application::Application;
        void content() override {
            for(auto window: CPPUI::Window().title("CPPUI virtual nodes")) {
                for(auto column: CPPUI::VBox()) {
                    for(auto controls: CPPUI::HBox()) {
                        CPPUI::Label("View1 n=" + std::to_string(model().count1));
                        CPPUI::Button("-").click([]() { model().count1 -= 1; });
                        CPPUI::Button("+").click([]() { model().count1 += 1; });
                    }
                    CPPUI::SubView<View1>();
                    for(auto controls: CPPUI::HBox()) {
                        CPPUI::Label(std::string("View2 show=") + (model().show2 ? "true" : "false"));
                        CPPUI::Button("Hide").click([]() { model().show2 = false; });
                        CPPUI::Button("Show").click([]() { model().show2 = true; });
                    }
                    if(model().show2) {
                        for(auto controls: CPPUI::HBox()) {
                            CPPUI::Label("View2 n=" + std::to_string(model().count2));
                            CPPUI::Button("-").click([]() { model().count2 -= 1; });
                            CPPUI::Button("+").click([]() { model().count2 += 1; });
                        }
                        CPPUI::SubView<View2>();
                    }
                    for(auto controls: CPPUI::HBox()) {
                        CPPUI::Label("View3 n=" + std::to_string(model().count3));
                        CPPUI::Button("-").click([]() { model().count3 -= 1; });
                        CPPUI::Button("+").click([]() { model().count3 += 1; });
                    }
                    CPPUI::SubView<View3>();
                }
            }
        }
    };
}}
#endif

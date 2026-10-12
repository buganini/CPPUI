#include <string>

#include "../../cppui/state.hpp"
#include "../../cppui/qt/application.hpp"
#include "../../cppui/qt/window.hpp"
#include "../../cppui/qt/layout.hpp"
#include "../../cppui/qt/label.hpp"
#include "../../cppui/qt/button.hpp"

using namespace CPPUI;

State<int> count1(1);
State<bool> show2(true);
State<int> count2(1);
State<int> count2sub(1);
State<int> count3(1);

class View1: public CPPUI::Qt::QtView {
    public:
    void content(void) {
        for(int i = 0; i < count1; i += 1) {
            Label("View1-" + std::to_string(i));
        }
    }
};

class View2Sub: public CPPUI::Qt::QtView {
    public:
    void content(void) {
        for(auto scope: HBox()) {
            Label("View2Sub n=" + std::to_string(count2sub));
            Button("-").click([]() { count2sub -= 1; });
            Button("+").click([]() { count2sub += 1; });
        }
        for(int i = 0; i < count2sub; i += 1) {
            Label("View2Sub-" + std::to_string(i));
        }
    }
};

class View2: public CPPUI::Qt::QtView {
    public:
    void content(void) {
        for(int i = 0; i < count2; i += 1) {
            Label("View2-" + std::to_string(i));
        }
        SubView<View2Sub>();
    }
};

class View3: public CPPUI::Qt::QtView {
    public:
    void content(void) {
        for(int i = 0; i < count3; i += 1) {
            Label("View3-" + std::to_string(i));
        }
    }
};

class App: public Application {
    public:
    App(int argc, char * argv[]): Application(argc, argv) {}

    void content(void) {
        for(auto window: Window()) {
            for(auto column: VBox()) {
                for(auto controls: HBox()) {
                    Label("View1 n=" + std::to_string(count1));
                    Button("-").click([]() { count1 -= 1; });
                    Button("+").click([]() { count1 += 1; });
                }
                SubView<View1>();

                for(auto controls: HBox()) {
                    Label(std::string("View2 show=") + (show2 ? "true" : "false"));
                    Button("Hide").click([]() { show2 = false; });
                    Button("Show").click([]() { show2 = true; });
                }

                if(show2) {
                    for(auto controls: HBox()) {
                        Label("View2 n=" + std::to_string(count2));
                        Button("-").click([]() { count2 -= 1; });
                        Button("+").click([]() { count2 += 1; });
                    }
                    SubView<View2>();
                }

                for(auto controls: HBox()) {
                    Label("View3 n=" + std::to_string(count3));
                    Button("-").click([]() { count3 -= 1; });
                    Button("+").click([]() { count3 += 1; });
                }
                SubView<View3>();
            }
        }
    }
};

int main(int argc, char * argv[]) {
    App app(argc, argv);
    app.run();
    return 0;
}

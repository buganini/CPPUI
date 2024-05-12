#include <iostream>
#include "../../cppui/qt/application.hpp"
#include "../../cppui/qt/window.hpp"
#include "../../cppui/qt/label.hpp"

using namespace CPPUI;
class UI: public Application {
    public:
    UI(int argc, char *argv[]): Application(argc, argv) {

    }

    void content(void) {
        for(auto _: Window().tag("window1")) {
            Label("label1").tag("label1");
            Label("label2").tag("label2");
            Label("label3");
        }
        std::cout << *this << std::endl;
    }
};

int main(int argc, char *argv[]) {
    UI app(argc, argv);
    app.tag("app").run();
}
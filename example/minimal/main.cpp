#include <iostream>
#include "../../cppui/qt/application.hpp"
#include "../../cppui/qt/window.hpp"

using namespace CPPUI;
class UI: public Application {
    public:
    UI(int argc, char *argv[]): Application(argc, argv) {

    }

    void content(void) {
        for(auto _: Window().tag("window1")) {

        }
        printUI(0);
    }
};

int main(int argc, char *argv[]) {
    UI app(argc, argv);
    app.tag("app").run();
}
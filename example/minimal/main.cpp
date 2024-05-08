#include <iostream>
#include "../../cppui/qt/application.hpp"
#include "../../cppui/qt/window.hpp"

using namespace CPPUI;
class UI: public Application {
    public:
    UI(int argc, char *argv[]): Application(argc, argv) {

    }

    void content(void) {
        std::cout << "application\n";
        for(auto _: Window()) {
            std::cout << "window\n";
        }
    }
};

int main(int argc, char *argv[]) {
    UI app(argc, argv);
    app.run();
}
#include <iostream>
#include "../qt_smoke.hpp"
#include "../../cppui/state.hpp"
#include "../../cppui/qt/application.hpp"
#include "../../cppui/qt/window.hpp"
#include "../../cppui/qt/layout.hpp"
#include "../../cppui/qt/label.hpp"
#include "../../cppui/qt/button.hpp"

using namespace CPPUI;
class UI: public Application {
    public:
    UI(int argc, char *argv[]): Application(argc, argv) {

    }

    State<int> n{1};

    void content(void) {
        for(auto _: Window().id("window1")) {
            for(auto _: VBox()) {
                for(auto _: HBox()) {
                    Button("-").click([this](){
                        n -= 1;
                    });

                    Label(std::to_string(n));

                    Button("+").click([this](){
                        n += 1;
                    });
                }
                for(auto _: HBox()) {
                    for(int i=0;i<n;i+=1){
                        Label(std::to_string(i));
                    }
                }
            }
        }

        // print UI hierarchy
        std::cout << *this << std::endl;
    }
};

int main(int argc, char *argv[]) {
    try {
        UI app(argc, argv);
        if(QtSmoke::requested(argc, argv)) {
            app.redraw();
            for(int i = 0; i < 3; ++i) {
                QtSmoke::require(QtSmoke::click(app, "+"), "increment button missing");
                QtSmoke::settle();
            }
            QtSmoke::require(app.n == 4 && QtSmoke::hasText(app, "4"), "counter did not increment");
            for(int i = 0; i < 5; ++i) {
                QtSmoke::require(QtSmoke::click(app, "-"), "decrement button missing");
                QtSmoke::settle();
            }
            QtSmoke::require(app.n == -1 && QtSmoke::hasText(app, "-1"), "empty counter range did not redraw");
            return 0;
        }
        app.run();
    } catch(const std::exception & error) {
        std::cerr << error.what() << std::endl;
        return 1;
    }
}

#include <iostream>
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

    State<int> n = State<int>(1);

    void content(void) {
        for(auto _: Window().tag("window1")) {
            for(auto _: VBox()) {
                for(auto _: HBox()) {
                    Button("-").click([=](){
                        n -= 1;
                    });
                    Label(std::to_string(n));
                    Button("+").click([=](){
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
    UI app(argc, argv);
    app.tag("app").run();
}
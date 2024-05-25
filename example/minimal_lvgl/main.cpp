#include <iostream>
#include "../../cppui/state.hpp"
#include "../../cppui/lvgl/application.hpp"
#include "../../cppui/lvgl/window.hpp"
#include "../../cppui/lvgl/label.hpp"
#include "../../cppui/lvgl/button.hpp"
#include "../../cppui/lvgl/layout.hpp"
#include "unistd.h"

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
    app.run();
}
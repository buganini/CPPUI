#include <iostream>
#include "../../cppui/state.hpp"
#include "../../cppui/wx/application.hpp"
#include "../../cppui/wx/window.hpp"
#include "../../cppui/wx/label.hpp"
#include "../../cppui/wx/button.hpp"
#include "../../cppui/wx/layout.hpp"
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
# Warning!
Pre-alpha status, not ready for use

# What is CPPUI
CPPUI is a declarative UI framework with two-way data binding.
CPPUI doesn't do UI itself, it turns imperative UI libraries into declarative flavor with virtual DOM and aims to maintain interoperability.

[Slides for SciWork 2023](https://speakerdeck.com/buganini/pui-declarative-ui-framework-for-python)

[PUI: Python Version](https://github.com/buganini/PUI)

# View Hierarchy Construction
```c++
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
        for(auto _: Window().id("window1")) {
            for(auto _: HBox()) {
                Label("label1").id("label1");
                Label("label2").id("label2");
                Label("label3");
            }
        }
        std::cout << *this << std::endl;
    }
};

int main(int argc, char *argv[]) {
    UI app(argc, argv);
    app.id("app").run();
}
```

Output | c++filt -t:
```
UI#app {
  CPPUI::Qt::Window#window1 {
    CPPUI::Qt::HBox {
      CPPUI::Qt::Label#label1 {
      }
      CPPUI::Qt::Label#label2 {
      }
      CPPUI::Qt::Label {
      }
    }
  }
}
```

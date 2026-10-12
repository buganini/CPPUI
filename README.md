# Warning!
Pre-alpha status, not ready for use

# What is CPPUI
CPPUI is a declarative UI framework with two-way data binding.
CPPUI doesn't do UI itself, it turns imperative UI libraries into declarative flavor with virtual DOM and aims to maintain interoperability.

[Slides for SciWork 2023](https://speakerdeck.com/buganini/pui-declarative-ui-framework-for-python)

[PUI: Python Version](https://github.com/buganini/PUI)

# Build and test the core

CPPUI is header-only. Link the CMake interface target `CPPUI::cppui`, or add this
repository to your include path. Core headers support C++11 and can be included
from multiple source files. The core checks do not require a UI toolkit:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

The build also checks that each core header compiles independently.

With Qt 6 development packages available, enable the Qt target, both existing
examples, and automated offscreen regressions:

```sh
cmake -S . -B build-qt -DCPPUI_BUILD_QT=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-qt
ctest --test-dir build-qt --output-on-failure
```

Qt applications can link `CPPUI::qt`; Qt's imported target supplies its compiler
requirements. CTest sets the offscreen platform for the Qt test automatically.
The interactive examples are `cppui_minimal_qt` and `cppui_virtual_nodes` in the
build directory. Both accept `--smoke` and run as CTest regressions, including
Valgrind variants when enabled. Their original CMake projects under `example/`
remain usable.

# Shared cookbook

The cookbook builds the same C++ recipe and gallery sources with separate Qt
and LVGL CMake projects. It keeps PUI's scrollable Example sidebar and Result
pane, plus the Qt menus; only the source-code pane is omitted. `--list` reports
available recipes and prerequisites for pages that are not ported yet.

With Qt 6 development packages installed:

```sh
cmake -S cookbook/qt -B build-cookbook-qt -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-cookbook-qt
./build-cookbook-qt/cppui_cookbook_qt
ctest --test-dir build-cookbook-qt --output-on-failure
```

The root `CPPUI_BUILD_QT=ON` build also includes `cppui_cookbook_qt`. Qt smoke tests
use the offscreen platform automatically.

For LVGL, provide an LVGL 9 source checkout (tested with
[LVGL 9.3.0](https://github.com/lvgl/lvgl/tree/c033a98afddd65aaafeebea625382a94020fe4a7)):

```sh
cmake -S cookbook/lvgl -B build-cookbook-lvgl \
  -DCPPUI_LVGL_SOURCE_DIR=/path/to/lvgl -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-cookbook-lvgl
ctest --test-dir build-cookbook-lvgl --output-on-failure
./build-cookbook-lvgl/cppui_cookbook_lvgl --smoke
```

The default LVGL build uses a headless display. With SDL2 development packages
installed, enable an interactive window:

```sh
cmake -S cookbook/lvgl -B build-cookbook-lvgl -DCPPUI_LVGL_SDL=ON
cmake --build build-cookbook-lvgl
./build-cookbook-lvgl/cppui_cookbook_lvgl
ctest --test-dir build-cookbook-lvgl --output-on-failure
```

The second configure command reuses the SDK path from the first. SDL builds also
test the SDL renderer with the dummy video driver (`--smoke --sdl`), including
device teardown, without needing a desktop display. Both projects
accept `-DCPPUI_VALGRIND_TESTS=ON` to register memory tests. See the
[cookbook documentation](cookbook/README.md) for capabilities and validation limits.

# Memory checks

With Valgrind installed, use a separate build without sanitizer flags:

```sh
cmake -S . -B build-valgrind -DCPPUI_BUILD_QT=ON \
  -DCPPUI_VALGRIND_TESTS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-valgrind
ctest --test-dir build-valgrind -L valgrind --output-on-failure
```

Every automated regression also runs under Memcheck when enabled. Invalid memory
accesses and definite, indirect, or possible leaks fail the test. Reachable
process-global allocations are reported but do not fail it. Qt tests use the
offscreen platform. No project suppression file is applied. To pass additional
platform-specific arguments, use `CPPUI_VALGRIND_OPTIONS` (a CMake list).

# View Hierarchy Construction
```c++
#include <iostream>
#include "cppui/qt/application.hpp"
#include "cppui/qt/window.hpp"
#include "cppui/qt/label.hpp"
#include "cppui/qt/layout.hpp"

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

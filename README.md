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

With Qt 6 Widgets and Test development packages available, enable the Qt target,
both existing examples, and automated offscreen regressions:

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

For LVGL, first obtain the [v9.6.0 sources](https://github.com/lvgl/lvgl/tree/80ca777e37a2b176770726a02e07a6fb79ef0b39):

```sh
git clone --depth 1 --branch v9.6.0 https://github.com/lvgl/lvgl.git lvgl-9.6.0
```

With SDL2 development packages installed, build and run the SDL2 cookbook:

```sh
cmake -S cookbook/lvgl_sdl2 -B build-cookbook-lvgl_sdl2 \
  -DCPPUI_LVGL_SOURCE_DIR=lvgl-9.6.0 -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-cookbook-lvgl_sdl2
./build-cookbook-lvgl_sdl2/cppui_cookbook_lvgl_sdl2
ctest --test-dir build-cookbook-lvgl_sdl2 --output-on-failure
```

With X11 development packages installed and an X display available, build and
run the X11 cookbook:

```sh
cmake -S cookbook/lvgl_x11 -B build-cookbook-lvgl_x11 \
  -DCPPUI_LVGL_SOURCE_DIR=lvgl-9.6.0 -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-cookbook-lvgl_x11
./build-cookbook-lvgl_x11/cppui_cookbook_lvgl_x11
ctest --test-dir build-cookbook-lvgl_x11 --output-on-failure
```

Both LVGL executables use their selected driver for interactive runs and
`--smoke`. CTest runs SDL2 with its dummy video/software-renderer drivers;
X11 tests use `DISPLAY` (a desktop X server or Xvfb). All three cookbook projects
accept `-DCPPUI_VALGRIND_TESTS=ON` to register memory tests. The X11 build applies
a display-thread cleanup fix to a build copy of the v9.6.0 driver. See the
[cookbook documentation](cookbook/README.md) for capabilities and validation limits.
Remaining implementation work is tracked in the [Qt plan](doc/pyside6-progress.md)
and [LVGL plan](doc/lvgl-progress.md).

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
offscreen platform. Valgrind runs set `QT_ENABLE_REGEXP_JIT=0` to avoid Qt regexp
JIT diagnostics without function names, as recommended by the
[Valgrind FAQ](https://valgrind.org/docs/manual/faq.html). Normal runs retain Qt's
default JIT setting. No project suppression file is applied. To pass additional
platform-specific arguments, use `CPPUI_VALGRIND_OPTIONS` (a CMake list).

PUI's `.layout(...)`, `.style(...)` and `.qt(...)` modifiers are available on
CPPUI nodes. For example, `Label("Title").layout(200, 40).style(0x336699, {}, 20)`
sets a preferred Qt size and text style. See the [modifier reference](REFERENCE.md#pui-modifiers)
for argument order, typed Qt options, redraw resets and LVGL mappings.

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

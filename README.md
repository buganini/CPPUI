# What is CPPUI
CPPUI is a declarative UI framework with two-way data binding.
CPPUI doesn't do UI itself, it turns imperative UI libraries into declarative flavor with virtual DOM and aims to maintain interoperability.

Backends are available for Qt, wxWidgets and LVGL. The project is under active
development, and APIs may evolve. See the [component reference](REFERENCE.md)
and [cookbook](cookbook/README.md) for supported features and examples in your
chosen backend.

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
cmake --build build-qt -j "$(nproc)"
ctest --test-dir build-qt --output-on-failure
```

Qt applications can link `CPPUI::qt`; Qt's imported target supplies its compiler
requirements. CTest sets the offscreen platform for the Qt test automatically.
The interactive examples are `cppui_minimal_qt` and `cppui_virtual_nodes` in the
build directory. Their separate `_smoke` executables run as CTest regressions,
including Valgrind variants when enabled. The [shared example sources](example/README.md)
also build with wx and LVGL; the standalone CMake projects under `example/`
remain usable.

[Views](REFERENCE.md#views) on Qt, wx and LVGL supports internal selection or a
`State<OptionalText>` binding. Pages retain their controls and subview state
across selection changes; the shared cookbook demonstrates both forms.
Qt [Tabs](REFERENCE.md#tabs) also supports these selection models.

[MaskedField](REFERENCE.md#maskedfield) provides password input on all three
backends. All three include the HeavyCanvas cookbook with up to 1,000,000 synthetic
sessions, hover inspection and wheel zoom. Qt/wx also support decorated Table cells.

With wxWidgets 3.2 development packages and a display available:

```sh
cmake -S . -B build-wx -DCPPUI_BUILD_WX=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-wx
ctest --test-dir build-wx --output-on-failure
./build-wx/cppui_minimal_wx
```

The wx examples are `cppui_minimal_wx` and `cppui_virtual_nodes_wx`, each with
a separate `_smoke` test executable. wx applications can link `CPPUI::wx`. On Linux, install `libwxgtk3.2-dev`;
on macOS, `brew install wxwidgets`. Native wx tests need an X/Wayland display
(or `xvfb-run` on Linux); they are not headless widgets. Add
`-DCPPUI_VALGRIND_TESTS=ON` in a non-sanitized build for memory checks. Linux wx
checks suppress only toolkit font/theme/image-loader/TLS and standalone-radio
metadata allocations reproduced by the plain native `wx.native_baseline` test;
declaration and wx object allocations remain checked.
Remaining work is tracked in the [wx plan](doc/wx-progress.md).

# Shared cookbook

The cookbook builds the same C++ recipe and gallery sources with separate Qt, wxWidgets
and LVGL CMake projects. It keeps PUI's scrollable Example sidebar and Result
pane, plus the Qt/wx menus; only the source-code pane is omitted. `--list` reports
available recipes and prerequisites for pages that are not ported yet.

From the repository root, choose a backend and build/run its cookbook:

```sh
make qt          # Qt 6 development packages
make wx          # wxWidgets 3.2 development packages
make lvgl_sdl2   # SDL2 development packages
make lvgl_x11    # X11 development packages and an available X display
```

Each target builds with `-j $(nproc)` and launches the cookbook after a
successful build. The default build type is `RelWithDebInfo`; override settings
with, for example, `make qt BUILD_TYPE=Debug JOBS=4`.

LVGL targets automatically clone v9.6.0 into `lvgl-9.6.0` if the sources are
missing and reuse an existing checkout. Use `make lvgl-checkout` to obtain only
the sources, or set `CPPUI_LVGL_SOURCE_DIR=/path/to/lvgl` for another location.
Pass extra configure options with `CMAKE_ARGS='-DOPTION=value'`.

See the [cookbook documentation](cookbook/README.md) for dependencies,
capabilities and validation limits. Remaining implementation work is tracked in
the [Qt plan](doc/pyside6-progress.md), [wx plan](doc/wx-progress.md) and
[LVGL plan](doc/lvgl-progress.md).

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

# Layout and styling

Start with the sizing supplied by each component and its parent layout. CPPUI
uses the selected backend's native size hints, layout rules and component
defaults. For example, Qt and wx TextField controls expand horizontally while
keeping their native height, and Spacer takes flexible space in an HBox or VBox.
Exact dimensions depend on the toolkit, platform, font and theme.

Use `.layout(...)` when a design needs explicit dimensions, a particular share
of flexible space, or custom padding. Width and height map to preferred sizes,
minimum sizes or object dimensions depending on the backend; the
[modifier reference](REFERENCE.md#pui-modifiers) describes these mappings.

Use typed named arguments for `.layout(...)` and `.style(...)`. Supply only the
settings you need, in any order:

```cpp
namespace layout = CPPUI::layout;
namespace style = CPPUI::style;

CPPUI::Label("Title")
    .layout(layout::width(200), layout::height(40), layout::padding(8))
    .style(style::fontSize(20), style::color(0x336699))
    .wideLayout(600, layout::width(320))
    .wideStyle(600, style::fontSize(24));
```

`wideLayout` and `wideStyle` apply their settings when the nearest non-virtual
parent's content width reaches the first argument. Resizing below that threshold
restores the base declaration's settings.

`style::fontFamily(const char*)` borrows its string; use a string literal or keep
the caller's buffer alive for the node declaration's lifetime. `nullptr` means
omitted. Positional overloads remain available. See the
[modifier reference](REFERENCE.md#pui-modifiers) for all named helpers, borrowed
string rules, typed `.qt(...)` options, redraw resets and backend mappings.

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

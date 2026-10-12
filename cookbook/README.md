# Shared C++ cookbook

The main cookbook UI in `cookbook.cpp` and the recipes are shared by Qt and LVGL. Each backend
has its own `CMakeLists.txt`, include selection, startup, and native smoke-test
helpers. Recipe code has no backend conditionals. Each executable links exactly
one toolkit. Qt corresponds to the Python reference's PySide6 backend.

Reference: [PUI cookbook at 009e9b9](https://github.com/buganini/PUI/tree/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook).

Button, Label, HBox, and VBox share their implementations across both backends.
Qt also enables shared ProgressBar, Text, and Splitter recipes through its source manifest.
The gallery preserves the reference's scrollable **Example** sidebar and expanding
**Result** pane. Qt also keeps the nested example menus and initially maximized
window. Only the source-code pane is omitted. Recipe spacing, weighted columns,
divider, and styled examples follow the reference on the available pages.

Unsupported pages show “Not Supported for this backend”; `--list` gives their
prerequisites. The [alignment plan](../doc/pyside6-progress.md) tracks remaining
components and public APIs. The backend `presentation.hpp` files provide private
cookbook layout/appearance adapters until those public APIs are decided; these
helpers do not imply general Scroll, menu, style, or layout API support in CPPUI.

## Qt

```sh
cmake -S cookbook/qt -B build-cookbook-qt -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-cookbook-qt
./build-cookbook-qt/cppui_cookbook_qt
ctest --test-dir build-cookbook-qt --output-on-failure
```

The repository's root `CPPUI_BUILD_QT=ON` build also includes this executable and
its smoke test. CTest selects Qt's offscreen platform automatically.

## LVGL

Use an LVGL 9 source checkout. Validated with
[v9.3.0](https://github.com/lvgl/lvgl/tree/c033a98afddd65aaafeebea625382a94020fe4a7).
Dependencies are supplied explicitly; the build does not fetch them.

```sh
cmake -S cookbook/lvgl -B build-cookbook-lvgl \
  -DCPPUI_LVGL_SOURCE_DIR=/path/to/lvgl -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-cookbook-lvgl
ctest --test-dir build-cookbook-lvgl --output-on-failure
```

This builds a headless software display for smoke testing. Add
`-DCPPUI_LVGL_SDL=ON` with SDL2 development files installed for an interactive
window, then run `cppui_cookbook_lvgl` without arguments. Ordinary `--smoke` stays
headless. SDL builds additionally register `cookbook.lvgl_sdl`, which runs
`--smoke --sdl` with SDL's dummy video and software-renderer drivers. This checks
the SDL display, input-device creation, recipes and cleanup without a desktop;
use the same flags without driver overrides to view the smoke run in a window.
The configuration uses libc allocation so memory
checkers can observe individual LVGL allocations. Application teardown preserves
the display-owned active screen and releases its CPPUI content before `lv_deinit`.
The example assumes CPPUI owns the content of that screen. In SDL builds, startup
owns the input devices and display; teardown releases those before quitting SDL
and deinitializing LVGL, avoiding LVGL 9.3's texture cleanup after SDL shutdown.

## Tests and memory checks

`--smoke` clicks every sidebar page three times, checks the two-column geometry,
scrollable list, Qt menus, native label contents, and A/B and progress buttons,
then tears down the application. Sidebar native identity survives page changes.
Both CMake projects support
`-DCPPUI_VALGRIND_TESTS=ON`; `ctest -L valgrind --output-on-failure` runs the same
scenarios with Memcheck. Use a separate build for ASan/UBSan and Valgrind.
The LVGL project also tests declaration retirement, native reuse, removal, and
application teardown through `lvgl.lifetime` and its optional Valgrind variant.

Interactive SDL input and real-display layout/focus checks remain outstanding.
Interactive close also needs an application-stop contract: LVGL's SDL handler
can destroy the display or exit before CPPUI tears down declarations. The tested
smoke path returns to startup for ordered cleanup; it does not validate native
window-close handling. See the alignment plan for that decision.

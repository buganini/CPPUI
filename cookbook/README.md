# Shared C++ cookbook

The recipes and gallery in this directory are shared by Qt and LVGL. Each backend
has its own `CMakeLists.txt`, include selection, startup, and native smoke-test
helpers. Recipe code has no backend conditionals. Each executable links exactly
one toolkit. Qt corresponds to the Python reference's PySide6 backend.

Reference: [PUI cookbook at 009e9b9](https://github.com/buganini/PUI/tree/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook).

Button, Label, HBox, and VBox share their implementations across both backends.
Qt also enables shared ProgressBar and Text recipes through its source manifest. These
pages remain partial until their displayed missing details are implemented. The gallery and `--list` report missing details and unavailable pages.
Full styles, layout options, bindings, source presentation, and remaining component
pages follow the [alignment plan](../doc/pyside6-progress.md). The navigation
buttons cycle through the same page registry on either backend.

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
window, then run `cppui_cookbook_lvgl` without arguments. The smoke test stays
headless even in an SDL build. The configuration uses libc allocation so memory
checkers can observe individual LVGL allocations. Application teardown preserves
the display-owned active screen and releases its CPPUI content before `lv_deinit`.
The example assumes CPPUI owns the content of that screen.

## Tests and memory checks

`--smoke` visits every page three times, checks native label contents, exercises
A/B buttons and gallery navigation, and tears down the application. Unsupported
pages display their prerequisites. Both CMake projects support
`-DCPPUI_VALGRIND_TESTS=ON`; `ctest -L valgrind --output-on-failure` runs the same
scenarios with Memcheck. Use a separate build for ASan/UBSan and Valgrind.

Interactive SDL input and real-display layout/focus checks remain outstanding.

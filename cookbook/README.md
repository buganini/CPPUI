# Shared C++ cookbook

The main cookbook UI in `cookbook.cpp` and the recipes are shared by Qt and LVGL. Each backend
has its own include selection, startup, and native smoke-test helpers. Separate
`qt`, `lvgl_sdl2`, and `lvgl_x11` CMake projects select the toolkit and window
driver. Recipe code has no backend conditionals. Each executable links exactly
one toolkit. Qt corresponds to the Python reference's PySide6 backend.

Reference: [PUI cookbook at 009e9b9](https://github.com/buganini/PUI/tree/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook).

Button, Label, HBox, VBox, and Canvas share their recipe implementations across both backends.
Qt also enables shared ProgressBar, Text, Splitter, Tab, Modal and Dialog recipes through
its source manifest.
The LVGL Canvas page keeps the 100x100 gray drawing area and rotation button.
Its basic painter uses LVGL software drawing and scaled bitmap text; image
resources and input callbacks remain pending. See the [Canvas reference](../REFERENCE.md#canvas)
for font/color differences and callback ownership.
The gallery preserves the reference's scrollable **Example** sidebar and expanding
**Result** pane. Qt also keeps the nested example menus and initially maximized
window. Only the source-code pane is omitted. Recipe spacing, weighted columns,
divider, and styled examples follow the reference on the available pages.

Unsupported pages show “Not Supported for this backend”; `--list` gives their
prerequisites. The [Qt plan](../doc/pyside6-progress.md) and
[LVGL plan](../doc/lvgl-progress.md) track remaining components and public APIs.
The backend `presentation.hpp` files provide private
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

Use an [LVGL 9.6.0 source checkout](https://github.com/lvgl/lvgl/tree/80ca777e37a2b176770726a02e07a6fb79ef0b39).
Dependencies are supplied explicitly; the build does not fetch them.

```sh
git clone --depth 1 --branch v9.6.0 https://github.com/lvgl/lvgl.git lvgl-9.6.0
```

### SDL2

Install SDL2 development packages, then run:

```sh
cmake -S cookbook/lvgl_sdl2 -B build-cookbook-lvgl_sdl2 \
  -DCPPUI_LVGL_SOURCE_DIR=lvgl-9.6.0 -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-cookbook-lvgl_sdl2
./build-cookbook-lvgl_sdl2/cppui_cookbook_lvgl_sdl2
ctest --test-dir build-cookbook-lvgl_sdl2 --output-on-failure
```

CTest uses SDL's dummy video and software-renderer drivers. To watch the same
smoke scenario in a window, run `cppui_cookbook_lvgl_sdl2 --smoke` from its build
directory without those driver overrides.

### X11

Install X11 development packages and use an available X display:

```sh
cmake -S cookbook/lvgl_x11 -B build-cookbook-lvgl_x11 \
  -DCPPUI_LVGL_SOURCE_DIR=lvgl-9.6.0 -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-cookbook-lvgl_x11
./build-cookbook-lvgl_x11/cppui_cookbook_lvgl_x11
ctest --test-dir build-cookbook-lvgl_x11 --output-on-failure
```

Both interactive and `--smoke` runs use `DISPLAY`. On CI, an Xvfb server can
supply that display, for example `xvfb-run -a ctest --test-dir
build-cookbook-lvgl_x11 --output-on-failure`.

The X11 driver hides the desktop cursor, so startup supplies an arrow image.
LVGL keeps it on the display's system layer, above recipe content; page changes
leave it intact and display teardown releases it.

LVGL v9.6.0's X11 driver stops its tick thread only when the window manager
closes the window. `x11-lifetime.cmake` builds a corrected copy that also joins
the thread on programmatic display deletion and makes its stop flag atomic.
The supplied LVGL checkout is unchanged; updating LVGL requires reviewing this
version-specific fix.

### Shared startup and cleanup

`cookbook/lvgl/` holds the common LVGL implementation. Both executables always
create their selected native display and input devices. The configuration uses
libc allocation so memory checkers can observe individual LVGL allocations.
Application teardown preserves
the display-owned active screen and releases its CPPUI content before `lv_deinit`.
The example assumes CPPUI owns the content of that screen. Startup owns the
input devices and display; teardown releases those before quitting SDL (when
used) and deinitializing LVGL.

## Tests and memory checks

`--smoke` clicks every sidebar page three times, checks the two-column geometry,
scrollable list, Qt menus, native label contents, and A/B, progress and Canvas
rotation buttons. It switches the Tab recipe's pages and accepts/cancels the
Dialog recipe's prompt (including accepted empty text), opens/closes the Modal
recipe through its bound buttons, then tears down the
application. Sidebar native identity survives page changes.
All three CMake projects support
`-DCPPUI_VALGRIND_TESTS=ON`; `ctest -L valgrind --output-on-failure` runs the same
scenarios with Memcheck. Use a separate build for ASan/UBSan and Valgrind.
Both LVGL projects also test declaration retirement, native reuse, removal, and
application teardown through `lvgl.lifetime` and its optional Valgrind variant.
`lvgl.canvas` verifies rendered primitives, clipping, text rotation/size/anchors,
resize, repeat painting, callback replacement, exceptions, self-removal, native
deletion, parent deletion and capture cleanup. Its Valgrind variant checks the
same cases. The cookbook smoke test also checks Canvas size and rendered pixels.

Interactive SDL2/X11 input and real-display layout/focus checks remain outstanding.
Interactive close also needs an application-stop contract: LVGL's SDL2/X11
handlers can destroy the display or exit before CPPUI tears down declarations.
The tested smoke path returns to startup for ordered cleanup; it does not validate native
window-close handling. See the [LVGL plan](../doc/lvgl-progress.md) for that decision.

# Shared C++ cookbook

The main cookbook UI in `cookbook.cpp` and the recipes are shared by Qt, wxWidgets and LVGL. Each backend
has its own include selection, startup, and native smoke-test helpers. Separate
`qt`, `wx`, `lvgl_sdl2`, and `lvgl_x11` CMake projects select the toolkit and window
driver. Recipe code has no backend conditionals. Each executable links exactly
one toolkit. Qt corresponds to the Python reference's PySide6 backend.

Reference: [PUI cookbook at 009e9b9](https://github.com/buganini/PUI/tree/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook).

Button, Label, HBox, VBox, Canvas, Checkbox, RadioButton, ProgressBar, TimelineView
and Tab share their recipe implementations across all three backends.
Qt also enables shared Table, Table(Adapter), Tree, Tree(Adapter), Grid, Stack, Scroll, ComboBox, Text, Image, MDI, Splitter, Modal and Dialog recipes through
its source manifest. Checkbox's boolean, list and dictionary examples use
Binding adapters and explicit State assignment across backends. Qt also has a
partial TextField recipe with separate string model/draft bindings; dynamic
numeric conversion and typed input/change payloads remain pending.
The LVGL Canvas page keeps the 100x100 gray drawing area and rotation button.
Its basic painter uses LVGL software drawing and scaled bitmap text; image
resources and input callbacks remain pending. See the [Canvas reference](../REFERENCE.md#canvas)
for font/color differences and callback ownership.
The gallery preserves the reference's scrollable **Example** sidebar and expanding
**Result** pane. Qt and wx also keep the nested example menus and initially maximized
window. Only the source-code pane is omitted. Recipe spacing, weighted columns,
divider, and styled examples follow the reference on the available pages.

Pages awaiting a CPPUI implementation show “Not yet ported to this backend”;
`--list` labels them `not ported` and describes the remaining work. `partial`
means a recipe is enabled with listed gaps; `ported` means that recipe is
implemented, not that every toolkit feature or input path has been verified.
These statuses do not claim that the underlying toolkit lacks the feature.
Document specific native limitations separately, including any needed adapter
or custom implementation. The Python-only DrawShapely and Matplotlib recipes
are intentionally excluded (won't-do).
The [Qt plan](../doc/pyside6-progress.md) and
[LVGL plan](../doc/lvgl-progress.md) and [wx plan](../doc/wx-progress.md) track remaining components and public APIs.
Recipes use public CPPUI components and PUI's `.layout(...)` / `.style(...)`
modifiers for sizes, padding, weights, colors and fonts. There is no separate
cookbook component namespace. Backend helpers handle startup, optional menus
and native test access. Qt, wx and LVGL sidebars use public Scroll; LVGL's basic
Scroll does not yet implement the complete axis/offset contract of the Scroll
recipe. See the [modifier reference](../REFERENCE.md#pui-modifiers) for the typed
`.qt(...)` options and LVGL's compiled-font approximation.
The gallery smoke test checks horizontal and vertical Spacer expansion and the
Canvas drawing area in addition to page navigation.

The root Makefile provides `make qt`, `make wx`, `make lvgl_sdl2`, and
`make lvgl_x11` as shortcuts for configuring, building, and running each cookbook
executable in `build-cookbook-<backend>`. Each target launches the executable
after a successful build. Builds use `-j $(nproc)` and default
to `RelWithDebInfo`. Override `JOBS`, `BUILD_TYPE`, `CPPUI_LVGL_SOURCE_DIR`
(default: `lvgl-9.6.0` in the repository root), or pass additional configure
options through `CMAKE_ARGS` as needed. LVGL targets automatically run
`make lvgl-checkout` to clone v9.6.0 if its sources are missing; existing sources
are reused. Install the toolkit development packages first.
The equivalent direct CMake commands follow.

## Qt

```sh
cmake -S cookbook/qt -B build-cookbook-qt -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-cookbook-qt
./build-cookbook-qt/cppui_cookbook_qt
ctest --test-dir build-cookbook-qt --output-on-failure
```

The repository's root `CPPUI_BUILD_QT=ON` build also includes this executable and
its smoke test. CTest selects Qt's offscreen platform automatically.

## wxWidgets

Requires wxWidgets 3.2 development files including core, base and HTML libraries.
Install `libwxgtk3.2-dev` on Debian/Ubuntu or `brew install wxwidgets` on macOS.
Run these commands from the repository root:

```sh
cmake -S cookbook/wx -B build-cookbook-wx -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-cookbook-wx
./build-cookbook-wx/cppui_cookbook_wx
ctest --test-dir build-cookbook-wx --output-on-failure
```

The root `CPPUI_BUILD_WX=ON` build includes the same executable and smoke test.
Native wx controls require a display. Linux CI can use
`xvfb-run -a ctest --test-dir build-cookbook-wx --output-on-failure`.
`--list` reports the enabled recipes; `--smoke` navigates and tests them.
Add `-DCPPUI_VALGRIND_TESTS=ON` to a non-sanitized build for memory checks.
The wx gallery memory test allows 300 seconds for three full navigation cycles.

wx shares the controls, Grid, Stack, Scroll, Canvas, Image, Tab, Timeline, Dialog,
Table/Tree and adapter recipes. The Text page needs a Markdown renderer;
plain/HTML APIs are available separately. MDI, nonblocking Modal and multipane
Splitter need native integration decisions. See the [wx plan](../doc/wx-progress.md).
Reference: [PUI/wx at 2e4f0eb](https://github.com/buganini/PUI/tree/2e4f0eb82996e271bb3f4bc1f7511ea8b4c966bd/PUI/wx).

## LVGL

Use an [LVGL 9.6.0 source checkout](https://github.com/lvgl/lvgl/tree/80ca777e37a2b176770726a02e07a6fb79ef0b39).
The Makefile's LVGL targets clone these sources automatically if missing.
Run `make lvgl-checkout` to obtain just the sources. Direct CMake builds require
the sources to be supplied explicitly, using that target or a manual clone:

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

The X11 build uses a native arrow cursor, so pointer movement is independent of
LVGL rendering and remote desktop frame delivery. Both LVGL backends refresh
the display and sample input every 16 ms (about 60 Hz). X11 startup installs a
`std::chrono::steady_clock` tick callback to track elapsed time accurately;
SDL2 uses `SDL_GetTicks`.

LVGL v9.6.0's X11 driver stops its tick thread only when the window manager
closes the window. `x11-driver.cmake` builds a corrected copy that supplies the
native cursor, joins the thread on programmatic display deletion, and makes
its stop flag atomic. The tick callback supersedes the thread's simulated ticks.
The supplied LVGL checkout is unchanged; updating LVGL requires reviewing these
version-specific fixes.

### Shared startup and cleanup

`cookbook/lvgl/` holds the common LVGL implementation. Both executables always
create their selected native display and input devices. The configuration uses
libc allocation so memory checkers can observe individual LVGL allocations.
When the display is still open, application teardown preserves its active screen
and releases its CPPUI content before `lv_deinit`. Closing the last X11 window
deletes the display and returns from the application loop. Native observers
invalidate the deleted objects before the remaining declarations are released.
The example assumes CPPUI owns the content of that screen. Startup owns the
input devices and display; teardown releases those before quitting SDL (when
used) and deinitializing LVGL.

## Tests and memory checks

`CPPUI_COOKBOOK_TRACE=1` prints per-page smoke progress for diagnostics.
`--smoke` clicks every sidebar page three times, checks the two-column geometry,
scrollable list, Qt/wx menus, native label contents, bound controls, text editing,
scroll anchoring, and A/B, progress and Canvas rotation buttons where supported.
The Table page checks string edits, integer choices and separate value/label mappings.
Table(Adapter) checks an edit shared between two tables and rejection in read-only
rows. `qt.table_adapter` covers source replacement, State updates, header
fallback, exceptions and native/adapter reclamation, also under Valgrind.
The Tree page checks the nested hierarchy, expansion and the Add item button.
Tree(Adapter) checks two views of external data, different initial expansion, and
shared subtree insertion. `qt.tree_adapter` checks model consistency, stable
selection/indexes, source replacement, bad hierarchies and ownership under
sanitisers and Valgrind.
It switches the Tab recipe's pages and accepts/cancels the
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
`lvgl.modifiers` checks explicit/omitted/zero dimensions, padding, weights,
colors, compiled font selection, scrolling and cleanup. The root Qt build's
`qt.modifiers` also covers size policies, layout constraints, stylesheet
precedence, image rescaling and independently redrawn weighted children.
`lvgl.spacer` covers public HBox/VBox expansion, equal sharing, resize, native
reuse, virtual views, repeated removal, native deletion and parent/application
teardown, with an optional Valgrind variant.

`lvgl.checkbox` checks binding writes, mirrored controls, dependency rebinding,
feedback suppression, exceptions, callback removal and native cleanup; the
cookbook checks boolean and collection membership interactions.
`lvgl.radiobutton` checks exclusive selection, independent groups, binding and
callback replacement, repeated clicks, keyboard selection, unmatched values,
exceptions and cleanup. Both cookbooks exercise the shared A/B recipe.
`lvgl.progressbar` checks fractional values, range clamping, invalid declarations,
state updates, busy animation continuity and cleanup. `lvgl.timeline` checks
timing, native reuse, keyed moves, fixed mounted intervals, exceptions and timer
cancellation during removal, native deletion and display teardown. Both tests
advance a deterministic LVGL clock; the cookbook smoke test also exercises the
progress buttons and mounts/removes the clock recipe.
`lvgl.tab` covers virtual pages, explicit grouping, empty tabs, keyed moves,
selection, label updates, independent subviews and native/declaration teardown.
The cookbook smoke test switches both shared Tab pages.
`lvgl.widgets` checks label callback replacement/removal, divider orientation
through virtual views, plain-text wrapping and cleanup, including Valgrind.

Interactive SDL2/X11 input and real-display layout/focus checks remain outstanding.
X11 window-manager close returns through application and runtime cleanup.
`lvgl.lifetime` covers loop exit without a display, deletion of the last display,
and continued operation while another display remains. SDL2's direct-exit path
and an explicit quit/host-driven loop contract still need work; see the
[LVGL plan](../doc/lvgl-progress.md).

The shared MDI recipe is enabled on Qt. TimelineView is enabled on all three
backends; its timer stops when the recipe is removed.

Qt smoke startup uses immediate `sync()`; interaction checks process Qt events to
settle queued redraws and lifecycle notifications. LVGL startup remains immediate.
The root build's `qt.tree` regression also checks model invariants with Qt Test,
persistent indexes, selection and expansion after reordering, independent virtual
subviews, callback retirement and native teardown, including Valgrind.

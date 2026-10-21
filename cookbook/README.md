# Shared C++ cookbook

All cookbook C++ sources and headers live directly in `cookbook/` and are shared
by Qt, wxWidgets and LVGL. Every executable compiles the same source list in
`common.cmake`. Only CMake files remain in the backend directories.
The public `cppui/backend.hpp` facade selects toolkit headers and exposes the
backend's name, view aliases and capabilities. Cookbook sources use only CPPUI
declarations; each backend owns its native startup and cleanup. Native event
injection and assertions live in `tests/cookbook/`, linked only into separate
`cppui_cookbook_<backend>_smoke` executables when `BUILD_TESTING=ON`.
Qt corresponds to the Python reference's PySide6 backend.

The separate `qt`, `wx`, `lvgl_sdl2` and `lvgl_x11` CMake projects define exactly
one of `CPPUI_BACKEND_QT`, `CPPUI_BACKEND_WX` or `CPPUI_BACKEND_LVGL`.
The facade rejects ambiguous selections. LVGL's configuration selects SDL2 or
X11. Shared recipes use `CPPUI_HAS_*` capability flags for optional features,
including bound Tab selection. Each executable links only its selected toolkit.

Reference: [PUI cookbook at c50b33f](https://github.com/buganini/PUI/tree/c50b33fcf7bdfdd7a353856b56733ea5969b8211/PUI/cookbook).

Native additions are audited through committed local/public PUI `c50b33f` (2026-10-22).
Grid now displays a manual colored mosaic. Adaptive Grid demonstrates
`.columns()` / `.wideColumns()` at 384/576/768 logical pixels, the three backfill
modes, hidden-item compaction, child span expansion, bound reversal and a retained text editor.
The gallery uses a horizontal Splitter for the Code and Result panes on all
backends. CMake defines `CPPUI_COOKBOOK_SOURCE_DIR` as the absolute cookbook
directory. Selecting a recipe or pressing **Reload** reads its C++ source from
disk, so source display updates do not require a rebuild and do not depend on
the process's working directory. Each recipe displays `<recipe-key>.cpp`,
including separate Button, Label, HBox and VBox files. The Code pane shows plain
text in a scrollable area. Missing files show their expected path. Keep the source
checkout at its build-time location; the executable does not bundle these files.

ComboBox demonstrates
`.autoexpand(false)` and a separate `.editModel(query)` binding. Its application
matching splits whitespace and OR-matches airport labels: `tai tok` yields four
Taipei/Tokyo candidates on Qt, wx and LVGL, with their original airport codes.
Clear resets both query and destination. Uncommitted PUI work, AI development
and Web/Pyodide changes remain excluded.
Adaptive displays reactive container dimensions and switches compact/wide layouts
at 600 logical pixels while sharing input state on Qt, wx and LVGL. It also
shows a retained input in `HVBox(600)` and height-dependent `VHBox(500)` labels.
Its `wideLayout`/`wideStyle` label changes padding, color and font size at the
same 600-pixel threshold. The gallery shell stays horizontal at narrow widths,
with the menu beside the Code/Result splitter, and opens at 1024×768 without maximizing.
Window titles and sidebar headings identify the active backend: `Cookbook (Qt)`,
`Cookbook (wxWidgets)`, `Cookbook (LVGL SDL2)` or `Cookbook (LVGL X11)`.
State Binding now uses an external getter/setter and shows the latest 100
getter/setter/change messages in a scrollable in-app log on all backends.
Table marks editable cells with `editable: ` while storing their raw values.
Refs demonstrates queued selection on every backend and window commands on
Qt/wx. The Qt/wx Drop page accepts files, directories, text and URLs. Image uses
the bundled public-domain Rijksmuseum windmill with attribution. Native performance changes are evaluated
separately; see the [Canvas benchmark decision](../doc/canvas-performance.md).
MaskedField runs on Qt, wx and LVGL with
single/buffered passwords and length labels. All backends include the airport
autocomplete ComboBox, HeavyCanvas and decorated Table price editors. HeavyCanvas
offers fourteen presets from 1,000 to 1,000,000 synthetic sessions,
indicator panels, hover details, wheel zoom, regeneration and zoom reset.
Datasets above 500,000 use zero-based grouped index labels instead of dates.
These presets draw every session and intentionally stress native rendering.
Run a `_smoke` executable with `--heavy-smoke` to render 1,000,000 sessions and
switch back to 1,000; its default run retains the faster 30,000-session coverage.
The `cookbook.heavycanvas_data` test checks full million-row generation.
Button, Checkbox, RadioButton and ComboBox include disabled examples matching PUI.
SDL2/X11 use the same recipe and native Canvas input. Image resources and custom
font loading remain separate LVGL Canvas work.

Button, Label, HBox, VBox, Canvas, Checkbox, RadioButton, ProgressBar, TimelineView,
Text, Image, Tab, Table, Table(Adapter) and Splitter share their recipe implementations
across all three backends.
LVGL Splitter supports nested horizontal/vertical panes and pointer/touch drag
handles. Redraw retains pane sizes and controls; changing composition or
orientation resets the allocation, and window resizing preserves proportions.
LVGL Image uses the bundled PNG/baseline-JPEG decoders and preserves the windmill's
aspect ratio at its declared width. Canvas image resources remain separate work.
Views also shares internal and bound selection examples across all backends.
Its TextField and Change text button demonstrate retained local state.
Qt also enables shared Tree, Tree(Adapter), Grid, Stack, Scroll, ComboBox, Image,
MDI, Splitter, Modal and Dialog recipes through the common registration table.
Checkbox's boolean, list and dictionary examples use
Binding adapters and explicit State assignment across backends. All backends have a
partial TextField recipe with separate string model/draft bindings; dynamic
numeric conversion and typed input/change payloads remain pending.
The wx Text page demonstrates plain multiline text and HTML. Its Markdown
example displays the raw source, matching PUI/wx. Calling the wx MarkDown factory
emits a compile-time warning that Markdown formatting is not rendered; the page
remains partial.
The LVGL Text page uses the same shared recipe: plain multiline text plus raw
HTML and Markdown source, with a visible formatting notice. Both fallback
factories warn at compile time; formatting and interactive selection remain
unimplemented, so the page is partial on SDL2 and X11.
The LVGL Canvas page keeps the 100x100 gray drawing area and rotation button.
Its basic painter uses LVGL software drawing and scaled bitmap text; image
resources remain pending. Move, leave and wheel callbacks work on SDL2 and X11. See the [Canvas reference](https://buganini.github.io/CPPUI/doc/Reference.html#canvas)
for font/color differences and callback ownership.
The gallery preserves the reference's scrollable cookbook sidebar and expanding
**Code**/**Result** splitter. Qt and wx also keep the nested example menus.
Recipe spacing, weighted columns,
divider, and styled examples follow the reference on the available pages.

Pages awaiting a CPPUI implementation show “Not yet ported to this backend”;
`--list` labels them `not ported` and describes the remaining work. `partial`
means a recipe is enabled with listed gaps; `ported` means that recipe is
implemented, not that every toolkit feature or input path has been verified.
These statuses do not claim that the underlying toolkit lacks the feature.
Document specific native limitations separately, including any needed adapter
or custom implementation. The Python-only DrawShapely and Matplotlib recipes
are intentionally excluded (won't-do).
The [Qt plan](../doc/qt-progress.md) and
[LVGL plan](../doc/lvgl-progress.md) and [wx plan](../doc/wx-progress.md) track remaining components and public APIs.
Recipes use public CPPUI components and `.layout(...)` / `.style(...)`
modifiers for sizes, padding, weights, colors and fonts. Use typed named helpers,
for example `.layout(CPPUI::layout::width(200), CPPUI::layout::padding(8))`
and `.style(CPPUI::style::fontSize(20), CPPUI::style::color(0x336699))`.
The Adaptive recipe uses the same helpers after the width threshold in
`.wideLayout(600, ...)` / `.wideStyle(600, ...)`. Positional calls also remain
supported. There is no separate cookbook component namespace.
Backend helpers handle startup, optional menus
and native test access. Qt, wx and LVGL sidebars use public Scroll; LVGL's basic
Scroll does not yet implement the complete axis/offset contract of the Scroll
recipe. See the [modifier reference](https://buganini.github.io/CPPUI/doc/Reference.html#pui-modifiers) for all named
helpers, borrowed `fontFamily` strings, typed `.qt(...)` options and LVGL's
compiled-font approximation.
The gallery smoke test checks horizontal and vertical Spacer expansion and the
Canvas drawing area in addition to page navigation.

Run the following commands from the repository root. The Makefile provides
`make qt`, `make wx`, `make lvgl_sdl2`, and
`make lvgl_x11` as shortcuts for configuring, building, and running each cookbook
executable in `build-cookbook-<backend>`. Each target launches the executable
after a successful build. Builds use `-j $(nproc)` and default
to `RelWithDebInfo`. Override `JOBS`, `BUILD_TYPE`, `CPPUI_LVGL_SOURCE_DIR`
(default: `lvgl-9.6.0` in the repository root), or pass additional configure
options through `CMAKE_ARGS` as needed. LVGL targets automatically run
`make lvgl-checkout` to clone v9.6.0 if its sources are missing; existing sources
are reused. Install the toolkit development packages first.

## Qt

Requires Qt 6 development packages.

```sh
make qt
```

The repository's root `CPPUI_BUILD_QT=ON` build also includes this executable and
its smoke test. CTest selects Qt's offscreen platform automatically.

## wxWidgets

Requires wxWidgets 3.2 development files including core, base and HTML libraries.
Install `libwxgtk3.2-dev` on Debian/Ubuntu or `brew install wxwidgets` on macOS.

```sh
make wx
```

The root `CPPUI_BUILD_WX=ON` build includes the same executable and smoke test.
Native wx controls require a display; Linux CI can supply one with Xvfb.
The gallery's `--list` reports the enabled recipes; the separate `_smoke`
executable navigates and tests them.
Add `-DCPPUI_VALGRIND_TESTS=ON` to a non-sanitized build for memory checks.
The wx gallery memory test allows 300 seconds for three full navigation cycles.

wx shares the controls, Grid, Stack, Scroll, Splitter, Canvas, Image, Tab, Timeline, Dialog,
Table/Tree and adapter recipes. The Text page needs a Markdown renderer;
plain/HTML APIs are available separately. Splitter uses nested native two-pane
controls for the same shared multipane recipe. MDI and nonblocking Modal need
native integration decisions. See the [wx plan](../doc/wx-progress.md).
Reference: [PUI/wx at 2e4f0eb](https://github.com/buganini/PUI/tree/2e4f0eb82996e271bb3f4bc1f7511ea8b4c966bd/PUI/wx).

## LVGL

Use an [LVGL 9.6.0 source checkout](https://github.com/lvgl/lvgl/tree/80ca777e37a2b176770726a02e07a6fb79ef0b39).
The Makefile's LVGL targets clone these sources automatically if missing.
Run `make lvgl-checkout` to obtain just the sources.

Table editing uses native LVGL textareas/dropdowns over the selected cell:
double-click or Enter opens the editor, Enter/choice activation commits, and Esc
cancels. Prefix/suffix stay outside the stored value. Headers scroll with the
native table; a redraw discards an unfinished draft. Table(Adapter) shows two
views of one State matrix. The `lvgl.table` and `lvgl.table_adapter` regressions
exercise real keypad/pointer events, scrolling, typed edits, source replacement,
rollback and callback-driven redraw/removal.

### SDL2

Install SDL2 development packages, then run:

```sh
make lvgl_sdl2
```

CTest uses SDL's dummy video and software-renderer drivers. To watch the same
smoke scenario in a window, run `cppui_cookbook_lvgl_sdl2_smoke` from its build
directory without those driver overrides.

### X11

Install X11 development packages and use an available X display:

```sh
make lvgl_x11
```

Both interactive and `_smoke` executables use `DISPLAY`. On CI, an Xvfb server
can supply that display.

The X11 build uses a native arrow cursor, so pointer movement is independent of
LVGL rendering and remote desktop frame delivery. Both LVGL backends refresh
the display and sample input every 16 ms (about 60 Hz). X11 startup installs a
`std::chrono::steady_clock` tick callback to track elapsed time accurately;
SDL2 uses `SDL_GetTicks`.

LVGL v9.6.0's X11 driver stops its tick thread only when the window manager
closes the window. `cmake/CPPUILVGLX11.cmake` builds a corrected copy that supplies the
native cursor, joins the thread on programmatic display deletion, and makes
its stop flag atomic. The tick callback supersedes the thread's simulated ticks.
The supplied LVGL checkout is unchanged; updating LVGL requires reviewing these
version-specific fixes.

### Shared startup and cleanup

`CPPUI::LVGL::Application` lazily creates the selected desktop display, input
devices and focus group when first rendering. `cppui/lvgl/desktop_conf.h` configures
both drivers; `cookbook/lvgl/` contains only CMake build helpers. The gallery's
`--list` does not initialize a display. The configuration uses libc allocation
so memory checkers can observe individual LVGL allocations.

Application teardown releases CPPUI content before its input devices, display,
SDL subsystem and LVGL runtime. Closing the last native window returns from the
application loop; subsequent redraws do not reopen it. Native observers invalidate
deleted objects before the remaining declarations are released. If an embedding
host supplies an existing display, Application borrows it and preserves its
runtime, display, input devices and focus group. Window uses the active screen
and assumes exclusive ownership of its CPPUI content.

## Tests and memory checks

The Makefile targets build and run the interactive cookbook only. The checks
below require a separate test build; see the root README for
[Qt/wx build and test instructions](../README.md#build-and-test-the-core).

`CPPUI_COOKBOOK_TRACE=1` prints per-page smoke progress for diagnostics.
The new native cases exercise masked drafts/commit/clear, decorated price cells,
raw autocomplete codes and HeavyCanvas zoom/reset/preset changes up to 30,000
sessions. wx's gallery timeout allows for the larger chart under sanitizers.
The `_smoke` executable clicks every sidebar page three times, checks the menu and Code/Result geometry,
source file contents and Reload without replacing the result pane,
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
same cases. `lvgl.canvas_input` checks local move/leave/wheel events, hit testing,
window exit and callback/device teardown. `lvgl.combobox` checks binding precedence,
declared candidates, edit/input bindings, expansion toggles, keyboard acceptance,
duplicate/empty values, scoped keyed items and self-removal. `qt.combobox` and
`wx.combobox` cover their native completion controls, including reactive edits
and cursor/selection retention. The shared smoke tests exercise whitespace/OR
matching and selection against the filtered airport list.
The cookbook smoke test checks Canvas size and rendered pixels and drives
HeavyCanvas zoom through actual SDL2/X11 input queues.
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
`lvgl.textfield` checks separate model/draft bindings, keyboard-group input,
Enter/focus commits, normalization and rejection, callback replacement,
unfinished drafts across redraw/page switches, exceptions, callback-driven
removal, deferred draft synchronization and native/display teardown. The SDL2
cookbook owns a keyboard/pointer focus group; X11 uses its driver's group.
Cookbook smoke checks assert that keyboard devices use the default group and
exercise both the TextField and Views recipes through it.
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
`lvgl.views` covers optional/internal selection, empty and multi-root pages,
hidden-page redraw, retained controls/state, keyed moves, invalid IDs, model
rebinding, setter-driven removal and native/display teardown. The cookbook
switches and clears both Views examples and checks retained state and controls.
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

HeavyCanvas follows PUI `39759a2`/`e46cb43`: completed-draw FPS, optional
continuous drawing and full-frame color changes are available on Qt/wx/LVGL.

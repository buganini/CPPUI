# LVGL implementation progress and remaining work

## Baseline and scope

Plan prepared on 2026-10-12 against:

- [CPPUI at 5befe3353d7f372dee4418cd439b833d5f15f93a](https://github.com/buganini/CPPUI/commit/5befe3353d7f372dee4418cd439b833d5f15f93a).
- [PUI at 009e9b9c3fbd724f0b73c862f8213ddc4742405c](https://github.com/buganini/PUI/tree/009e9b9c3fbd724f0b73c862f8213ddc4742405c),
  for shared reactive behavior and [cookbook examples](https://github.com/buganini/PUI/tree/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook).
- [LVGL 9.6.0 at 80ca777e37a2b176770726a02e07a6fb79ef0b39](https://github.com/lvgl/lvgl/tree/80ca777e37a2b176770726a02e07a6fb79ef0b39),
  for native objects, events, widgets, displays, and SDL2/X11 integration.

The pinned PUI tree has no LVGL backend. This is a plan for CPPUI's existing
`cppui/lvgl/` implementation, not a claim of direct Python/LVGL parity. Preserve
shared C++11 declarations and recipes while mapping behavior explicitly onto
LVGL. Desktop-only features need a stated LVGL capability or omission.

Coordinate shared-core work and public binding/layout/event contracts with the
[Qt alignment plan](pyside6-progress.md). Do not create competing backend-specific
public APIs to work around an unresolved common contract. Record blockers below
and continue independent tasks. Keep this document limited to remaining work
and current contracts; summarize completed changes briefly in `Changelog.txt`.

## Current basis and remaining acceptance

Use **not yet ported** for missing CPPUI implementations and **partial** for
implemented behavior with remaining gaps. An unchecked task remains work to do.
Reserve **native limitation** for a verified LVGL API constraint, with its source
and the adapter or custom implementation needed. A missing dedicated widget does
not establish that a feature is impossible. **Excluded** means an explicit scope
decision, such as the Python-only integrations below. An unresolved design choice
must name the choice; it must not be reported as toolkit non-support. Call work
**blocked** only when a specific dependency prevents progress, and name that
dependency. Routine implementation choices remain part of the porting work.

Public LVGL headers currently cover Application, Window, HBox/VBox, Label,
Button, Checkbox, RadioButton, TextField, MaskedField, Spacer, Divider, plain Text with raw-source
Html/MarkDown fallbacks, basic Scroll, Tabs/Tab, Views, ProgressBar, TimelineView
and Canvas drawing. Shared recipes use the
public PUI layout/style modifiers; see the
[component reference](../REFERENCE.md#pui-modifiers) for native mappings and limits.

Shared options from PUI `020ef46`/`184ffcd` map to Button `.enabled(bool)` using
`LV_STATE_DISABLED` and TextField `.placeholder(text)` using the native textarea
hint. Omission on a fresh declaration restores enabled/empty defaults. Both
SDL2/X11 cookbooks include the TextField hint; regressions cover native reuse,
reactive button state, hint clearing and active draft/cursor preservation.
ComboBox remains a separate unported wrapper.

The native PUI delta audit through `8a8ef99` (2026-10-16) adds MaskedField
(`139d070`) through native textarea password mode. It shares TextField's string
model/draft, placeholder, callback and lifetime behavior. Both SDL2/X11 enable
the shared password cookbook. HeavyCanvas remains not yet ported here because
ComboBox and Canvas pointer callbacks are missing; Qt/wx implement the recipe.
Application display-name/title fallback and native Table decorations belong to
the desktop wrappers; LVGL host titles/Table remain tracked below. Web/Pyodide
changes are excluded at the user's request.

| Area | Current basis | Remaining acceptance / phase |
| --- | --- | --- |
| Core integration | Shared reconciliation, nested virtual views, standalone view cleanup and top-level builder recovery | Nested partial-sync failure recovery needs a shared contract |
| Native lifetime | Views own declarations; primary/secondary native observers clear on deletion; callbacks detach on retirement | Define external remount and shared-screen ownership before broadening interop (2, 6) |
| Application and native drivers | Loop returns when the last display closes; LVGL delay callback; separate SDL2/X11 startup and cleanup | Define explicit quit/host-driven pumping and borrowed-runtime ownership; handle SDL2 direct exit (2) |
| Label/Button | Owned text, styles and current button/label callbacks | Input payloads follow the deferred common contract (3) |
| Grid/Stack | Not yet ported to CPPUI | Add shared placement API and choose native track sizing, then verify spans/overlap (3) |
| ProgressBar/Text/Image | ProgressBar with hundredths, clamping and busy animation; plain wrapping Text and raw Html/MarkDown fallbacks in the shared cookbook | Text selection/formatting and Image replacement/resource lifetime remain to port (4) |
| Bound controls | Boolean Checkbox, typed RadioButton and string TextField with Binding/State; cookbook membership/draft examples | ComboBox value selection remains to port; shared text conversion and event payloads remain deferred (4) |
| Scroll/Splitter | Basic public Scroll | Map mixed forced/automatic axis policies, add anchoring and decide splitter capability (5) |
| Views | Retained native pages, internal/optional bound selection, keyed reuse and shared cookbook | Real-display input/layout checks remain part of general validation |
| Modal/dialog/menu/window features | Active-screen Window only | Agree LVGL overlays and desktop-only omissions; use shared Optional results and preserve borrowed-screen ownership (5) |
| Timeline/Canvas/resources | Cancellable TimelineView timers; basic Canvas primitives, text and native-resolution buffers | Add image resources, font/density capabilities and deferred input support (6) |
| Table/Tree/adapters/interop | Shared typed adapter interfaces; LVGL wrappers not yet ported | Map the public interfaces to LVGL models or custom widgets and define interop borrowing (6) |
| Validation | Independent headers, C++11, lifecycle and cookbook CTest; sanitizer/Valgrind runs | Expand component/input cases, real SDL2/X11 interaction, multiple resolutions and target-device validation (2–6) |

## Ownership contract

- Factory results remain borrowed construction references. Applications keep
  State and stable IDs, not nodes across reconciliation. Do not add public weak
  wrappers or reference-counted node ownership.
- Declarations own their tree and callback data. Native `.ui` access observes
  an LVGL object; LVGL's parent tree owns child native objects. Make that access
  typed and invalidate every associated observer before its native owner dies.
- On reuse, transfer native access and install current callbacks before releasing
  the predecessor. Internal retired-node forwarding is valid only while the
  framework still owns both declarations.
- Cancel events/timers and detach subscriptions before destroying declarations.
  Snapshot active user callbacks. Define safe completion of callbacks which
  redraw, remove themselves, or request application shutdown.
- `Window` currently borrows the display's active screen and assumes exclusive
  ownership of its CPPUI content. It must not delete the screen. This is not yet
  an arbitrary shared-screen, multi-window, or externally replaced-screen API.
- Application content must be released before input devices/displays, then SDL,
  then LVGL. The cookbook startup owns those runtime resources; do not make the
  reusable backend unconditionally deinitialize a host-owned LVGL runtime.
- Confine state mutation, reconciliation and LVGL access to the owning UI thread.
  Background work must marshal there; scheduler APIs need the shared completion
  and error-delivery contract.

## Implementation sequence

### Phase 2 — Runtime, SDL2/X11 and input

Reference: [LVGL SDL event loop](https://github.com/lvgl/lvgl/blob/80ca777e37a2b176770726a02e07a6fb79ef0b39/src/drivers/sdl/lv_sdl_window.c),
[keyboard driver](https://github.com/lvgl/lvgl/blob/80ca777e37a2b176770726a02e07a6fb79ef0b39/src/drivers/sdl/lv_sdl_keyboard.c),
and [X11 driver](https://github.com/lvgl/lvgl/blob/80ca777e37a2b176770726a02e07a6fb79ef0b39/src/drivers/x11/lv_x11_display.c).

The X11 cookbook uses a native arrow cursor, a monotonic `steady_clock` tick
callback, and a sleeping delay callback. Both desktop backends use 16 ms
display/input timers. It builds a corrected copy of the v9.6.0 display driver
to install the native cursor and stop and join its tick thread during
programmatic deletion, with an atomic stop flag. The clock callback supersedes
the thread's simulated ticks.
Review this version-specific workaround when updating LVGL. The application loop
returns after the last display is deleted and uses LVGL's delay callback.
X11 window-manager close invalidates native handles, releases declarations, then
returns to startup for remaining input/runtime cleanup. Lifetime tests cover
no-display entry, last-display close and multiple-display loop ownership.

- [ ] Add explicit quit and host-driven pumping beyond last-display loop exit.
  Define tick, delay, idle and host ownership responsibilities.
- [ ] Route SDL2 direct exit through application cleanup and define borrowed
  display/runtime close ownership. Test pending work, repeated quit and teardown
  from an active callback; verify the host's runtime survives borrowed use.
- [ ] Extend host-facing focus-group and keyboard/encoder ownership contracts.
  The SDL2 cookbook owns its default pointer/keyboard group, and X11 uses the
  driver's group; TextField borrows these through native object membership.
  Exercise SDL2/X11 pointer,
  wheel and keyboard input through the native driver, including scrolling to
  clipped sidebar pages and focus after page changes.
- [ ] Verify real-display rendering/input at multiple sizes and on an embedded
  display/input target. Dummy-driver smoke checks do not validate physical input.

Acceptance: native close returns through application cleanup; no event reaches
retired declarations or a deleted display; keyboard focus reaches usable controls.

### Phase 3 — Public layout, styling and existing widgets

Reference: shared [HBox](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/hbox.py),
[VBox](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/vbox.py),
[Grid](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/grid.py),
[Stack](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/stack.py),
[Label](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/label.py)
and [Button](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/button.py) cases.

- [ ] Add public Grid and Stack after native track sizing is agreed; verify
  nested padding, grid spans and overlapping child order.
- [ ] Add input payloads after the deferred common event contract is addressed.
- [ ] Verify layout/style geometry at multiple display sizes and densities;
  define runtime font resources beyond the current compiled-font approximation.

### Phase 4 — Text, progress, images and bound controls

- [ ] Implement interactive text selection and choose rich-text/Markdown
  renderers. The shared `text.cpp` page is enabled on SDL2/X11 with plain wrapping
  Text and raw-source Html/MarkDown fallbacks. Fallback calls warn at compile
  time, the page displays a formatting notice, and `--list` marks it partial.
  This does not implement formatted rendering or selection.
- [ ] Add Image with owned/borrowed source lifetime, decoding failure behavior,
  declared dimensions and replacement cleanup.
- [ ] Using the shared `Binding<T>`/State adapters, add ComboBox/item equivalents.
  Preserve distinct labels/values, empty selections and current callbacks
  through redraw. Keep
  automatic collection observation and value-based Checkbox overloads tied to
  the shared collection contract; explicit boolean membership adapters work now.

Acceptance: port [text.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/text.py),
[image.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/image.py)
and the binding/control pages. Assert one commit per edit, no feedback loop,
native reuse, valid model retention after invalid input, and resource reclamation.

### Phase 5 — Containers and window interactions

Views adopts the shared PUI behavior introduced in `e5927cb` and `65f2c02`:
unique non-empty child IDs, internal or optional bound selection, retained page
state, and blank content for cleared or unknown keys without rewriting the model.
Each page is an LVGL VBox; hidden pages remain mounted and can redraw independently.
Selection dispatch follows the current declaration across synchronous redraws,
and stops safely if a model setter removes the container or destroys its owner.
The shared cookbook uses TextField and a Change text button to exercise retained
state and native editor reuse.

- [ ] Extend basic Scroll with `.scrollX()`/`.scrollY()`, the shared signed-zero
  end anchor and position retention through resize, growth, shrinkage and content
  replacement. Resolve mixed forced/automatic axis policies before supporting
  that combination; it currently throws explicitly.
- [ ] Implement Splitter with custom resizable panes, or record an explicit scope
  decision to omit it. The missing CPPUI wrapper is not a proven LVGL limitation.
  Do not duplicate the shared splitter recipe to hide capability differences.
- [ ] Agree modal overlays, dialogs, menus/toolbars and Window capabilities
  suitable for LVGL before implementing them. Desktop native dialogs, MDI,
  window-manager flags and Qt embedding have no automatic LVGL equivalent.

Acceptance: port [scroll.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/scroll.py),
[tab.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/tab.py),
[splitter.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/splitter.py)
and agreed modal/dialog cases, or report the omitted capability explicitly.

### Phase 6 — Timers, drawing, data widgets and embedding

- [ ] Add LVGL ImageResource/loadImage/drawImage after agreeing source ownership,
  decoder availability and file access; test crop/scale, source regions, opacity,
  replacement and missing images.
- [ ] Define Canvas font loading and display-density scaling. Basic text scales
  the object's bitmap font to a pixel line height; Qt uses font points. Verify
  device rendering/antialiasing expectations before promising equivalent output.
  Pointer events remain tied to the explicitly deferred shared input contract.
- [ ] Implement Table/Tree against the public typed adapter interfaces;
  investigate custom tree behavior separately from LVGL's table widget. Preserve
  adapter/data ownership; arbitrary mixed/custom values remain a shared gap.
- [ ] Define native-object/display embedding and external owner deletion before
  exposing interop. Support shared-screen content only under an explicit scope.

Acceptance: use the reference [Canvas](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/canvas.py),
[ragged table](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/table_ragged.py)
and [tree sync](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/tree_sync.py)
cases where the agreed LVGL capability supports them. **Won't do — Shapely and
Matplotlib:** the Python-only Canvas.drawShapely and MatplotlibCanvas integrations
and their cookbook recipes are excluded from CPPUI.

## Implementation choices and native constraints

- **Scroll policy mapping:** Qt now follows PUI's Optional<bool> axis policies,
  signed-zero end anchor and pre-sync position capture. LVGL has one scrollbar
  visibility mode for an object. Basic public Scroll rejects a forced axis paired
  with an automatic axis; choose a mapping or custom scrollbar implementation for
  that mixed case. The offset representation needs no further shared decision.
- **Grid sizing:** existing node coordinates/spans can drive placement. LVGL
  additionally requires explicit row/column track arrays; choose content-sized
  versus fractional tracks and their interaction with the shared size/weight
  options. See the pinned [Grid API](https://github.com/lvgl/lvgl/blob/80ca777e37a2b176770726a02e07a6fb79ef0b39/include/lvgl/layouts/lv_grid.h).
- **Selection controls:** LVGL's [dropdown](https://github.com/lvgl/lvgl/blob/80ca777e37a2b176770726a02e07a6fb79ef0b39/include/lvgl/widgets/lv_dropdown.h)
  takes newline-separated labels and exposes an unsigned selected index; it does
  not supply Qt's editable text field or negative-index empty selection contract.
  Choose a restricted native control versus a composite implementation.
  RadioButton already uses LVGL 9.6's native sibling exclusion via
  `lv_obj_set_radio_button`; separate layouts form separate groups. General
  focus-group ownership remains in the runtime/input tasks above.
- **Shared contracts:** use the shared Binding/State adapters and follow the Qt plan's
  default reset for omitted properties, stderr render errors, Optional dialog
  results and PUI virtual Tab mapping. Input payloads are explicitly deferred;
  remaining subscription/value types, retained-view inputs and nested-sync
  recovery stay coordinated there. Inspect reference code/tests before raising
  any further decision; do not block independent work on those gaps.
- **Runtime ownership and stop:** Application returns when the last display closes;
  LVGL's SDL2 driver can still call `exit`. Choose host-owned versus
  backend-owned pumping and native-event interception for an explicit quit API.
  Native resources must outlive declarations in both modes.
- **Screen ownership:** active-screen content is currently exclusive. Choose
  scoped content roots, screen switching, multiple displays and external-delete
  remount behavior before broadening Window or interop.
- **Editor commits:** TextField preserves active drafts and commits on Enter or
  focus loss, with adapter readback for rejected/normalized values. Idle draft
  synchronization uses a cancellable LVGL async task after reconciliation;
  native events keep lifetime state across synchronous redraws and stop after
  removal. The shared string model/draft API is ported. Shared numeric conversion
  and typed input/change payloads remain deferred across backends.
- **Capability differences:** decide rich text and
  selection, font/image resources, custom splitters/trees, and desktop features
  independently. State verified native limits separately from unfinished wrappers;
  pixel-identical desktop rendering is not implied by sharing recipe source.

## Shared cookbook and verification

Keep `cookbook/cookbook.cpp` and recipe sources shared. Select SDL2 or X11 in
`cookbook/lvgl_sdl2/` or `cookbook/lvgl_x11/`, with shared LVGL sources, headers
and build logic in `cookbook/lvgl/`; keep native APIs and conditional
compilation out of recipe files. Preserve the Example sidebar, scrolling list,
Result pane and recipe layouts. Only the reference's source-code pane is omitted;
`--list` reports unfinished wrappers as `not ported` and enabled recipes with gaps
as `partial`. Record any verified toolkit limitation separately in the docs.

SDL2 C++11 build and tests (requires SDL2 development packages):

```sh
cmake -S cookbook/lvgl_sdl2 -B build-cookbook-lvgl_sdl2 \
  -DCPPUI_LVGL_SOURCE_DIR=lvgl-9.6.0 -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-cookbook-lvgl_sdl2
ctest --test-dir build-cookbook-lvgl_sdl2 --output-on-failure
```

For X11, substitute `lvgl_x11` for `lvgl_sdl2` in both source/build paths; install
X11 development packages and provide `DISPLAY`. See the [cookbook instructions](../cookbook/README.md)
for full commands and the shallow v9.6.0 checkout. Both executables use their
selected native driver for `--smoke` and interactive runs. CTest uses dummy
video/software rendering for `cookbook.lvgl_sdl2`; `cookbook.lvgl_x11` uses an
X server, optionally Xvfb. Keep interactive close, physical input/focus and
embedded-device checks pending until explicitly tested.

Enable `CPPUI_VALGRIND_TESTS=ON` in a non-sanitized build and run
`ctest --test-dir build-cookbook-lvgl_sdl2 -L valgrind --output-on-failure` (or use
the X11 build directory). New cases
must use `cppui_add_test` so memory checks cover them. Run ASan/UBSan/leak checks
in a separate build with both the LVGL C library and CPPUI C++ code instrumented.
Keep per-allocation libc tracking in the host test configuration. No invalid
accesses or definite/indirect/possible leaks are acceptable; reachable library
globals are reported separately. Do not hide framework leaks with suppressions.

After each completed change, update component/build docs, examples and tests;
remove its finished tasks from this plan; add a concise component-prefixed
change summary to `Changelog.txt`, merging related work into milestone entries;
then commit before the next change. Keep detailed validation commands/results
in tests, documentation or commit messages.
Update the Qt plan only for shared contracts or shared-core gaps, keeping
LVGL-specific remaining work here.

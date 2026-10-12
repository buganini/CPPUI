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

Public LVGL headers currently cover Application, Window, HBox/VBox, Label,
Button, Spacer and basic Canvas drawing. The shared cookbook additionally has
private presentation helpers for weights, styles, scrolling, dividers and page
links. Those helpers do not establish public component support.

| Area | Current basis | Remaining acceptance / phase |
| --- | --- | --- |
| Core integration | Shared state/reconciliation, common retirement updates | Nested virtual views retain native order and state; direct view destruction and failed builds reclaim declarations (1) |
| Native lifetime | Application owns declarations; Window borrows the active screen | Deletion notifications invalidate observers; active callbacks, parent deletion and screen replacement cannot access retired nodes (1) |
| Application and native drivers | Timer loop; separate SDL2/X11 cookbook startup and smoke teardown | Define stop/event ownership, then close gracefully before native display destruction; support host-driven pumping without POSIX-only assumptions (2) |
| HBox/VBox | Basic flex flow and child sizing | Common padding/dimensions/weights reset correctly, including zero; nested and virtual geometry remains stable (3) |
| Label/Button | Owned text and current button callbacks | Public label clicks, event payloads and style reset follow common contracts; repeated callbacks/removal release captures (1, 3) |
| Spacer/Divider/Grid/Stack | Public expanding Spacer; cookbook-only divider | Define fixed spacing and shared placement/style API, then add Divider/Grid/Stack and verify spans/overlap (3) |
| ProgressBar/Text/Image | Missing public components | Numeric range conversion, wrapping, source replacement and resource lifetime are explicit; rich-text gaps stay visible (4) |
| Bound controls | Missing | Text edits and boolean/value selections commit once, rebind correctly and avoid feedback loops (4) |
| Scroll/Tabs/Tab/Splitter | Cookbook-only scrolling; other public components missing | Preserve offsets/selection across content changes; apply PUI's virtual Tab mapping and decide splitter capability (5) |
| Modal/dialog/menu/window features | Active-screen Window only | Agree LVGL overlays and desktop-only omissions; use shared Optional results and preserve borrowed-screen ownership (5) |
| Timeline/Canvas/resources | Basic Canvas primitives, text and native-resolution buffers | Add Timeline, image resources, font/density capabilities and deferred input support (6) |
| Table/Tree/adapters/interop | Missing | Choose portable value/identity and borrowing contracts before adapting LVGL models or building custom widgets (6) |
| Validation | Independent headers, C++11, lifecycle and cookbook CTest; sanitizer/Valgrind runs | Expand native dispatch/virtual/input cases, real SDL2/X11 interaction, multiple resolutions and target-device validation (1–6) |

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

### Phase 1 — Existing backend lifetime and reconciliation

Reference: [PUI virtual-node cases](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/virtual_nodes.py),
[nested views](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/nested_view.py),
and [LVGL object lifetime](https://github.com/lvgl/lvgl/blob/80ca777e37a2b176770726a02e07a6fb79ef0b39/src/core/lv_obj_tree.c).

- [ ] Make standalone LVGL View destruction release its owned declarations;
  verify both standalone and Application-owned roots, without deleting borrowed
  screens or double-deleting native descendants.
- [ ] Audit raw native storage and secondary handles such as a button's label.
  Add native-delete invalidation with callback contexts whose own lifetime is
  explicit. Teardown after external deletion must be safe; remount semantics
  remain blocked by the external-ownership decision.
- [ ] Add native tests for keyed/nested virtual moves, empty subviews, independent
  redraw, repeated removal/reinsertion and top-level builder failure recovery.
- [ ] Test callback replacement, self-removal, callback-triggered redraw and
  capture reclamation. Verify CPPUI disconnects only its own event callbacks.

Acceptance: stable native identity/order, current dispatch and exact declaration
reclamation under allocation counts, ASan/UBSan/leak checks and Valgrind. Keep
nested partial-sync recovery tied to the shared-core decision.

### Phase 2 — Runtime, SDL2/X11 and input

Reference: [LVGL SDL event loop](https://github.com/lvgl/lvgl/blob/80ca777e37a2b176770726a02e07a6fb79ef0b39/src/drivers/sdl/lv_sdl_window.c),
[keyboard driver](https://github.com/lvgl/lvgl/blob/80ca777e37a2b176770726a02e07a6fb79ef0b39/src/drivers/sdl/lv_sdl_keyboard.c),
and [X11 driver](https://github.com/lvgl/lvgl/blob/80ca777e37a2b176770726a02e07a6fb79ef0b39/src/drivers/x11/lv_x11_display.c).

The X11 cookbook supplies the driver's optional cursor image on LVGL's system
layer, independently of recipe content, and a sleeping delay callback. It builds
a corrected copy of the v9.6.0 display driver to stop
and join its tick thread during programmatic deletion, with an atomic stop flag.
Review this version-specific workaround when updating LVGL. It does not resolve
the application-stop and external-deletion contracts below.

- [ ] After choosing the stop/host-loop contract, replace the unbounded POSIX
  sleep loop with an application loop that can stop and allow ordered teardown.
  Define tick, delay, idle and host-pump responsibilities.
- [ ] Intercept/route native close before LVGL deletes the borrowed display or
  deinitializes itself. Test pending work, repeated quit and teardown from an
  active callback; verify the host's runtime survives borrowed use.
- [ ] Specify focus-group and keyboard/encoder ownership. Exercise SDL2/X11 pointer,
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

- [ ] Apply the agreed common layout/style options to LVGL, with explicit
  omitted/reset behavior, font-resource ownership and supported units.
- [ ] Add public Divider, Grid and Stack; verify nested padding, grid spans and
  overlapping child order. Add fixed-size spacing and configurable weights
  after the shared layout options are defined.
- [ ] Add public clickable labels and agreed input payloads. Retained widgets
  must use current handlers and restore properties when modifiers are removed.
- [ ] Migrate cookbook presentation helpers onto these public components once
  parity is verified, preserving the gallery and recipe geometry.

### Phase 4 — Text, progress, images and bound controls

- [ ] Add ProgressBar using LVGL's bar after defining range/precision/busy-state
  mapping. Enable the shared `progressbar.cpp` recipe through the LVGL manifest.
- [ ] Add plain Text and wrapping/selection capabilities where supported.
  Choose rich-text/Markdown capability before enabling the complete `text.cpp`
  page; a plain label must not silently stand in for formatted text.
- [ ] Add Image with owned/borrowed source lifetime, decoding failure behavior,
  declared dimensions and replacement cleanup.
- [ ] Using the shared `Binding<T>`/State adapters, add TextField, Checkbox,
  RadioButton and ComboBox/item equivalents. Preserve in-progress edits, distinct
  labels/values, empty selections and current callbacks through redraw.

Acceptance: port [progressbar.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/progressbar.py),
[text.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/text.py),
[image.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/image.py)
and the binding/control pages. Assert one commit per edit, no feedback loop,
native reuse, valid model retention after invalid input, and resource reclamation.

### Phase 5 — Containers and window interactions

- [ ] Add public Scroll after the shared start/end anchor contract is settled;
  preserve position through resize, growth, shrinkage and content replacement.
- [ ] Add Tabs/Tab using LVGL tabview with [PUI's mapping](../REFERENCE.md#tabs): each
  physical child under a virtual Tab becomes a page with its ancestor's label;
  no implicit grouping or discarded children. Retain selection on keyed moves
  and document the reference's label-update quirk.
- [ ] Decide a custom resizable-pane implementation versus unsupported Splitter.
  Do not duplicate the shared splitter recipe to hide capability differences.
- [ ] Agree modal overlays, dialogs, menus/toolbars and Window capabilities
  suitable for LVGL before implementing them. Desktop native dialogs, MDI,
  window-manager flags and Qt embedding have no automatic LVGL equivalent.

Acceptance: port [scroll.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/scroll.py),
[tab.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/tab.py),
[splitter.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/splitter.py)
and agreed modal/dialog cases, or report the omitted capability explicitly.

### Phase 6 — Timers, drawing, data widgets and embedding

- [ ] Add cancellable TimelineView timers after scheduling and retained-view
  input contracts are agreed; no redraw after removal/shutdown.
- [ ] Add LVGL ImageResource/loadImage/drawImage after agreeing source ownership,
  decoder availability and file access; test crop/scale, source regions, opacity,
  replacement and missing images.
- [ ] Define Canvas font loading and display-density scaling. Basic text scales
  the object's bitmap font to a pixel line height; Qt uses font points. Verify
  device rendering/antialiasing expectations before promising equivalent output.
  Pointer events remain tied to the explicitly deferred shared input contract;
  public size/background modifiers follow the shared layout/style contract.
- [ ] Define Table/Tree/adapters against the shared value/identity contracts;
  investigate custom tree behavior separately from LVGL's table widget.
- [ ] Define native-object/display embedding and external owner deletion before
  exposing interop. Support shared-screen content only under an explicit scope.

Acceptance: use the reference [timeline regression](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/timeline_regression.py),
[Canvas](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/canvas.py),
[ragged table](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/table_ragged.py)
and [tree sync](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/tree_sync.py)
cases where the agreed LVGL capability supports them. Matplotlib/Shapely bridges
remain deferred with the Qt plan.

## Decisions awaiting direction

- **Shared contracts:** use the shared Binding/State adapters and follow the Qt plan's
  default reset for omitted properties, stderr render errors, Optional dialog
  results and PUI virtual Tab mapping. Input payloads are explicitly deferred;
  remaining subscription/value types, retained-view inputs and nested-sync
  recovery stay coordinated there. Inspect reference code/tests before raising
  any further decision; do not block independent work on those gaps.
- **Runtime ownership and stop:** the existing Application loops forever while
  LVGL's SDL2/X11 drivers may destroy displays or call `exit`. Choose host-owned versus
  backend-owned pumping and native-event interception before implementing quit.
  Native resources must outlive declarations in both modes.
- **Screen ownership:** active-screen content is currently exclusive. Choose
  scoped content roots, screen switching, multiple displays and external-delete
  remount behavior before broadening Window or interop.
- **Capability differences:** decide progress busy/range behavior, rich text and
  selection, font/image resources, custom splitters/trees, and desktop features
  independently. Unsupported must remain visible; pixel-identical desktop
  rendering is not implied by sharing recipe source.

## Shared cookbook and verification

Keep `cookbook/cookbook.cpp` and recipe sources shared. Select SDL2 or X11 in
`cookbook/lvgl_sdl2/` or `cookbook/lvgl_x11/`, with shared LVGL sources, headers
and build logic in `cookbook/lvgl/`; keep native APIs and conditional
compilation out of recipe files. Preserve the Example sidebar, scrolling list,
Result pane and recipe layouts. Only the reference's source-code pane is omitted;
any toolkit capability limitation must remain explicit in `--list` and docs.

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
change summary to `Changelog.txt`; then commit before the next change. Keep
detailed validation commands/results in tests, documentation or commit messages. Update the Qt plan only for shared contracts
or shared-core gaps, keeping LVGL-specific remaining work here.

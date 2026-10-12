# LVGL implementation progress and remaining work

## Baseline and scope

Plan prepared on 2026-10-12 against:

- [CPPUI at 5befe3353d7f372dee4418cd439b833d5f15f93a](https://github.com/buganini/CPPUI/commit/5befe3353d7f372dee4418cd439b833d5f15f93a).
- [PUI at 009e9b9c3fbd724f0b73c862f8213ddc4742405c](https://github.com/buganini/PUI/tree/009e9b9c3fbd724f0b73c862f8213ddc4742405c),
  for shared reactive behavior and [cookbook examples](https://github.com/buganini/PUI/tree/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook).
- [LVGL 9.3.0 at c033a98afddd65aaafeebea625382a94020fe4a7](https://github.com/lvgl/lvgl/tree/c033a98afddd65aaafeebea625382a94020fe4a7),
  for native objects, events, widgets, displays, and SDL integration.

The pinned PUI tree has no LVGL backend. This is a plan for CPPUI's existing
`cppui/lvgl/` implementation, not a claim of direct Python/LVGL parity. Preserve
shared C++11 declarations and recipes while mapping behavior explicitly onto
LVGL. Desktop-only features need a stated LVGL capability or omission.

Coordinate shared-core work and public binding/layout/event contracts with the
[Qt alignment plan](pyside6-progress.md). Do not create competing backend-specific
public APIs to work around an unresolved common contract. Record blockers below
and continue independent tasks. Keep this document limited to remaining work
and current contracts; completed behavior and verification belong in
`Changelog.txt`.

## Current basis and remaining acceptance

Public LVGL headers currently cover Application, Window, HBox/VBox, Label and
Button. The shared cookbook additionally has private presentation helpers for
spacing, weights, styles, scrolling, dividers and page links. Those helpers do
not establish public component support.

| Area | Current basis | Remaining acceptance / phase |
| --- | --- | --- |
| Core integration | Shared state/reconciliation, common retirement updates | Nested virtual views retain native order and state; direct view destruction and failed builds reclaim declarations (1) |
| Native lifetime | Application owns declarations; Window borrows the active screen | Deletion notifications invalidate observers; active callbacks, parent deletion and screen replacement cannot access retired nodes (1) |
| Application and SDL | Timer loop; headless and SDL startup/smoke teardown | Define stop/event ownership, then close gracefully before native display destruction; support host-driven pumping without POSIX-only assumptions (2) |
| HBox/VBox | Basic flex flow and child sizing | Common padding/dimensions/weights reset correctly, including zero; nested and virtual geometry remains stable (3) |
| Label/Button | Owned text and current button callbacks | Public label clicks, event payloads and style reset follow common contracts; repeated callbacks/removal release captures (1, 3) |
| Spacer/Divider/Grid/Stack | Cookbook-only spacer/divider; no public components | Define shared placement/style API, then verify orientation, spans, overlap, geometry and native reuse (3) |
| ProgressBar/Text/Image | Missing public components | Numeric range conversion, wrapping, source replacement and resource lifetime are explicit; rich-text gaps stay visible (4) |
| Bound controls | Missing | Text edits and boolean/value selections commit once, rebind correctly and avoid feedback loops (4) |
| Scroll/Tabs/Tab/Splitter | Cookbook-only scrolling; other public components missing | Preserve offsets/selection across content changes; apply PUI's virtual Tab mapping and decide splitter capability (5) |
| Modal/dialog/menu/window features | Active-screen Window only | Agree LVGL overlays and desktop-only omissions; use shared Optional results and preserve borrowed-screen ownership (5) |
| Timeline/Canvas/resources | Missing | Cancel timers and paint work on removal; use current callbacks; handle resource ownership, coordinates and input correctly (6) |
| Table/Tree/adapters/interop | Missing | Choose portable value/identity and borrowing contracts before adapting LVGL models or building custom widgets (6) |
| Validation | Independent headers, C++11, lifecycle and cookbook CTest; sanitizer/Valgrind runs | Expand native dispatch/virtual/input cases, real SDL interaction, multiple resolutions and target-device validation (1–6) |

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
and [LVGL object lifetime](https://github.com/lvgl/lvgl/blob/c033a98afddd65aaafeebea625382a94020fe4a7/src/core/lv_obj_tree.c).

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

### Phase 2 — Runtime, SDL and input

Reference: [LVGL SDL event loop](https://github.com/lvgl/lvgl/blob/c033a98afddd65aaafeebea625382a94020fe4a7/src/drivers/sdl/lv_sdl_window.c),
[keyboard driver](https://github.com/lvgl/lvgl/blob/c033a98afddd65aaafeebea625382a94020fe4a7/src/drivers/sdl/lv_sdl_keyboard.c).

- [ ] After choosing the stop/host-loop contract, replace the unbounded POSIX
  sleep loop with an application loop that can stop and allow ordered teardown.
  Define tick, delay, idle and host-pump responsibilities.
- [ ] Intercept/route native close before LVGL deletes the borrowed display or
  deinitializes itself. Test pending work, repeated quit and teardown from an
  active callback; verify the host's runtime survives borrowed use.
- [ ] Specify focus-group and keyboard/encoder ownership. Exercise SDL pointer,
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
- [ ] Add public Spacer, Divider, Grid and Stack; verify fixed/expanding sizes,
  nested padding, grid spans and overlapping child order.
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
- [ ] Using the agreed `Binding<T>`/State adapters, add TextField, Checkbox,
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
- [ ] Map the Qt CanvasPainter/ImageResource drawing API onto LVGL, with explicit
  native resource ownership and capability gaps. Test transforms, source
  regions, density and queued paint during removal. Pointer events remain tied
  to the explicitly deferred shared input contract.
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

- **Shared contracts:** follow the Qt plan's accepted Binding/State adapters,
  default reset for omitted properties, stderr render errors, Optional dialog
  results and PUI virtual Tab mapping. Input payloads are explicitly deferred;
  remaining subscription/value types, retained-view inputs and nested-sync
  recovery stay coordinated there. Inspect reference code/tests before raising
  any further decision; do not block independent work on those gaps.
- **Runtime ownership and stop:** the existing Application loops forever while
  LVGL's SDL driver may destroy displays and call `exit`. Choose host-owned versus
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

Keep `cookbook/cookbook.cpp` and recipe sources shared. Select LVGL sources,
headers and dependencies in `cookbook/lvgl/`; keep native APIs and conditional
compilation out of recipe files. Preserve the Example sidebar, scrolling list,
Result pane and recipe layouts. Only the reference's source-code pane is omitted;
any toolkit capability limitation must remain explicit in `--list` and docs.

Headless C++11 build and tests:

```sh
cmake -S cookbook/lvgl -B build-cookbook-lvgl \
  -DCPPUI_LVGL_SOURCE_DIR=/path/to/lvgl -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-cookbook-lvgl
ctest --test-dir build-cookbook-lvgl --output-on-failure
```

Enable `CPPUI_LVGL_SDL=ON` with SDL2 development files to add SDL driver tests;
run the executable without arguments for the interactive window. CTest uses
dummy video/software rendering for `cookbook.lvgl_sdl`. Keep interactive close,
physical input/focus and embedded-device checks pending until explicitly tested.

Enable `CPPUI_VALGRIND_TESTS=ON` in a non-sanitized build and run
`ctest --test-dir build-cookbook-lvgl -L valgrind --output-on-failure`. New cases
must use `cppui_add_test` so memory checks cover them. Run ASan/UBSan/leak checks
in a separate build with both the LVGL C library and CPPUI C++ code instrumented.
Keep per-allocation libc tracking in the host test configuration. No invalid
accesses or definite/indirect/possible leaks are acceptable; reachable library
globals are reported separately. Do not hide framework leaks with suppressions.

After each completed change, update component/build docs, examples and tests;
remove its finished tasks from this plan; record validation in `Changelog.txt`;
then commit before the next change. Update the Qt plan only for shared contracts
or shared-core gaps, keeping LVGL-specific remaining work here.

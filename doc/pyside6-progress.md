# Qt implementation alignment with PUI/PySide6

## Baseline and scope

Plan prepared on 2026-10-12 from these repository baselines:

- CPPUI: [848cf9a97c45ae6ca0050ab3fc645daf15009665](https://github.com/buganini/CPPUI/commit/848cf9a97c45ae6ca0050ab3fc645daf15009665).
- Reference: [PUI](https://github.com/buganini/PUI), commit
  [009e9b9c3fbd724f0b73c862f8213ddc4742405c](https://github.com/buganini/PUI/commit/009e9b9c3fbd724f0b73c862f8213ddc4742405c).
- The inspected PUI shared core, PySide6 backend, tests, and cookbook have no
  local changes. Unrelated local Flet changes are outside this baseline.

Align CPPUI's native Qt 6 implementation in `cppui/qt/` with the observable
behavior of `PUI/PySide6` and its shared reactive core. The filename identifies
the reference backend; implementation remains C++. Preserve the existing
declarative construction style and C++11 compatibility for the shared core. The
Qt backend follows Qt 6's imported-target compiler requirements. Shared-core
changes must account for the existing wx and LVGL implementations; expanding
those backends is outside scope.

Use reference implementation and regression tests together to establish each
contract. Where documentation, implementation, and tests disagree, record the
case and chosen behavior before implementing it. Python decorators, dynamic
attributes, and garbage collection need C++ equivalents, not literal ports.
Pin any subsequently adopted reference changes by component and commit here.

### C++ ownership and reclamation requirements

C++ does not provide Python's automatic reference counting and cyclic garbage
collection. Matching PUI behavior therefore requires an explicit ownership and
reclamation design, in addition to the UI port. Qt parent ownership and
`deleteLater()` manage native objects; they do not manage CPPUI declarations,
state subscriptions, or captured callback data.

- Give every declaration, native object, subscription, timer, and callback an
  explicit owner. Use RAII for owned resources and distinguish borrowed native
  objects from those CPPUI may destroy.
- Treat parent links, retired-node forwarding, listeners, and callback captures
  as potential dangling references or ownership cycles. Keep back-references
  non-owning and invalidate them before their targets are reclaimed. Do not add
  shared/weak ownership bookkeeping to factory return values.
- Separate logical retirement from physical reclamation. Callbacks currently
  executing and queued redraws, timers, paints, or signals must either retain
  valid dispatch state or resolve an invalidatable handle. Following a pointer
  to an already deleted declaration is never a reclamation strategy.
- Cancel or invalidate pending work, detach subscriptions/connections, and
  release owned subtrees in a defined shutdown order. Account for native
  deferred deletion before the Qt event loop becomes unavailable.
- Require repeated redraw/reorder/remove cycles to release retired declarations
  and captures without unbounded retention. Validate both invalid accesses and
  leaks; absence of a crash alone does not establish safe reclamation.

### Borrowed return values and explicit native lifetimes

Keep the existing borrowed node return values for now. Defer public weak-reference
wrappers and `.ref()` APIs to avoid extra bookkeeping. Prefer explicit ownership
and typed raw pointers for internal `.ui` storage; a weak-pointer wrapper is not
required when the framework can enforce the full lifecycle.

| Reference | Representation | Lifetime contract |
| --- | --- | --- |
| Declaration | Owned by the builder/tree | Ownership is transferred or released explicitly during reconciliation |
| Returned node reference | Existing borrowed `Node&` or derived reference | Use for immediate fluent modifiers and construction scopes; do not retain across reconciliation |
| Internal Qt object reference | Typed `QWidget*`, `QLayout*`, etc. in the Qt backend | Non-owning access under the native owner's lifetime and teardown rules |
| Non-QObject native resource | Explicit owning/borrowed resource type | Separate handling for resources such as `QSpacerItem` |

Application code keeps state and stable IDs, rather than references to nodes
created in `content()`. The root Application and application-owned State objects
retain their normal ownership. Imperative operations on mounted nodes must use
the current node supplied by framework dispatch or a future scoped lookup; a
retained factory reference cannot safely reach it through `getNode()`.

Enforce these internal lifecycle rules:

1. Specify one owner for each native object: a Qt parent or an explicit owner.
   The declaration's `.ui` pointer is an observer, not a second deleting owner.
2. On reuse, transfer native access and rebind event dispatch to the current
   declaration before clearing the retired declaration's `.ui` and deleting it.
3. On removal, mark the subtree inactive, detach subscriptions/connections, and
   cancel or invalidate pending work before reclaiming declarations. Clear native
   access throughout the affected subtree before deleting its native owner or
   scheduling `deleteLater()`. Dispatch remains disabled during deferred deletion.
4. Queued work must use framework-owned dispatch contexts or cancellable queue
   entries whose lifetimes are explicit. Disconnecting a signal alone is not a
   substitute for invalidating already queued work. Avoid callbacks that retain
   a raw pointer to a retired declaration.
5. Snapshot the callback before invoking user code and defer reentrant
   reconciliation/reclamation until active dispatch completes. Confine native
   pointer access and lifecycle changes to the GUI thread.
6. Account for Qt parent destruction, central-widget replacement, and native
   self-deletion. Borrowed interop objects require an owner-lifetime contract or
   destruction notification that clears access before the next use. If outside
   ownership cannot satisfy that contract, consider a targeted `QPointer` there.

Typed raw pointers avoid per-reference weak bookkeeping and fit native Qt APIs.
The tradeoff is that they do not automatically clear when a native owner destroys
an object: every supported destruction path must uphold the rules above. A
non-null pointer by itself is not evidence of a live object. `QPointer` provides
automatic clearing for QObject destruction, but would still require declaration
retirement and dispatch rules; it is an optional guard for external ownership,
not the default `.ui` representation. See
[Qt's QPointer contract](https://doc.qt.io/qt-6/qpointer.html).

Acceptance: immediate construction APIs remain usable without public weak handles.
Test native reuse, parent-driven destruction, content replacement, callback
self-removal, teardown with pending work, and borrowed-object destruction under
the documented interop contract. Sanitizers and allocation checks verify that
explicit teardown leaves neither stale accesses nor retained declarations.

## Current status

“Partial” means code exists, not that parity or runtime correctness is verified.
The checklist below contains remaining work only.

| Area | CPPUI baseline | Remaining alignment |
| --- | --- | --- |
| DOM reconciliation | Partial: type/id matching, keyed moves, virtual flattening, subtree removal, ordering/grid flags | Full identity contract, dispatch lifetime safety, ordering/grid coverage |
| Views | Partial: `View`, `QtView`, `SubView<T>()`, independent redraw | Queued/coalesced Qt redraw, setup lifecycle, safe builder scopes, parameterized views |
| State | Partial: `State<T>` read tracking, assignment, `+=`, `-=`, listener cleanup; both state/view destruction orders tested | Bindings, batching, mutation guards, collection changes |
| Application | Partial: creates `QApplication`, shows/closes children, starts loop | Safe application ownership, icon/options, queued quit |
| Window | Partial: `QMainWindow` and central widget | Title/icon/size, initial state flags, imperative state changes, keypress |
| HBox/VBox | Partial: insertion/removal and native reuse | Padding, dimensions, weights, Qt policies, correct native mounting |
| Label/Button | Partial: text and button click | Common modifiers, selectable/clickable labels, event contract, safe callback rebinding |
| Grid/Stack/Spacer/Divider | Missing native components | Core grid/order flags alone do not implement these widgets |
| TextField/Checkbox/RadioButton | Missing | Two-way editing and selection |
| ComboBox/ComboBoxItem/ProgressBar | Missing | Selection/value mapping and numeric progress |
| Text/Html/MarkDown/Image | Missing | Rich/selectable text and image presentation |
| Scroll/Tabs/Tab/Splitter | Missing | Container semantics and retained interaction state |
| Modal/imperative dialogs | Missing | Bound visibility, callbacks, results/cancellation |
| MenuBar/Menu/MenuAction/ToolBar/ToolBarAction/MDI | Missing | Actions, out-of-order children, native ownership |
| Canvas/ImageResource | Missing | Drawing, pointer events, HiDPI |
| Table/TableNode/Tree/TreeNode/adapters | Missing | Qt models, editing, stable identity |
| Qt interop/TimelineView | Missing | Borrowed widgets, embedding, cancellable timers |
| Validation | Core and offscreen Qt CTest harnesses, independent header builds, multi-file checks | Broader lifecycle, dispatch, and component parity coverage |

The six existing components are documented in [REFERENCE.md](https://github.com/buganini/CPPUI/blob/848cf9a97c45ae6ca0050ab3fc645daf15009665/REFERENCE.md).
The [virtual-node example](https://github.com/buganini/CPPUI/blob/848cf9a97c45ae6ca0050ab3fc645daf15009665/example/test_virtual_nodes/main.cpp) ports one PUI
scenario but runs an interactive event loop and contains no assertions.

## Implementation sequence

Complete phases 0–3 before adding most widgets. Phases 4–8 build on those
contracts; implement each component as a separate reviewable change with its
behavioral checks and documentation.

### Phase 0 — Contract and test harness (P0)

- [ ] Specify the C++ API shapes for owned IDs/text, `Binding<T>`, event payloads,
  layout/style options, and borrowed construction references. Document that
  application code must not retain those references across reconciliation. Keep
  existing no-argument button callbacks usable while adding event-aware overloads.
- [ ] Map every component in the status table to a milestone, reference example,
  and acceptance case. Track intentional extensions and omissions separately.

Acceptance: the remaining public API choices are documented and each planned
component has a reference example and a concrete acceptance case.

### Phase 1 — Node identity, ownership, and reconciliation (P0)

Reference: [node.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/PUI/node.py), [dom.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/PUI/dom.py),
[virtual_nodes.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/virtual_nodes.py), and
[subview_id.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/subview_id.py).

- [ ] Audit `cppui/dom.hpp`, `node*.hpp`, and Qt update methods together. Matching
  must account for the reference's view-relative identity, explicit IDs, grid
  coordinates/spans, and native-widget identity where applicable.
- [ ] Replace assumptions that Python-style retired-node links remain valid in
  C++. `sync()` currently deletes retired declarations; a retained pointer cannot
  safely follow `getNode()` afterward. Implement explicit internal dispatch and
  reclamation lifetimes for callbacks, timers, and imperative operations without
  introducing a public weak node wrapper.
- [ ] Ensure every backend update participates in the common retirement/transfer
  protocol. Transfer child ownership and parent links before releasing the old
  declaration; destroy native subtrees exactly once.
- [ ] Implement the ownership/reclamation requirements above, including cycle
  prevention, safe self-removal from callbacks, and application teardown. Define
  when retired declarations become reclaimable without retaining every prior
  render indefinitely.
- [ ] Give `QApplication`, borrowed widgets, and owned widgets distinct cleanup
  paths. `QtView::destroy()` currently assumes its `ui` is a `QWidget`.
- [ ] Own IDs and button text rather than retaining caller-provided `const char*`.
  Guard deferred deletion and callback delivery after removal. Retain individual
  signal connections instead of disconnecting unrelated application listeners.

Acceptance: keyed reorders reuse native objects; virtual siblings retain correct
offsets; empty/nested/removed subtrees reconcile correctly; root teardown and
deferred callbacks produce no use-after-free or double deletion under sanitizers.
Exercise terminal, out-of-order, reversed, and non-movable nodes explicitly.
Repeated redraw/removal and complete teardown release CPPUI-owned declarations,
subscriptions, and callback captures; use leak checks or allocation counters to
distinguish retained framework objects from Qt/process-global allocations.

### Phase 2 — Reactive state, bindings, and view scheduling (P0)

Reference: [state.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/PUI/state.py), [view.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/PUI/view.py),
[Qt base.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/PUI/PySide6/base.py), and
[application.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/PUI/PySide6/application.py).

- [ ] Separate immediate synchronization from redraw requests. Add dirty/updating
  guards, queue Qt redraws on the GUI thread, coalesce bursts, and ignore pending
  work for removed views. Define a safe way to await completion in tests.
- [ ] Add one-time setup and parameterized subview construction. Define when
  matching views retain local state and how changed inputs reach reused views.
- [ ] Make construction scopes exception-safe and safe across `break`/`return`.
  The current range iterator calls `exit()` only on increment. Restore both
  stacks and dispose of incomplete declarations after builder failure.
- [ ] Introduce typed readable/writable bindings, conversion/validation for text
  editors, change subscriptions, external getter/setter binding, and explicit
  notification. Specify subscription ownership and `State` copy/move behavior.
- [ ] Implement scoped batched mutation and reject mutation during view building.
  Suppress unchanged-value notifications where consistent with the reference;
  refresh dependencies and detach destroyed subscribers safely.
- [ ] Define observable list/map updates and element bindings corresponding to
  PUI's collection state. State explicitly how nested changes notify views;
  mutable container access must not silently bypass notification.
- [ ] Add application icon/options and queued `quit()`. Document application
  creation/reuse and the thread boundary for state updates.

Acceptance: port the scenarios in [tests/batched_state_mutation.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/batched_state_mutation.py),
[tests/set_state_in_view_builder.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/set_state_in_view_builder.py), [tests/redraw_semaphore.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/redraw_semaphore.py), and
[tests/nested_view.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/nested_view.py) into assertions. A redraw burst settles to the latest
state; a removed subscriber receives no work; invalid text leaves the model
valid; builder failure leaves the next render usable.

### Phase 3 — Common modifiers, layout, and existing widgets (P1)

Reference: [node modifiers](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/PUI/node.py),
[Qt base](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/PUI/PySide6/base.py),
[layout.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/PUI/PySide6/layout.py), and corresponding cookbook pages.

- [ ] Add layout width/height, weight, padding, margin, and grid placement APIs.
  Implement supported reference behavior and document parsed-but-unused options
  instead of claiming they work. Distinguish omitted values from zero.
- [ ] Apply generic colors/fonts and typed Qt options for size policies,
  constraints, stylesheet, and application style. Resolve naming conflicts with
  the existing `BaseLayout::layout` member before introducing `.layout(...)`.
- [ ] Align HBox/VBox margins, stretch updates, size hints, and nested layouts.
  Recompute properties from current declarations and define reset behavior.
- [ ] Add `Grid`, `Stack`, `Spacer`, and `Divider`, including grid spans,
  overlapping Stack order, and `QSpacerItem` ownership/non-movability.
- [ ] Extend Window with title, icon, size, keypress, initial-only maximize and
  fullscreen flags, and imperative methods to change those states.
- [ ] Extend Label with selectable text/clicks and Button with current callback
  dispatch and reference focus behavior. Add common event payloads and fluent
  modifiers needed by later widgets.

Acceptance: port HBox/VBox/Grid/Stack cookbook scenarios; change weights to zero,
dimensions, styles, and grid spans on a retained tree. Verify native geometry
and child order. Port the window-state and size-policy cases from
[pyside6_regression.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/pyside6_regression.py).

### Phase 4 — Bound controls and text/image components (P1)

- [ ] Add `TextField(model, edit_model)`: preserve in-progress edits through
  redraw, emit input/change at the correct times, validate commit, and keep
  programmatic updates from triggering user-edit callbacks.
- [ ] Add Checkbox boolean/collection membership binding and RadioButton value
  selection, including rebinding and current click callbacks.
- [ ] Add ComboBox/ComboBoxItem with editable text, index/text models, distinct
  labels/values, empty contents, invalid selection, and dynamic item updates.
- [ ] Add ProgressBar with the reference hundredths conversion, including
  fractional maximum (`0.25` becomes native `25`, `0.125` becomes `12`).
- [ ] Add Text, Html, MarkDown, and Image with selection, wrapping, dimensions,
  path updates, and common modifiers as supported by the pinned reference.

Acceptance: port the binding, textfield, checkbox, radiobutton, combobox,
progressbar, text, and image cookbook cases. Assert a single commit, no feedback
loops, current binding/callback after reuse, and negative ComboBox index handling.

### Phase 5 — Containers and window interactions (P1)

- [ ] Add Scroll with axis policies, safe single-content replacement, retained
  user position, start/end offsets, and `END`. Preserve the reference distinction
  between positive zero and negative-zero end anchoring in the C++ API.
- [ ] Add Tabs/virtual Tab and Splitter, with reactive labels, child insertion,
  removal, reordering, and native interaction state. Record `tabposition` as a
  reference gap: the pinned implementation stores it but does not apply it.
- [ ] Add Modal with bound status/off-value, open/close callbacks, native-close
  synchronization, and Window's initial/imperative state contract.
- [ ] Add OpenDirectory, OpenFile, OpenFiles, SaveFile, Information, Warning,
  Critical, Confirm, and Prompt. Define typed C++ results that preserve accepted
  values versus cancellation, including the reference's inconsistent empty
  directory string versus `None` for other cancelled file operations.

Acceptance: port [scroll_sync.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/scroll_sync.py); test content
growth/shrinkage and a resize during reconciliation. Exercise tab/splitter edits,
modal close/reopen/removal, dialog filters/defaults, and cancellation. Native
dialog interaction and window-manager state also require manual display checks.

### Phase 6 — Menus, toolbars, MDI, interop, and timers (P2)

- [ ] Add MenuBar/Menu/MenuAction and ToolBar/ToolBarAction, including nested
  menus, widget actions, icons, current trigger callbacks, and keyed reorders.
- [ ] Route these children through Window's native APIs without replacing its
  central content. Implement MdiArea/MdiSubWindow and safe child removal.
- [ ] Add equivalents of `QtInPui` and `PuiInQt`: specify borrowed native widget
  lifetime, removal/reparenting, and embedding into an existing Qt application.
- [ ] Add TimelineView with a cancellable Qt timer, stable current-node lookup,
  interval semantics, and no redraw after removal or shutdown.

Acceptance: actions fire once using current data after repeated redraws; menus
and toolbars can coexist with central content and modals; borrowed widgets
survive declaration removal. Port the lifecycle case in
[timeline_regression.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/timeline_regression.py) and adapt
[pyside6_interop.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/examples/pyside6_interop.py).

### Phase 7 — Canvas and input (P2)

- [ ] Add Canvas painter callbacks and drawing for text, lines, polylines, cubic
  Beziers, polygons, rectangles, ellipses, and images; support fill/stroke,
  transforms, text anchors, and image source regions/opacity.
- [ ] Add ImageResource loading, cropping, and scaling. Keep coordinates in
  logical pixels and update pixel density before each paint.
- [ ] Implement mouse down/up/move/double-click, enter/leave, wheel deltas and
  modifiers, plus Qt drag-enter/drop callbacks and their acceptance behavior.
- [ ] Ensure retained paint/event callbacks resolve the current declaration and
  become inert after destruction. Track Shapely integration separately below.

Acceptance: port Canvas and drop cookbook cases, compare rendered output at
multiple scale factors, and verify event payloads and callback replacement.
Painting queued before removal must not access deleted declarations.

### Phase 8 — Table and Tree models (P2)

- [ ] Add C++ table/tree adapter interfaces corresponding to
  [interfaces.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/PUI/interfaces.py), plus declarative TableNode and
  TreeNode forms. Choose typed data/identity representations before model code.
- [ ] Implement Table headers, explicit/inferred counts, ragged and empty rows,
  read-only/editable cells, display versus edit values, choice editors, autofit,
  and click/double-click callbacks. Preserve raw choice types and duplicate labels.
- [ ] Implement Tree model notifications, node moves, persistent-index remapping,
  expansion/collapse APIs and callbacks, selection, and Qt drag/drop behavior.
  Reconciliation must preserve native state for retained nodes.

Acceptance: port [table_ragged.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/table_ragged.py) and
[tree_sync.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/tree_sync.py), including immediate choice commit,
unknown-value placeholders, inert missing cells, and expansion after redraw.
Exercise both external adapters and declarative nodes using cookbook examples.

## Explicitly deferred or requiring a C++ design decision

- Python hot reload and decorator syntax: preserve reusable declarative views;
  runtime C++ code replacement is outside this alignment plan.
- MatplotlibCanvas and Canvas.drawShapely: keep optional Python/geometry bridges
  deferred until their dependency and ownership strategy is selected. Native
  Canvas must work without them. These remain documented parity gaps.
- Dynamic Qt keyword parameters: expose typed options and native interop;
  enumerate supported options rather than promising arbitrary Python attributes.
- Existing PUI gaps, such as unused tab position and inconsistently applied
  layout/style options: document observed behavior and any intentional CPPUI
  improvement. Do not infer complete support from a constructor argument alone.

## Validation and progress updates

After every completed change, remove its finished checklist items and stale
notes from this document. Keep this file focused on remaining work, current
contracts, and outstanding checks. Record the completed behavior, validation
commands/results, and tested versions in `Changelog.txt`, then commit before
starting the next change. Do not retain completed-work logs here.

For each component, check initial creation, unchanged redraw, property/binding/
callback changes, keyed reorder, removal, and destruction where applicable.
Use core assertions for reconciliation/state, Qt offscreen tests for native
objects/signals/models, and sanitizer runs for lifetime changes. Keep real-display
checks for focus, dialogs, native window states, and HiDPI. Shared-core changes
also need existing wx/LVGL build checks when their dependencies are available.
For reclamation changes, include AddressSanitizer/UndefinedBehaviorSanitizer and
LeakSanitizer where supported, plus deterministic destruction/allocation checks
for callback cycles, retired nodes, state/view destruction order, and shutdown.

Build the Qt examples and automated suite with CMake 3.24+ and Qt 6 development
packages:

```sh
cmake -S . -B build-qt -DCPPUI_BUILD_QT=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-qt
ctest --test-dir build-qt --output-on-failure
```

Omit `CPPUI_BUILD_QT` to test the C++11 core without a toolkit. CTest sets the Qt
offscreen platform automatically. The local temporary toolchain is under
`/tmp/cppui-tools/root/usr`; use that prefix for `CMAKE_PREFIX_PATH`, its
`lib/x86_64-linux-gnu` for `LD_LIBRARY_PATH`, and its
`lib/x86_64-linux-gnu/qt6/plugins` for `QT_PLUGIN_PATH`. LeakSanitizer execution
requires an environment without ptrace tracing.

Remove a task only after its acceptance checks pass. Keep CPPUI's README/REFERENCE
and relevant examples current, and retain outstanding manual checks or intentional
departures from PUI here. Core/widget parity requires all remaining phases to
pass, with optional integrations still listed as explicit gaps.

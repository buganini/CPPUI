# Qt implementation alignment with PUI/PySide6

## Baseline and scope

Plan prepared on 2026-10-12 from these repository baselines:

- CPPUI: [848cf9a97c45ae6ca0050ab3fc645daf15009665](https://github.com/buganini/CPPUI/commit/848cf9a97c45ae6ca0050ab3fc645daf15009665).
- Reference: [PUI](https://github.com/buganini/PUI), commit
  [009e9b9c3fbd724f0b73c862f8213ddc4742405c](https://github.com/buganini/PUI/commit/009e9b9c3fbd724f0b73c862f8213ddc4742405c).

Align CPPUI's native Qt 6 implementation in `cppui/qt/` with the observable
behavior of `PUI/PySide6` and its shared reactive core. The filename identifies
the reference backend; implementation remains C++. Preserve the existing
declarative construction style and C++11 compatibility for the shared core. The
Qt backend follows Qt 6's imported-target compiler requirements. Shared-core
changes must account for the existing wx and LVGL implementations; expanding
those backends is outside scope except for the shared cookbook build and
compatibility fixes needed to run its existing components.

Track LVGL-specific component/runtime work in the [LVGL plan](lvgl-progress.md).
Shared-core and public API decisions remain coordinated here.

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

Each row maps remaining work to its implementation phase and a pinned reference
case. Acceptance involving undecided APIs applies after that contract is chosen;
the separate deferred section records intentional omissions.

| Area | Current basis | Remaining acceptance | Phase / reference case |
| --- | --- | --- | --- |
| DOM reconciliation | Scoped identity, native moves, virtual flattening and reclamation | External native deletion/remount respects identity; queued dispatch cannot reach reclaimed nodes | 1 / [virtual nodes][ref-virtual] |
| Views | `QtView` queued/coalesced redraw, immediate sync and one-time setup | Changed retained-view inputs arrive; a failed nested render can recover | 2 / [nested views][ref-nested], [redraw semaphore][ref-redraw] |
| State | Read tracking, assignment, `+=`, `-=`, listener cleanup and Binding adapters | Change subscriptions and explicit notification; a batch renders its final value; nested collection edits notify subscribers | 2 / [binding][ref-binding], [batched mutation][ref-batch] |
| Application | Owned/borrowed `QApplication`, queued quit | Apply and reset declared icon/options without changing borrowed native ownership | 2 / [gallery startup][ref-gallery] |
| Window | `QMainWindow` and central widget | Update title/icon/size; initial flags apply only on creation; imperative state changes and keypress payloads work | 3 / [Qt regression][ref-qt] |
| HBox/VBox | Insertion/removal, native reuse, default padding | Reset padding/dimensions/weights, including zero; apply copied Qt size policies | 3 / [HBox][ref-hbox], [VBox][ref-vbox], [Qt regression][ref-qt] |
| Label/Button | Owned text, selectable labels and current click callbacks | Styles reset on reuse; event payloads follow the chosen contract | 3 / [Label][ref-label], [Button][ref-button] |
| Grid/Stack/Spacer | Public box Spacer; core grid/reverse-order flags | Define shared placement options, then port Grid spans and Stack overlap | 3 / [Grid][ref-grid], [Stack][ref-stack], [HBox][ref-hbox] |
| TextField/Checkbox/RadioButton | Boolean Checkbox with Binding/State and membership adapters | Preserve unfinished edits; validate commit; define observable collection and typed value selections | 4 / [TextField][ref-textfield], [Checkbox][ref-checkbox], [RadioButton][ref-radio] |
| ComboBox/ComboBoxItem | Missing | Preserve distinct labels/values across item updates; handle empty contents and negative selection | 4 / [ComboBox][ref-combo], [Qt regression][ref-qt] |
| Image | Missing | Update source and declared dimensions while preserving native identity | 4 / [Image][ref-image] |
| Scroll | Missing | Preserve scroll anchor through insertion/reorder/removal and resizing | 5 / [Scroll][ref-scroll], [scroll sync][ref-scroll-sync] |
| MenuBar/Menu/MenuAction/ToolBar/ToolBarAction/MDI | Missing | Actions fire once with current data; keyed moves retain native objects; menus/toolbars/MDI coexist with central content and release ownership correctly | 6 / [gallery menus][ref-gallery], [Window routing][ref-window], [MDI][ref-mdi] |
| Canvas input | Native drawing and image resources | Add input/drop callbacks with safe replacement/removal; verify rendering on a real HiDPI display | 7 / [Canvas][ref-canvas], [Drop][ref-drop] |
| Table/TableNode/Tree/TreeNode/adapters | Missing | Ragged cells stay inert; typed choices commit once; tree moves retain selection, expansion and persistent indexes | 8 / [Table][ref-table], [Table adapter][ref-table-adapter], [Tree][ref-tree], [Tree adapter][ref-tree-adapter] |
| Qt interop/TimelineView | Missing | Borrowed widgets survive removal according to their ownership contract; timers stop on removal/shutdown | 6 / [embedding example][ref-interop], [Widget][ref-widget], [Timeline][ref-timeline] |
| Validation | Core/native CTest, headers, sanitizer and Valgrind harnesses | Port each remaining case above; exercise wx lifecycle with its dependencies and manual focus/window/HiDPI behavior on a display | 0–8 / [Qt regression][ref-qt], [wx regression][ref-wx] |

[ref-virtual]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/virtual_nodes.py
[ref-nested]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/nested_view.py
[ref-redraw]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/redraw_semaphore.py
[ref-batch]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/batched_state_mutation.py
[ref-qt]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/pyside6_regression.py
[ref-wx]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/wx_regression.py
[ref-scroll-sync]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/scroll_sync.py
[ref-interop]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/examples/pyside6_interop.py
[ref-window]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/PUI/PySide6/window.py
[ref-gallery]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/__main__.py
[ref-binding]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/binding.py
[ref-hbox]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/hbox.py
[ref-vbox]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/vbox.py
[ref-label]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/label.py
[ref-button]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/button.py
[ref-grid]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/grid.py
[ref-stack]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/stack.py
[ref-textfield]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/textfield.py
[ref-checkbox]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/checkbox.py
[ref-radio]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/radiobutton.py
[ref-combo]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/combobox.py
[ref-image]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/image.py
[ref-scroll]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/scroll.py
[ref-mdi]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/mdi.py
[ref-canvas]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/canvas.py
[ref-drop]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/drop.py
[ref-table]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/table.py
[ref-table-adapter]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/table_adapter.py
[ref-tree]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/tree.py
[ref-tree-adapter]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/tree_adapter.py
[ref-widget]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/widget.py
[ref-timeline]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/timeline.py

## Implementation sequence

Work through the available tasks below. Inspect the pinned implementation and
tests before raising a decision: follow reference behavior where it settles the
question. When a C++ API or lifecycle choice remains genuinely unresolved,
record its context under design decisions and continue independent work.

Implement each component as a separate reviewable change with its behavioral
checks and documentation. Respect its actual dependencies; unfinished work in
phases 0–3 does not block independent components in later phases.

### Agreed contracts for remaining work

- Bound controls use `Binding<T>` adapters, with `State<T>` accepted through an
  adapter. Adapters own callables and borrow the referenced model; external
  getter/setter storage needs explicit redraw unless the getter reads State.
  Follow PUI's binding behavior where it transfers to C++.
- Removing a previously specified layout/style property restores its default
  on redraw. An explicit zero remains distinct from an omitted property.
- Catch queued render exceptions inside the GUI callback and report them to
  stderr. Preserve the previous UI when the failure precedes native
  reconciliation; recovery after partial native updates is separate work.
- Portable input event objects are explicitly deferred. Existing no-argument
  callbacks remain usable.

### Phase 0 — Contract and test harness (P0)

- [ ] Settle concrete layout/style option types against reference behavior.
  Keep event-aware overloads deferred without blocking components that use
  existing callbacks.

Acceptance: the remaining public API choices are documented and each planned
component has a reference example and a concrete acceptance case.

### Phase 1 — Node identity, ownership, and reconciliation (P0)

Reference: [node.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/PUI/node.py), [dom.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/PUI/dom.py),
[virtual_nodes.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/virtual_nodes.py), and
[subview_id.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/subview_id.py).

- [ ] Replace assumptions that Python-style retired-node links remain valid in
  C++. `sync()` currently deletes retired declarations; a retained pointer cannot
  safely follow `getNode()` afterward. Implement explicit internal dispatch and
  reclamation lifetimes for callbacks, timers, and imperative operations without
  introducing a public weak node wrapper.
- [ ] Validate wx native ownership transfer and teardown with its runtime
  dependencies available: child ownership/parent links must transfer before the
  old declaration is released, and native subtrees must be destroyed once.
- [ ] Extend ownership coverage to queued callbacks/timers and externally owned
  or self-deleting native widgets. Ensure new event APIs cannot retain retired
  declarations indefinitely or introduce owning cycles.
- [ ] Specify borrowed-widget cleanup and native remount after external owner
  destruction, including the native-widget identity used for matching interop
  declarations. Clearing typed native access makes teardown safe, but a later
  redraw must explicitly remount the deleted native subtree before it is reusable.

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

- [ ] Add parameterized subview construction after defining how changed inputs
  reach matching views that retain local state.
- [ ] Recover from a nested builder failure after native reconciliation has
  already started. Choose transactional prebuilding versus explicit partial-sync
  recovery; top-level declaration rollback alone cannot undo native updates.
- [ ] Extend `Binding<T>` with conversion/validation helpers for text editors,
  change subscriptions and explicit notification of external getter/setter
  storage. Specify subscription ownership and `State` copy/move behavior.
- [ ] Implement scoped batched mutation and suppress unchanged-value
  notifications where consistent with the reference. Choose comparison behavior
  for State values without equality and exception delivery when a batch flushes.
- [ ] Define observable list/map updates and element bindings corresponding to
  PUI's collection state. State explicitly how nested changes notify views;
  mutable container access must not silently bypass notification.
- [ ] Add application icon/options after the common options API is chosen.

Acceptance: port the scenarios in [tests/batched_state_mutation.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/batched_state_mutation.py),
[tests/redraw_semaphore.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/redraw_semaphore.py), and
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
  Once the public API is chosen, migrate the cookbook's private presentation
  adapters onto it while preserving the reference layout and recipe styling.
- [ ] Align configurable HBox/VBox padding, stretch updates, and size hints.
  Recompute properties from current declarations and reset omitted properties
  to their defaults.
- [ ] Add `Grid` and `Stack`, including grid spans and overlapping Stack order.
- [ ] Extend Window with title, icon, size, keypress, initial-only maximize and
  fullscreen flags, and imperative methods to change those states.
- [ ] Add common event payloads and fluent modifiers needed by later widgets.

Acceptance: port HBox/VBox/Grid/Stack cookbook scenarios; change weights to zero,
dimensions, styles, and grid spans on a retained tree. Verify native geometry
and child order. Port the window-state and size-policy cases from
[pyside6_regression.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/pyside6_regression.py).

### Phase 4 — Bound controls and text/image components (P1)

- [ ] Add `TextField(model, edit_model)`: preserve in-progress edits through
  redraw, emit input/change at the correct times, validate commit, and keep
  programmatic updates from triggering user-edit callbacks.
- [ ] Add RadioButton value selection, including rebinding and current click
  callbacks. Decide automatic collection observation and value-based Checkbox
  overloads; explicit boolean membership Binding adapters are supported.
- [ ] Add ComboBox/ComboBoxItem with editable text, index/text models, distinct
  labels/values, empty contents, invalid selection, and dynamic item updates.
- [ ] Add Image with dimensions, path updates, and common modifiers as supported
  by the pinned reference.

Acceptance: port the binding, textfield, checkbox, radiobutton, combobox,
progressbar, text, and image cookbook cases. Assert a single commit, no feedback
loops, current binding/callback after reuse, and negative ComboBox index handling.

### Phase 5 — Containers and window interactions (P1)

- [ ] Add Scroll with axis policies, safe single-content replacement, retained
  user position, start/end offsets, and `END`. Preserve the reference distinction
  between positive zero and negative-zero end anchoring in the C++ API.
- [ ] Verify native desktop file dialogs and window-manager interaction on a
  real display, including Modal focus/flags; offscreen widget-dialog tests do
  not establish platform parity.

Acceptance: port [scroll_sync.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/scroll_sync.py); test content
growth/shrinkage and a resize during reconciliation. Native dialog interaction,
Modal focus and window-manager state require manual display checks.

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

- [ ] Implement mouse down/up/move/double-click, enter/leave, wheel deltas and
  modifiers, plus Qt drag-enter/drop callbacks and their acceptance behavior
  after the deferred input-event contract is addressed.
- [ ] Ensure input callback replacement/removal cannot dispatch to a retired
  declaration. Track Shapely integration separately below.
- [ ] Verify Canvas rendering and logical coordinates on a real HiDPI display.

Acceptance: port the drop cookbook case and verify event payloads, acceptance,
callback replacement and removal. Retain automated painting/lifetime coverage
while validating physical-display rendering separately.

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

## Shared C++ cookbook

Port the [PUI cookbook](https://github.com/buganini/PUI/tree/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook)
as shared C++ recipe sources. Qt is CPPUI's native backend corresponding to the
PySide6 reference; Python and C++ cannot share the same recipe source directly.
Keep the shared main UI in `cookbook/cookbook.cpp`. Use separate Qt and LVGL
`CMakeLists.txt` files (build commands in `README.md`)
to select backend headers,
capabilities, startup, and toolkit dependencies. Keep toolkit includes and
conditional compilation out of the recipes. Each executable links one backend.
Do not duplicate recipes or silently present unsupported behavior as a full port.

- [ ] Port each recipe alongside its components, preserving the state changes
  and interactions. The table below tracks only outstanding recipe work.
- [ ] Verify real-display layout/input on both backends; Qt offscreen and SDL
  dummy-driver recipe tests do not establish physical input or focus parity.

Preserve the reference gallery's Example sidebar, scrolling page list, Result
pane, Qt menus, and recipe layout. The source-code pane is intentionally omitted
at the user's request; no replacement navigation or extra status banner belongs
in the gallery. Keep backend capability details in `--list` and documentation.

| Reference recipe (`cookbook/`) | Prerequisite phase | Acceptance / remaining work |
| --- | --- | --- |
| `grid.py`, `stack.py` | 3 | Grid placement, overlap |
| `binding.py`, `textfield.py` | 2, 4 | External-change notification, changed callback, edit/commit and conversion |
| `radiobutton.py`, `combobox.py` | 2, 4 | Boolean/collection/value selections and dynamic items |
| `image.py` | 3, 4 | Sized image asset |
| `scroll.py` | 5 | Scroll offsets and retained native state |
| `mdi.py`, `widget.py`, `timeline.py` | 6 | MDI children, borrowed Qt widget, timer stops on removal |
| `drop.py` | 7 | Event payloads and accepted/rejected drops |
| `table.py`, `table_adapter.py`, `tree.py`, `tree_adapter.py` | 8 | Declarative/adapted data, editing, identity and expansion |
| `drawshapely.py`, `matplotlib.py` | Deferred integrations | Requires an agreed C++ bridge/dependency strategy |

## Decisions awaiting direction

Continue independent implementation while these contracts remain undecided.

- **Binding ownership:** extend the adapter for PUI's conversion/change/emit
  behavior. Document C++ subscription lifetimes and State copy/move semantics;
  ask only if tracing the reference and existing C++ ownership rules leaves a
  public choice unresolved.
- **Batch completion:** queued redraw errors use stderr as agreed. A future
  batch-scope exit still needs defined exception behavior during stack unwinding.
- **Common options:** choose concrete C++ layout/style/Qt option types against
  reference behavior. Omitted properties reset as agreed. Event payloads remain
  explicitly deferred; they do not block drawing or existing callbacks.
- **Parameterized retained views:** reconciliation retains matching View objects
  and discards their replacement declarations. Constructor forwarding alone
  cannot deliver changed inputs to the retained view. Decide an explicit input
  update contract before claiming parameterized-view parity.
- **Nested reconciliation failures:** range scopes and a failing top-level
  `content()` can unwind before mounting. A new nested View is currently built
  during `sync()`, after earlier siblings may have changed. Atomic rollback needs
  prebuilding/matching work or a defined partial-sync recovery protocol; skip
  that extension until its failure contract is chosen.
- **External native ownership:** Qt declarations now clear their native access
  when a parent deletes the QObject, and application teardown is safe. Remounting
  after external deletion and borrowing interop widgets need an agreed ownership
  contract; do not claim arbitrary external native mutation is supported.
- **Specialized APIs:** scroll end anchoring (including negative zero),
  table/tree value and identity types, and external widget
  ownership need their own C++ contracts before the corresponding phase. Their
  context and acceptance cases are recorded in phases 5–8 above.

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
  improvement. Tab position and empty/multi-child label behavior are described
  in the [component reference](../REFERENCE.md#tabs). Do not infer complete
  support from a constructor argument alone.

## Validation and progress updates

Normal state notifications and view redraws must be silent. Keep framework
debug tracing opt-in; queued render failures are reported to stderr as agreed.

After every completed change, remove its finished checklist items and stale
notes from this document. Keep this file focused on remaining work, current
contracts, and outstanding checks. Add a concise component-prefixed change
summary to `Changelog.txt`, merging related work into milestone entries; then
commit before starting the next change. Keep detailed validation commands/results
in tests, documentation or commit messages.
Do not retain completed-work logs here.

For each component, check initial creation, unchanged redraw, property/binding/
callback changes, keyed reorder, removal, and destruction where applicable.
Use core assertions for reconciliation/state, Qt offscreen tests for native
objects/signals/models, and sanitizer runs for lifetime changes. Keep real-display
checks for focus, dialogs, native window states, and HiDPI. Shared-core changes
also need existing wx/LVGL build checks when their dependencies are available.
For reclamation changes, include AddressSanitizer/UndefinedBehaviorSanitizer and
LeakSanitizer where supported, Valgrind Memcheck, plus deterministic destruction/allocation checks
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
requires an environment without ptrace tracing. Enable `CPPUI_VALGRIND_TESTS`
in a separate non-sanitized build to run the regressions with Memcheck; use
`ctest -L valgrind --output-on-failure`. All new regression and cookbook smoke
targets must register through `cppui_add_test` so they receive leak checks too.

Remove a task only after its acceptance checks pass. Keep CPPUI's README/REFERENCE, shared cookbook,
examples, and tests current, and retain outstanding manual checks or intentional
departures from PUI here. Core/widget parity requires all remaining phases to
pass, with optional integrations still listed as explicit gaps.

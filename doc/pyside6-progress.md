# Qt implementation alignment with PUI/PySide6

The 2026-10-21 sync ports ComboBox candidate menus, `.autoexpand()`,
`.editModel()` and `.input()` from PUI through `0dc72e2`. Candidates follow
application declarations without a second native filter. Reactive edits retain
the editor/caret, and typed item values remain free text. See the
[sync audit and remaining platform checks](pui-sync-2026-10-21.md).

The 2026-10-20 sync adds `HVBox(minWidth)` and `VHBox(minHeight)`, retaining
native containers and editor state across direction changes. The shared Adaptive
recipe and cookbook shell follow committed local PUI `efda649`; see the
[sync audit](pui-sync-2026-10-20.md).

The 2026-10-19 sync adds frozen hidden subtrees, reactive content-size observation
and adaptive visibility bounds, with a shared Adaptive cookbook. It follows the
committed PUI `b4771b8` snapshot; see the [sync audit](pui-sync-2026-10-19.md).

PUI `97e442c` adds the backend name to the cookbook window title and sidebar
heading: `Cookbook (Qt)`.

The native sync through PUI `1c7b45c` adds the shared Binding log viewer and
editable Table cookbook markers, with Qt decorated Table validation, focus and
editor cleanup.
The shared DOM offset cache was adopted after native measurement; see the
[earlier audit](pui-sync-2026-10-18.md) and [performance decision](dom-offset-performance.md).

An earlier follow-up adds `.visible()`, delayed UI callbacks through `getPoster()`,
and typed tab placement with the shared `Side` enum. Hidden content retains native
state; callbacks run on the UI loop and stop executing after target removal. The
Qt tab recipe demonstrates top and bottom placement with bound selection.

Named node refs follow PUI `d83bbe2`: UI-thread lookup, retained binders and
queued selection commands are ported, with Qt/wx desktop window commands and
a shared Refs recipe. Worker submission, redraw and removal are tested. Native
State-ref inspection publishers (Web/AI tooling) remain excluded.

Canvas/HeavyCanvas now follow PUI `39759a2`/`e46cb43`: completed-draw FPS,
continuous painting and full-frame color changes. Overlay removal and repeat
painting are covered by native regressions.

Previous native delta audit: committed local PUI `efda649` (2026-10-20). See the
[component audit and exclusions](pui-sync-2026-10-20.md). Native Options,
optional Landlock, Canvas FPS and refs are ported; Qt/wx additionally include
portable inbound drops and directory filters. AI development tooling and
Web/Pyodide remain excluded. The earlier native baseline follows below.

Previous native delta audit: PUI `f0c1cf5` (2026-10-17). Shared controls use
`disabled` (`d7dc23a`) and HeavyCanvas includes fourteen presets through
1,000,000 (`18d1c5e`), with index labels above 500,000. Canvas styles are cached
within each frame (`8d3fa8d`/`4d77b90`); window state setters only store initial
flags (`77d996b`) and preserve live native state on redraw. Web/Pyodide changes
remain excluded. Flet/Tkinter-only caches and PUI release-note consolidation
require no CPPUI implementation changes. Older gaps below remain open.

Canvas scopes follow PUI `cae7713`/`10fe513`/`f0c1cf5`: Qt restores native
state, wx restores CPPUI pen/brush choices, and LVGL provides compatibility
no-ops. The new redundant-setter guards were evaluated in native C++ and are
intentionally not adopted: benefits were small or inconsistent. See the
[benchmark method and decision](canvas-performance.md). Browser event/command
changes remain excluded.

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
Track wxWidgets component/runtime work in the [wx plan](wx-progress.md).
Shared-core and public API decisions remain coordinated here.

Use reference implementation and regression tests together to establish each
contract. Where documentation, implementation, and tests disagree, record the
case and chosen behavior before implementing it. Python decorators, dynamic
attributes, and garbage collection need C++ equivalents, not literal ports.
Pin any subsequently adopted reference changes by component and commit here.

Adopted reference updates beyond the original baseline:

- Previous native delta audit through PUI `8a8ef99` (2026-10-16): MaskedField (`139d070`),
  Application/default Window titles (`417eaa7`), Table decorations (`fb5ecf8`),
  autocomplete (`f9487be`) and HeavyCanvas through `8a8ef99` are ported. The new
  Canvas move/leave/wheel callbacks use owned C++ callables and an explicit
  coordinate/delta payload; other input APIs remain listed below. Password
  draft/binding behavior, decorated editor commits, title fallback and callback
  replacement/removal have native regressions. HeavyCanvas uses C++ random
  distributions, so seeded values need not match Python bit for bit.
  Web/Pyodide, their collection/binding changes and browser scheduling are
  excluded at the user's request. Other Python-only backends and release/tooling
  changes require no CPPUI port. This audit does not complete older parity tasks.
- Button/Checkbox/RadioButton/ComboBox disabled state follows PUI `d7dc23a`;
  TextField placeholders follow `184ffcd`, including the shared recipes. C++
  uses `.disabled(bool = true)` (default false) and `.placeholder(text)` setters
  with defaults restored by omission on a fresh declaration. The former enabled
  setter is removed. Native reuse, reactive disabled updates, model updates,
  draft/cursor preservation and silent property changes have regression coverage.
- Window content sizing follows PUI `7d8655e`: the top-level native layout uses
  `SetNoConstraint`, preserving declared and user-set sizes as content grows.
- Tabs follows PUI `65f2c02` (including placement from `b9e3ad5`): one physical
  page per Tab, optional internal/external selection, native model updates,
  imperative selection, ID/label keys and first-page fallback for missing keys.
  The receiver refresh fix in `7206343` is already handled by BaseWidget's
  per-declaration connection cleanup and Tabs' reconnect on every update.
  Matching regressions check repeated native selection after old declarations
  are reclaimed and preservation of unrelated native selection listeners.
- ComboBox follows PUI `9a3b142` and its shared `combobox_get`/set helpers:
  retain editable buffers, clear the index for free text, display selected labels,
  and preserve unbound selection by item identity across keyed moves.
- Views follows PUI `e5927cb`, the selection contract in `65f2c02`, and cookbook
  updates through `c84796f`: optional internal/external models, retained native
  pages and subviews, keyed moves, blank/unknown selection, and pre-mount ID
  validation. C++ `select()` requires the current declaration under the borrowed
  return contract below; Python's old-handle forwarding is not copied.
  PUI `c84796f` adopts CPPUI's Change text button, Saved text label and distinct
  state-selection labels, so the shared example already matches that update.
  The `7206343` Views regression also covers repeated native button selection
  and checkbox-triggered redraws while retaining the selected native page.
- The remaining Qt/shared deltas were compared through PUI `a4c6840`:
  reused Table adapter resets (`df4cbac`), guarded queued quit (`675e831`), and
  native teardown through virtual subviews (`9d24163`) already have C++
  equivalents. Optional binding clears (`fd69ff0`) preserve empty `OptionalText`
  directly. The Splitter cookbook follows `d3f0679` with three horizontal panes.
  Python decorator deprecation, signatures, Python 3.9 dependencies, and Textual
  tooling do not change the C++ API. The class-based C++ construction API stays
  in place. This delta sync does not complete the remaining parity work below.
  Subsequent changes add no Qt runtime delta: the shared Text sample uses a
  styled `span`, and wx HTML/TextField sizing is tracked in the wx plan. The
  TextField sizing changes in Flet, Textual and Tkinter have no CPPUI backend.
  Automatic sizing guidance from `1beb36d` is documented with CPPUI's actual
  backend mappings. The Textual-only redraw fix (`4b78b2a`) has no backend to
  port here; Qt/wx already queue redraws, while LVGL TextField defers draft
  synchronization until reconciliation has finished.
  PUI's 1.0/1.1 version metadata does not change CPPUI's release version.

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

Remaining tasks and unresolved contracts are listed below. Display checks are outstanding verification, not blocked on API choices.
Backend-specific wx verification is tracked in the wx plan.

Each row maps remaining work to its implementation phase and a pinned reference
case. Acceptance involving undecided APIs applies after that contract is chosen;
the separate deferred section records intentional omissions.

| Area | Current basis | Remaining acceptance | Phase / reference case |
| --- | --- | --- | --- |
| DOM reconciliation | Scoped identity, native moves, virtual flattening and reclamation | Define native remount and interop identity after external deletion | 1 / [virtual nodes][ref-virtual] |
| Views | `QtView` queued/coalesced redraw, immediate sync and one-time setup | Changed retained-view inputs arrive; a failed nested render can recover | 2 / [nested views][ref-nested], [redraw semaphore][ref-redraw] |
| State | Read tracking, assignment, `+=`, `-=`, listener cleanup and Binding adapters | Change subscriptions and explicit notification; a batch renders its final value; nested collection edits notify subscribers | 2 / [binding][ref-binding], [batched mutation][ref-batch] |
| Window | `QMainWindow`, title/icon/size and initial/imperative state flags | Keypress payloads follow the deferred event contract; verify window-manager behavior on a display | 3 / [Qt regression][ref-qt] |
| Label/Button | Owned text, selectable labels, styles and current click callbacks | Event payloads follow the deferred contract | 3 / [Label][ref-label], [Button][ref-button] |
| Grid/Stack | Native Grid spans and StackAll overlap | Add shared fluent grid placement | 3 / [Grid][ref-grid], [Stack][ref-stack], [HBox][ref-hbox] |
| TextField/Checkbox | String TextField/MaskedField model/draft bindings; boolean Checkbox and membership adapters | Define conversion helpers and observable collections | 4 / [TextField][ref-textfield], [Checkbox][ref-checkbox], [RadioButton][ref-radio] |
| ComboBox/ComboBoxItem | Index/string bindings, distinct labels/values, editable text and dynamic items | Define raw non-string value adapters and deferred event payloads | 4 / [ComboBox][ref-combo], [Qt regression][ref-qt] |
| Canvas input | Native drawing, image resources, move/leave/wheel callbacks | Add remaining pointer callbacks; verify rendering on a real HiDPI display | 7 / [Canvas][ref-canvas], [Drop][ref-drop] |
| Table/Tree values and input | Typed external adapters with explicit ownership and model-owned indexes | Define arbitrary cell values and deferred Tree drag/drop payloads | 8 / [Table adapter][ref-table-adapter], [Tree adapter][ref-tree-adapter] |
| Qt interop | Missing | Borrowed widgets survive removal according to their ownership contract | 6 / [embedding example][ref-interop], [Widget][ref-widget] |
| Validation | Core/native CTest, headers, sanitizer and Valgrind harnesses | Port each remaining case above; manual focus/window/HiDPI behavior on a display | 1–8 / [Qt regression][ref-qt], [wx regression][ref-wx] |

[ref-virtual]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/virtual_nodes.py
[ref-nested]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/nested_view.py
[ref-redraw]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/redraw_semaphore.py
[ref-batch]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/batched_state_mutation.py
[ref-qt]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/pyside6_regression.py
[ref-wx]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/wx_regression.py
[ref-interop]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/examples/pyside6_interop.py
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
[ref-canvas]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/canvas.py
[ref-drop]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/drop.py
[ref-table-adapter]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/table_adapter.py
[ref-tree-adapter]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/tree_adapter.py
[ref-widget]: https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/widget.py

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
- Canvas move/wheel use the shared `CanvasPointerEvent` coordinate/delta payload
  on Qt/wx; leave and existing control callbacks take no arguments. General
  keyboard/button/modifier event objects remain deferred; portable inbound drops are implemented.

### Phase 1 — Node identity, ownership, and reconciliation (P0)

Reference: [node.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/PUI/node.py), [dom.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/PUI/dom.py),
[virtual_nodes.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/virtual_nodes.py), and
[subview_id.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/subview_id.py).

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

Acceptance: port the scenarios in [tests/batched_state_mutation.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/batched_state_mutation.py),
[tests/redraw_semaphore.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/redraw_semaphore.py), and
[tests/nested_view.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/nested_view.py) into assertions. A redraw burst settles to the latest
state; a removed subscriber receives no work; invalid text leaves the model
valid; builder failure leaves the next render usable.

### Phase 3 — Common modifiers, layout, and existing widgets (P1)

Reference: [node modifiers](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/PUI/node.py),
[Qt base](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/PUI/PySide6/base.py),
[layout.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/PUI/PySide6/layout.py), and corresponding cookbook pages.

- [ ] Add PUI's fluent grid placement API; existing node coordinate/span fields
  already drive Grid. Verify changed spans and native geometry on a real display.
- [ ] Add Window keypress after the deferred event contract is addressed; verify
  initial and imperative window-state behavior with a real window manager.
- [ ] Add common event payloads after the explicitly deferred input contract is addressed.

Acceptance: verify HBox/VBox/Grid/Stack native geometry, child order and size
policies on a real display; add fluent grid placement coverage. Exercise the
window-state scenarios from
[pyside6_regression.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/pyside6_regression.py).

### Phase 4 — Bound controls and text/image components (P1)

- [ ] Extend TextField beyond string bindings after shared conversion helpers
  are chosen; typed input/change event objects remain explicitly deferred.
- [ ] Decide automatic collection observation and value-based Checkbox overloads;
  explicit boolean membership Binding adapters are supported.
- [ ] Extend ComboBox beyond string item values after choosing raw value adapters;
  typed change-event payloads remain deferred.

Acceptance: port the binding, textfield, checkbox, radiobutton, combobox,
progressbar, text, and image cookbook cases. Assert a single commit, no feedback
loops, current binding/callback after reuse, and negative ComboBox index handling.

### Phase 5 — Containers and window interactions (P1)

- [ ] Verify native desktop file dialogs and window-manager interaction on a
  real display, including Modal focus/flags; offscreen widget-dialog tests do
  not establish platform parity.

Acceptance: native dialog interaction, Modal focus and window-manager state
require manual display checks.

### Phase 6 — Native interop (P2)

- [ ] Add equivalents of `QtInPui` and `PuiInQt`: specify borrowed native widget
  lifetime, removal/reparenting, and embedding into an existing Qt application.

Acceptance: borrowed widgets survive declaration removal. Adapt
[pyside6_interop.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/examples/pyside6_interop.py).

### Phase 7 — Canvas and input (P2)

- [ ] Implement mouse down/up/double-click, enter and modifiers after the
  remaining input-event contract is addressed.
- [ ] Extend the existing move/leave/wheel lifetime coverage to each added input
  callback so replacement/removal cannot dispatch to a retired declaration.
- [ ] Verify Canvas rendering and logical coordinates on a real HiDPI display.

Portable inbound drops and the Drop cookbook are ported through PUI `526b071`,
including payloads, acceptance, callback replacement/removal and viewport events.
Retain automated painting/lifetime coverage
while validating physical-display rendering separately.

### Phase 8 — Table/Tree values and input (P2)

- [ ] Define portable arbitrary mixed/custom cell values beyond typed scalar
  Table adapters and string Tree labels before expanding those value contracts.
- [ ] Add Tree drag/drop behavior after the deferred input-event contract is
  addressed; keep acceptance/rejection and callback lifetime explicit.

Acceptance: verify added value conversion and storage lifetimes under their
chosen contracts; verify accepted/rejected Tree drops and safe callback
replacement/removal under the deferred event contract.

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
| `binding.py`, `textfield.py` | 2, 4 | External-change notification, binding change callbacks and text conversion/events |
| `widget.py` | 6 | Borrowed Qt widget |
| `drop.py` | 7 | Event payloads and accepted/rejected drops |

## Decisions awaiting direction

Continue independent implementation while these contracts remain undecided.

- **Binding ownership:** extend the adapter for PUI's conversion/change/emit
  behavior. Document C++ subscription lifetimes and State copy/move semantics;
  ask only if tracing the reference and existing C++ ownership rules leaves a
  public choice unresolved.
- **Batch completion:** queued redraw errors use stderr as agreed. A future
  batch-scope exit still needs defined exception behavior during stack unwinding.
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
- **Arbitrary values:** mixed/custom table cell types and non-string tree labels
  still need a portable conversion/storage contract beyond typed scalar adapters.

## Excluded, deferred or requiring a C++ design decision

- Python hot reload and decorator syntax: preserve reusable declarative views;
  runtime C++ code replacement is outside this alignment plan.
- **Won't do — Shapely and Matplotlib:** Canvas.drawShapely, MatplotlibCanvas
  and their cookbook recipes are Python-only integrations, excluded from CPPUI.
- Arbitrary dynamic Qt keyword parameters remain excluded; the supported typed
  `.qt(...)` fields are listed in the [modifier reference](../REFERENCE.md#pui-modifiers).
- Existing PUI gaps, such as Scroll offset capture and inconsistently applied
  layout/style options: document observed behavior and any intentional CPPUI
  improvement. Native tab position and backend-specific empty/multi-child behavior are described
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

Build the Qt examples and automated suite with CMake 3.24+ and Qt 6 Widgets and
Test development packages:

```sh
cmake -S . -B build-qt -DCPPUI_BUILD_QT=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-qt -j "$(nproc)"
ctest --test-dir build-qt --output-on-failure
```

Omit `CPPUI_BUILD_QT` to test the C++11 core without a toolkit. CTest sets the Qt
offscreen platform automatically. CMake can use an installed Qt 6 development
package directly; set `CMAKE_PREFIX_PATH` if using a separate Qt installation.
LeakSanitizer execution
requires an environment without ptrace tracing. Enable `CPPUI_VALGRIND_TESTS`
in a separate non-sanitized build to run the regressions with Memcheck; use
`ctest -L valgrind --output-on-failure`. All new regression and cookbook smoke
targets must register through `cppui_add_test` so they receive leak checks too.
The harness sets `QT_ENABLE_REGEXP_JIT=0` only for Valgrind runs, following the
[Valgrind FAQ](https://valgrind.org/docs/manual/faq.html) for Qt regexp JIT
diagnostics. Do not suppress CPPUI memory errors.

Remove a task only after its acceptance checks pass. Keep CPPUI's README/REFERENCE, shared cookbook,
examples, and tests current, and retain outstanding manual checks or intentional
departures from PUI here. Core/widget parity requires all remaining phases to
pass, with optional integrations still listed as explicit gaps.

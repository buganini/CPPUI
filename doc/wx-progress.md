# wxWidgets alignment with PUI/wx

PUI `97e442c` adds the backend name to the cookbook window title and sidebar
heading: `Cookbook (wxWidgets)`.

The native sync through PUI `1c7b45c` adds the shared Binding log viewer and
editable Table cookbook markers, with wx decorated Table control ownership and
event forwarding.
The shared DOM offset cache was adopted after native measurement; see the
[latest audit](pui-sync-2026-10-18.md) and [performance results](dom-offset-performance.md).

Splitter follows PUI/wx `d3f0679`: arbitrary pane counts built from nested native
two-pane splitters, widget/layout content, padding, both orientations and retained
controls during keyed changes. Unchanged composition preserves sash positions;
structural or orientation changes redistribute equally, as PUI does. Independent
virtual-view redraws refresh the physical container. The shared Splitter recipe
is enabled. See the [contract](../REFERENCE.md#splitter).

Splitter verification (2026-10-18): build with `-j "$(nproc)"`, including C++11
and independent-header checks; all 28 wx/core CTests and the standalone cookbook
smoke test pass on GTK/Linux. The Splitter regression also passes the existing
wx Valgrind configuration, covering native ownership, sash retention/resize,
keyed reorder and editor drafts, independent views, external deletion and
removal from a click callback. The rebuilt wx cookbook was launched.

An earlier follow-up adds `.visible()`, delayed UI callbacks through `getPoster()`,
and typed tab placement with the shared `Side` enum. Hidden content retains native
state; callbacks run on the UI loop and stop executing after target removal. wx
Stack now owns a native panel, and foreground styles use SetOwnForegroundColour.

Portable inbound drops follow PUI `526b071`/`e912d45`: file/text/URI data, suffix filters,
explicit acceptance and the shared Drop cookbook. Replacement, removal and
native data negotiation are covered by regression tests.

Named node refs follow PUI `d83bbe2`: UI-thread lookup, retained binders and
queued selection commands are ported, with Qt/wx desktop window commands and
a shared Refs recipe. Worker submission, redraw and removal are tested. Native
State-ref inspection publishers (Web/AI tooling) remain excluded.

Canvas/HeavyCanvas now follow PUI `39759a2`/`e46cb43`: completed-draw FPS,
continuous painting and full-frame color changes. Overlay removal and repeat
painting are covered by native regressions.

Latest native delta audit: PUI `97e442c` (2026-10-18). See the
[component audit and exclusions](pui-sync-2026-10-18.md). Native Options,
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
[measurement and decision](canvas-performance.md). Browser event/command
changes remain excluded.

Prepared on 2026-10-13 against CPPUI `07271a3` and
[PUI 2e4f0eb](https://github.com/buganini/PUI/tree/2e4f0eb82996e271bb3f4bc1f7511ea8b4c966bd/PUI/wx).
Use the native C++ wxWidgets API and the existing CPPUI public interfaces, with
C++11 headers and the same shared cookbook sources. Inspect the reference code
and tests before treating a difference as a design decision. This checklist
contains remaining work only; remove completed items after each tested commit.

Views follows PUI `ffd2256`, the shared optional selection contract in `65f2c02`,
and the native cleanup cases in `9d24163`: wxSimplebook owns retained panel pages,
each with a zero-padding VBox for ordinary controls, layouts and virtual views.
Internal and external models share `select()` and blank/unknown-key behavior.
The common cookbook demonstrates both forms. As with Qt, imperative selection
uses the current C++ declaration, not a factory reference saved across redraws.

The Text cookbook page follows PUI's wx-enabled page at `f9c22fa`: plain
multiline Text and Html are available. MarkDown displays its source as plain Text,
matching PUI/wx's fallback (`pui_supported = False`), and warns at compile-time
when called. The page remains partial because Markdown formatting is unfinished.

Html sizing follows PUI `9aec567`: derive the native best client size from parsed
content, invalidate it after content updates, and expand along both layout axes.
Native regressions cover initial visibility, retained-window content growth and
shrinkage, and expansion when the available layout space changes. The shared
HTML sample uses `span` for its styled text, matching that reference update.

TextField sizing follows PUI `5fc15ed`, audited through `b551e19`: use the native
minimum size and height, with horizontal expansion priority 3. Explicit layout
dimensions still apply and reset when omitted. Native regressions cover fields
in rows and columns, window resizing and explicit dimension reset. The remaining
`b551e19` changes affect Flet, Textual and Tkinter, which CPPUI does not implement.

## Ownership and established contracts

Button/Checkbox/RadioButton/ComboBox `.disabled(bool = true)` follows PUI
`d7dc23a` (default false); the former enabled setter is removed. TextField
`.placeholder(text)` follows `184ffcd` through native `SetHint`, including the
shared cookbook hint. Omission on a fresh declaration restores enabled/empty
defaults. Native regressions cover reactive toggles, identity and editable-mode
preservation, programmatic updates while disabled, and hint changes/clears
without altering the active draft, cursor or callbacks. Audited through PUI
`a4c6840`; its release metadata does not change the CPPUI version.

The previous native delta audit reached PUI `8a8ef99` (2026-10-16). MaskedField
(`139d070`) uses wxTE_PASSWORD with the existing model/draft contract.
Application title fallback (`417eaa7`) and Table prefix/suffix/placeholder
(`fb5ecf8`) use native display names and owned composite editors. wxChoice lacks
an empty-field hint; text editors use SetHint. HBox compact children center
vertically and VBox compact children align left (`e534b8e`/`129068b`). Backgrounds
preserve native inheritance until explicitly set, and reset with wxNullColour
when removed (`a99e2cf`). Tabs already had the expansion priority in `9093d23`.
The shared autocomplete and HeavyCanvas recipes are enabled; Canvas now exposes
move/leave/wheel callbacks with local coordinates and native angular deltas.
Callbacks are copied before invocation and refreshed/cleared on redraw/removal.
Web/Pyodide changes are excluded at the user's request; older gaps remain below.

Factory results are borrowed construction references. Applications retain State
and IDs, not declarations across redraws. Native window pointers are observers;
wx parents own child windows and attached sizers own nested sizers. Transfer
native access and dispatch on reuse; invalidate observers before destroying
native owners. Detached layout subtrees must release both their sizers and their
child windows, which have separate native ownership.

Queue/coalesce redraws on the GUI thread, cancel pending work on teardown, and
snapshot callbacks before calling application code. Support borrowed wxApp
runtimes without shutting them down. Report queued exceptions to stderr and
preserve the old UI for failures before reconciliation. Shared nested-failure
recovery and retained-view inputs remain tracked in the
[Qt plan](pyside6-progress.md). Use Binding/State adapters, reset omitted style
and layout options, and preserve explicit zero. Dialog cancellation uses shared
Optional results. Input payloads beyond Canvas move/wheel remain deferred. Shapely
and Matplotlib are Python-only and will not be ported.

## Validation environment

Native runtime/reconciliation tests run against wxGTK 3.2.9, including owned and
borrowed applications, external destruction, keyed moves, subtree replacement,
queued failure recovery, callback self-removal and repeated teardown. The plain
`wx.native_baseline` executable uses no CPPUI declarations. Linux memory checks
use narrow suppressions for its fontconfig/Pango/GTK caches, worker TLS and
wxRB_SINGLE GTK weak-notification metadata; no CPPUI or wx object allocation
stack is suppressed. Pango 1.57's Cairo shaping path also reports a 27-byte
possibly-lost language record in the unchanged native-only baseline; its narrow
suppression requires the Pango/Cairo/HarfBuzz allocation stack. Validate other wx ports
separately. Root CMake: `-DCPPUI_BUILD_WX=ON`; tests require a display.

## Reference cases

- [Runtime/base ownership and layout](https://github.com/buganini/PUI/blob/2e4f0eb82996e271bb3f4bc1f7511ea8b4c966bd/PUI/wx/base.py)
  and [wx regression tests](https://github.com/buganini/PUI/blob/2e4f0eb82996e271bb3f4bc1f7511ea8b4c966bd/tests/wx_regression.py).
- [Table](https://github.com/buganini/PUI/blob/2e4f0eb82996e271bb3f4bc1f7511ea8b4c966bd/PUI/wx/table.py)
  and [Table tests](https://github.com/buganini/PUI/blob/2e4f0eb82996e271bb3f4bc1f7511ea8b4c966bd/tests/wx_table.py).
- [Tree](https://github.com/buganini/PUI/blob/2e4f0eb82996e271bb3f4bc1f7511ea8b4c966bd/PUI/wx/tree.py)
  and [Tree tests](https://github.com/buganini/PUI/blob/2e4f0eb82996e271bb3f4bc1f7511ea8b4c966bd/tests/wx_tree.py).
- [Shared cookbook](https://github.com/buganini/PUI/tree/2e4f0eb82996e271bb3f4bc1f7511ea8b4c966bd/cookbook)
  and [timeline tests](https://github.com/buganini/PUI/blob/2e4f0eb82996e271bb3f4bc1f7511ea8b4c966bd/tests/timeline_regression.py).

## Decisions and verification boundaries

- Remaining input payloads, observable collections, retained-view inputs, arbitrary data
  values and native embedding share the existing deferred CPPUI contracts.
- **Markdown:** not yet implemented in CPPUI/wx or the PUI/wx reference;
  choose and integrate a Markdown renderer/dependency. The shared Text cookbook
  page demonstrates plain multiline Text, Html and MarkDown's raw-source fallback.
  The fallback has a call-site compiler warning; header inclusion alone is quiet.
- **Modal:** the shared API opens application-modal content without blocking
  reconciliation. wxGTK's [ShowWindowModal fallback](https://github.com/wxWidgets/wxWidgets/blob/v3.2.9/src/common/dlgcmn.cpp#L450)
  calls blocking ShowModal. Choose a modeless dialog plus an application-wide
  modal coordinator, including nested dialogs and windows belonging to a borrowed
  host runtime, before promising the Qt contract. Standard blocking dialogs are
  already separate APIs.
- **MDI:** wxMDIChildFrame requires a wxMDIParentFrame; CPPUI Window currently owns
  wxFrame and MdiArea is an ordinary child in the shared API. Choose whether the
  Window factory selects an MDI native class or the area uses a custom embedded
  subwindow implementation. PUI/wx supplies no mapping to follow.
- **Toolbars:** wxFrame has one managed toolbar slot and native toolbar controls
  require wxControl, while the shared Qt API allows multiple bars and arbitrary
  widgets/layout content. Choose a wxAUI dependency versus a custom toolbar host
  before defining equivalent docking, content ownership and ordering. PUI/wx
  has no toolbar contract; menus are independent and supported.
- Only Linux/GTK is available for immediate native verification. Windows/macOS,
  physical input, window-manager behavior and mixed-density displays require
  their corresponding environments; do not label them tested from compilation.

After each completed change: update reference/build docs and examples/tests,
remove its tasks here, add a concise milestone in `Changelog.txt`, and commit.

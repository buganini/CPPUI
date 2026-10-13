# wxWidgets alignment with PUI/wx

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

## Ownership and established contracts

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
Optional results. Portable input payloads remain explicitly deferred. Shapely
and Matplotlib are Python-only and will not be ported.

## Validation environment

Native runtime/reconciliation tests run against wxGTK 3.2.9, including owned and
borrowed applications, external destruction, keyed moves, subtree replacement,
queued failure recovery, callback self-removal and repeated teardown. The plain
`wx.native_baseline` executable uses no CPPUI declarations. Linux memory checks
use narrow suppressions for its fontconfig/Pango/GTK caches, worker TLS and
wxRB_SINGLE GTK weak-notification metadata; no CPPUI or wx object allocation
stack is suppressed. Validate other wx ports
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

- Input payloads, observable collections, retained-view inputs, arbitrary data
  values and native embedding share the existing deferred CPPUI contracts.
- **Markdown:** not yet implemented in CPPUI/wx or the PUI/wx reference;
  choose and integrate a Markdown renderer/dependency. The shared Text cookbook
  page demonstrates plain multiline Text, Html and MarkDown's raw-source fallback.
  The fallback has a call-site compiler warning; header inclusion alone is quiet.
- **Splitter:** not yet ported in CPPUI/wx. PUI `d3f0679` supplies nested native
  two-pane splitters for arbitrary child counts. Port that implementation and
  verify that sash positions survive keyed insertion/reordering.
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

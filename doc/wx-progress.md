# wxWidgets alignment with PUI/wx

Prepared on 2026-10-13 against CPPUI `07271a3` and
[PUI 2e4f0eb](https://github.com/buganini/PUI/tree/2e4f0eb82996e271bb3f4bc1f7511ea8b4c966bd/PUI/wx).
Use the native C++ wxWidgets API and the existing CPPUI public interfaces, with
C++11 headers and the same shared cookbook sources. Inspect the reference code
and tests before treating a difference as a design decision. This checklist
contains remaining work only; remove completed items after each tested commit.

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

## Implementation sequence

1. **Shared cookbook and validation.** Add `cookbook/wx` startup/manifest/build
   files, enable each implemented shared recipe, preserve the sidebar/result
   layout, and add build instructions. Expand native regression/smoke coverage,
   sanitizers and Valgrind after each component. Check real-display interaction
   and platform-specific behavior separately from scripted native events.

## Validation environment

Native runtime/reconciliation tests run against wxGTK 3.2.9, including owned and
borrowed applications, external destruction, keyed moves, subtree replacement,
queued failure recovery, callback self-removal and repeated teardown. The plain
`wx.native_baseline` executable uses no CPPUI declarations. Linux memory checks
use narrow suppressions for its fontconfig/Pango/GTK caches and worker TLS;
no CPPUI or wx object allocation stack is suppressed. Validate other wx ports
separately. Root CMake: `-DCPPUI_BUILD_WX=ON`; tests require a display.

Remaining memory investigation: `valgrind.wx.controls` reports 176 bytes in four
possibly-lost GTK/GObject weak-notification allocations (no invalid access or
definite/indirect leak); sanitizer checks pass. The native-only baseline reproduces
font/theme/Glycin worker allocations, but not these final records, which remain
unsuppressed. Resolve or isolate them before declaring all wx memory checks clean.

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
- **Markdown:** PUI/wx marks it unsupported; choose a Markdown renderer/dependency
  before enabling the complete shared Text cookbook page. Plain Text and Html
  already have native APIs.
- PUI/wx has no Stack, Splitter, Modal, MDI or toolbar implementation. Inspect
  wxWidgets capabilities and existing CPPUI contracts when reaching these;
  record specific unresolved behavior here instead of assuming all are blocked.
- Only Linux/GTK is available for immediate native verification. Windows/macOS,
  physical input, window-manager behavior and mixed-density displays require
  their corresponding environments; do not label them tested from compilation.

After each completed change: update reference/build docs and examples/tests,
remove its tasks here, add a concise milestone in `Changelog.txt`, and commit.

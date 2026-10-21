# Native layout sync — 2026-10-21

Reference: committed local and public PUI
`d07abf51683501b44becf2c427d93470ee42722a`, following the
[`974b88b` Grid sync](pui-sync-2026-10-21-grid.md). HTTPS main matched the clean
local checkout at inspection. Uncommitted PUI changes, AI development and
Web/Pyodide remain excluded.

| PUI change | CPPUI disposition |
| --- | --- |
| Reversible boxes (`9f0d854`) | `HBox(inverse)`, `VBox(inverse)`, and independent `inverseH`/`inverseV` arguments on HVBox/VHBox across Qt, wx and LVGL |
| Native divider fixes (`f3d0435`) | wx uses native best size and changes line orientation on retained controls; Qt expansion priorities track the line's current axis |
| Qt expansion priorities (`d739369`) | Visible siblings compete by component priority or explicit weight; static controls pack at the start; nested virtual updates recalculate the physical box |
| Responsive Grid children (`a591f24`) | Typed `.wideGrid(minWidth, grid::...)` placement/span overrides preserve base identity in manual and automatic grids |
| Cookbook updates (`9f0d854`, `2d99bf3`, `a591f24`) | Reversal checkboxes, shared colors, stronger wide styles, selected-sidebar font weight, responsive editor span |
| Reference consolidation (`a23c250`) | C++ reference updated for boxes, sizing priorities, responsive placement and current divider behavior |
| Textual/Tkinter work (`8310686`, `2fa9cde`, `4c22434`), documentation hosting (`85e10ba`), Pyodide heading (`d07abf5`) | No corresponding native CPPUI implementation change; terminal APIs and Python-only integrations remain excluded |

## Retained layout and C++ choices

Box reversal uses QBoxLayout direction, LVGL reverse flex flow, and in-place wx
sizer-item reversal. Logical declarations and flattened mounting indices remain
in declaration order. wx translates those indices when adding/removing items in
a reversed sizer; its padding wrappers and controls remain mounted. This avoids
copying Python's reversal occurrence maps and retained-view redraw bookkeeping:
equal anonymous declarations, nested virtual children and independently added
children retain their correspondence without reversing the C++ DOM. Physical
child boxes keep their own order. CPPUI already restores the previous children
and cleans failed proposals when view construction throws.

Qt boxes retain a flat child list using the shared declaration-rebinding hook.
Their post-sync pass assigns stretch and cross-axis alignment from visible
children's priorities. Positive weights outrank component defaults; weight zero
disables main-axis expansion. Spacer defaults to weight one. A retained control's
native policy is restored before applying a new declaration, so removed weights,
policy overrides and axis changes do not leave stale sizing. Independent reflow
also tolerates a sibling whose native QWidget was externally deleted. QtInPui's borrowed
native-widget extension is still outside CPPUI's existing wrapper support.

Responsive Grid fields are effective placement. An inline optional base cell
records identity before the first `.wideGrid()` call; later `.grid()` calls
update only the supplied base fields. This also preserves legacy direct-field
initialization before responsive calls. Thresholds read the nearest physical
parent's content width, include the boundary, and apply in declaration order.
In automatic mode only spans affect placement. The cookbook editor spans one
column below 576 pixels and two at or above that width.

The sparse-grid regression also exposed a wx GridBagSizer limitation: it rejects
growable columns beyond its calculated occupied tracks. CPPUI now establishes
an empty trailing track when necessary and recalculates the track count before
registering growable columns. This supports sparse, empty and changing-span
automatic grids without a native assertion.

The selected-sidebar weight follows each backend's existing style support; Qt
and LVGL's previously documented font-weight limitations remain. The existing
no-source-pane cookbook design is retained. These changes implement native
semantics; no Python dispatch cache, painter optimization or measured speedup
claim is introduced, and no benchmark results are retained.

## Verification

All four native cookbooks and standalone-header checks built in RelWithDebInfo.
The combined core/Qt/wx/examples suite passed **80 tests**; LVGL SDL2 and X11
passed **28 each**, totaling **136**. Core Grid helpers also compiled and passed
with explicit C++11. `git diff --check` is clean.

The new native regression covers ordinary and adaptive reversal on both axes,
nested virtual views, independent insertions/removals, hidden items, anonymous
occurrence identity, physical child-container boundaries, divider rotation and
thickness, editor draft/selection retention and teardown. Grid tests cover exact
600/800-pixel thresholds, ordered overrides, base identity, `.id()`/`.ref()`
after overrides, manual/automatic reflow, retained drafts, sparse tracks and
empty-grid transitions. Qt geometry tests cover priority competition, hidden
winners, zero/removed weights, native-policy removal, independent nested-view
updates, native sibling deletion and natural-height scroll contents.

Qt reversal and expansion regressions passed ASan/UBSan with leak detection;
wx reversal passed the same checks using only the existing fontconfig leak
suppression. LVGL reversal and responsive Grid regressions passed Valgrind with
zero errors and no remaining allocations. LeakSanitizer ran outside sandbox
tracing, which prevents its thread inspection.

wx tests used a temporary Xvfb display unpacked under `/tmp`, without installing
system packages. Desktop-driven wx resize checks initially missed breakpoints;
the isolated display resolved that interference. The X11 driver's application
and cookbook tests crashed at startup under Xvfb, but both passed on the desktop
X display; that Xvfb-specific driver behavior is not resolved by this sync.
The cookbook pixel check was updated to the new rectangle color. GTK's single
active text selection required focusing the selected test editor after setting
anonymous editor drafts; drafts and identity are checked for all three editors.

Coverage is Linux with Qt 6.10.2, wxWidgets 3.2.9 and LVGL 9.6.0. Windows/macOS,
real IME composition, screen readers and a full DPI/theme matrix remain
unverified. Visual reversal does not promise a corresponding keyboard Tab-order
change. Older backend parity gaps remain listed in their progress documents.

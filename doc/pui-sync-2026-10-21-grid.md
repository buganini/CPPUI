# Native Grid sync — 2026-10-21

Reference: committed local and public PUI
`974b88be12f10894f79f80da5aab74c07126f1fd`, following the
[`0dc72e2` ComboBox sync](pui-sync-2026-10-21.md). The local PUI tree was clean
at inspection; uncommitted work, AI development and Web/Pyodide remain excluded.

| PUI change | CPPUI disposition |
| --- | --- |
| Adaptive Grid, three-state placement, native reflow (`c93f414`) | Shared placement plus Qt, wx and LVGL adapters; retained controls, hidden-item compaction and independent virtual-child reflow |
| Dense mode moved into columns (`0f8506b`) | `.columns(n, grid::dense(mode))`, with inline `Optional<bool>` and ordered placement by default |
| Manual/Adaptive Grid mosaics and all three modes (`81fc853`, `774f73a`, `974b88b`) | Shared native recipes, 384/576/768 logical-pixel breakpoints, visibility, reversal and retained editor |
| Textual Grid defaults (`b9e9881`) | No terminal backend; shared graphical column count/dense semantics are ported |
| Tkinter/Textual/Flet/browser Splitters (`38b7320`, `0ff1e7d`, `593bfd0`, `bd82abf`) | Changes to backends outside CPPUI's scope; no direct port |
| Cookbook Code/Result Splitter (`7567642`) | Explicitly skipped at the user's request: CPPUI does not display a source pane |

## C++ API and retained layout

`.columns(n)` requires a positive count and resets backfill to row-ordered.
`.columns(n, grid::dense(true))` searches from the top-left;
`.columns(n, grid::dense(false))` advances the placement cursor.
`grid::dense({})` explicitly selects ordered mode. Matching `.wideColumns()`
calls replace only the count, in declaration order, using the nearest physical
parent's existing reactive size binding. Unknown initial geometry uses the base
count. Thresholds are inclusive native logical pixels.

Named child `.grid(grid::row(...), grid::column(...), grid::rowspan(...),
grid::columnspan(...))` arguments follow CPPUI's typed modifier design. No map,
argument lookup or per-argument allocation is introduced. Existing coordinate
fields remain usable. Auto-placement stores an optional effective `grid_cell`
separately from declaration coordinates and identity. Hidden items have no
automatic cell; oversized spans clamp for placement and recover after widening.
The reference documents manual coordinate identity and automatic reflow separately.

Grid retains a list of mounted physical children. The existing wx child-rebinding
hook is moved to the shared Node retirement protocol, keeping all three backends'
mounted lists current before old declarations are reclaimed. Independent virtual
child reconciliation requests Grid reflow. Qt retains widgets under the same
parent and rebuilds layout items; wx detaches padding wrappers without deleting
windows/sizers and recreates their GridBag positions. LVGL updates retained grid
cells and keeps descriptor-array storage alive through native style replacement.

## Performance applicability

The shared placement behavior is needed independently of Python binding overhead.
CPPUI stores occupied rectangles and skips collisions to their right edge instead
of reproducing Python's per-cell set. Memory scales with items rather than the
number of spanned cells. Deterministic randomized tests compare this algorithm
against a separate cell-by-cell reference for all three modes. This is a semantic
and allocation choice, not a measured runtime speedup claim. No painter change,
Python dispatch cache or benchmark result is added.

## Verification

Core tests cover modes, invalid counts/spans, named-argument validation, hidden
items, span restoration and the cookbook's distinct waterfall placements.
Native tests inspect actual cells, check editor identity/draft/caret retention,
change columns, hide/reorder items, redraw a virtual child independently, switch
between automatic/manual placement, cross width breakpoints and tear down grids.
Shared cookbook smoke tests exercise all three modes, hide C, reverse tiles,
edit retained text and shrink the window.

Qt/wx native Grid regressions passed ASan/UBSan with leak checks (wx uses the
existing fontconfig suppression). The LVGL regression passed Valgrind with
zero errors and no remaining allocations. Existing wx stack, splitter and
container regressions also passed after moving the child-rebinding hook.

All four cookbooks and standalone header checks built in RelWithDebInfo using
`-j "$(nproc)"`. The final core/Qt/wx/examples suite passed 77 tests; SDL2 and X11
each passed 27, totaling 131. The full build caught an include-order namespace
collision in the new Grid helper; qualifying the shared namespace fixed it.
The first SDL2 cookbook run found an obsolete test lookup for the old manual
Grid's IDs. That check now validates the new mosaic's actual native geometry,
and both LVGL cookbook suites pass. Source hashes matched after the builds.
The four rebuilt cookbook executables were launched; all four processes stayed
alive and their main windows were visible on the desktop.

Platform coverage is Linux with Qt 6.10.2, wxWidgets 3.2.9 and LVGL 9.6.0.
macOS/Windows, screen readers, native IME composition and the full DPI/theme
matrix remain unverified. Dense/ordered visual placement can differ from
logical declaration/Tab order; reflow does not reorder application data.

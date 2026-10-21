# Qt remaining alignment work

This file tracks open Qt and shared-core gaps against the CPPUI/PUI contracts.
Remove resolved items; keep change history and completed verification results
out of this document. Backend-specific gaps belong in the
[wx plan](wx-progress.md) and [LVGL plan](lvgl-progress.md).

## Shared reactive core and bindings

- [ ] Deliver changed constructor inputs to retained, parameterized subviews
  without losing their local state.
- [ ] Recover from nested builder failures after native reconciliation starts.
  Choose transactional prebuilding or an explicit partial-sync recovery protocol
  so the next render remains usable.
- [ ] Add Binding conversion/validation helpers for non-string TextField models,
  change subscriptions and explicit notification of external getter/setter
  storage. Define subscription ownership and State copy/move behavior.
- [ ] Implement scoped batched mutation and unchanged-value notification
  suppression. Define comparison for values without equality and exception
  delivery when a batch flushes, including during stack unwinding.
- [ ] Add observable list/map changes, element bindings and value-based Checkbox
  overloads; nested mutations must notify subscribers.
- [ ] Define portable raw ComboBox item values and arbitrary mixed/custom
  Table/Tree cell values beyond the current string/scalar adapters. Preserve
  conversion, selection identity and storage lifetimes.

## Input and control callbacks

- [ ] Define shared keyboard/button/modifier payloads and typed input/change
  events, then apply them to Window keypress and control callbacks.
- [ ] Add Canvas mouse down/up, double-click, enter and modifier information.
  Verify each callback uses the current declaration and stops after removal.
- [ ] Add Tree drag/drop under the shared event contract, with explicit
  acceptance/rejection and safe callback replacement/removal.

## Native interop and ownership

- [ ] Add borrowed-widget wrapping and embedding into an existing Qt application,
  corresponding to QtInPui/PuiInQt. Define removal, reparenting and external-owner
  lifetime; removing a declaration must preserve a borrowed widget as specified.
- [ ] Define native-widget identity and remounting after external destruction.
  Clearing a deleted native pointer alone does not rebuild its subtree on the
  next redraw.

## Remaining validation

- [ ] Verify native file dialogs, Modal focus/flags and initial/imperative Window
  states with a real window manager.
- [ ] Verify physical input, focus traversal, clipboard/drag-selection, Chinese
  IME composition and screen-reader behavior across controls and candidate menus.
- [ ] Verify layout, fonts, Canvas rendering and logical input coordinates across
  real HiDPI/mixed-density displays, themes and scrolling configurations.
- [ ] Verify supported desktop hosts beyond Linux, including native ownership
  and teardown behavior on Windows/macOS.

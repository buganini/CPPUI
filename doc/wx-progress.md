# wxWidgets remaining alignment work

This file tracks only open wxWidgets gaps against the shared CPPUI/PUI contracts.
Remove resolved items; keep change history and completed verification results
out of this document. Shared-core decisions are tracked in the
[Qt/shared plan](qt-progress.md).

## Widgets and desktop containers

- [ ] Add formatted Markdown rendering; choose a renderer/dependency for the
  current raw-source fallback.
- [ ] Add optional bound selection to Tabs and native-to-model updates.
- [ ] Implement non-blocking application-modal content. wxGTK's ShowWindowModal
  fallback calls blocking ShowModal; define a modeless dialog plus modal
  coordination, including nested dialogs and borrowed host windows.
- [ ] Implement MDI under the shared MdiArea/MdiSubWindow contract. Choose an MDI
  Window class or custom embedded subwindows: wxMDIChildFrame requires a
  wxMDIParentFrame, while CPPUI Window currently owns wxFrame.
- [ ] Implement ToolBar with the shared docking/content/ordering behavior.
  Choose wxAUI or a custom host for multiple bars and arbitrary widget/layout
  content; wxFrame's single managed toolbar slot is insufficient.

## Shared-core dependencies and input

- [ ] Apply shared retained-view input updates and nested reconciliation failure
  recovery to wx scheduling and native ownership.
- [ ] Add shared Binding conversion, external-change subscriptions, batching and
  observable collections, including non-string TextField models and value-based
  Checkbox bindings.
- [ ] Add raw ComboBox item values and mixed/custom Table/Tree values under the
  shared conversion/storage contract.
- [ ] Add remaining keyboard/button/modifier and typed input/change payloads,
  including Canvas pointer callbacks and Tree drag/drop. Preserve acceptance
  behavior and callback lifetime through redraw/removal.
- [ ] Define borrowed native-widget embedding and remounting after external
  destruction, with explicit window/sizer ownership and host-runtime lifetime.

## Remaining validation

- [ ] Verify non-GTK wx ports on Windows/macOS, including native ownership,
  dialogs, candidate popups and shutdown. Verify Cocoa custom-colour control
  remounts and physical input, and Windows Stack mixed-control transparency,
  focus and hit testing on native hosts.
- [ ] Verify physical input, window-manager behavior, focus traversal and
  clipboard/drag-selection across controls.
- [ ] Verify Chinese IME composition and accessibility/screen-reader behavior.
- [ ] Verify layout, fonts and Canvas coordinates on HiDPI/mixed-density displays
  and across themes and scrolling configurations.

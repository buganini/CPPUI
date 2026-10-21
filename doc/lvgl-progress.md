# LVGL remaining alignment work

This file tracks only open gaps against the shared CPPUI/PUI contracts for
LVGL 9.6.0. Remove resolved items; keep change history and completed verification
results out of this document.

## Text and fonts

- [ ] Implement interactive text selection for Label/Text.
- [ ] Implement formatted HTML and Markdown rendering; the current wrappers
  display raw source.
- [ ] Add runtime font loading, family selection, font-weight handling and
  Unicode fallback beyond the compiled Montserrat subset. Define exact size and
  display-density behavior for widgets and Canvas text.

## Containers and data widgets

- [ ] Add Scroll `.scrollX()`/`.scrollY()` and the shared signed-zero end anchor,
  including declarative offset retention through content replacement, growth,
  shrinkage and viewport resize.
- [ ] Support mixed forced/automatic scrollbar policies. LVGL exposes one
  scrollbar visibility mode per object; choose an adapter or custom scrollbar
  implementation for the currently rejected combination.
- [ ] Add optional bound selection to Tabs, including cleared/unknown selection
  and model updates from native tab changes.
- [ ] Implement Splitter with resizable panes and retained sash positions, or
  make an explicit scope decision to omit it.
- [ ] Implement Tree and Tree(Adapter) against the shared typed adapter contract,
  including hierarchy, expansion, selection, editing and retained model identity.

## Drawing and desktop interactions

- [ ] Add LVGL ImageResource, `loadImage()` and Canvas `drawImage()`. Define source
  ownership, decoder/file access and behavior for cropping, scaling, opacity,
  replacement and missing images.
- [ ] Add remaining Canvas pointer callbacks after the shared input-event
  contract is defined.
- [ ] Define LVGL mappings or explicit exclusions for modal overlays, dialogs,
  menus/toolbars, MDI and desktop Window commands. Port applicable shared recipes
  with the shared result/selection contracts.
- [ ] Define and implement inbound file/text/URL drops for desktop LVGL hosts,
  including acceptance, filtering and callback lifetime.

## Runtime, ownership and interop

- [ ] Add explicit application quit and host-driven event-loop pumping. Specify
  tick, delay, idle and host ownership responsibilities.
- [ ] Route SDL2 `SDL_QUIT` through ordered application cleanup instead of the
  driver's direct shutdown/exit path. Cover pending work, repeated quit and
  shutdown from callbacks without destroying a borrowed host runtime.
- [ ] Define host-facing focus-group and keyboard/encoder ownership for embedded
  and externally supplied input devices.
- [ ] Define scoped content roots, screen switching, multiple displays and
  remount behavior after external native deletion. Active-screen content still
  assumes exclusive use; arbitrary shared-screen/native-object embedding needs
  an explicit ownership contract.
- [ ] Define scheduler completion and error delivery for background submissions,
  cancellation and shutdown while keeping all LVGL access on the UI thread.

## Shared-core dependencies

Coordinate these gaps with the [shared/Qt plan](pyside6-progress.md); do not add
competing LVGL-specific public contracts.

- [ ] Recover from nested builder failures after native reconciliation starts.
- [ ] Deliver changed inputs to retained, parameterized subviews.
- [ ] Add Binding conversion/validation helpers, external-change subscriptions
  and notification ownership. TextField numeric conversion and typed
  input/change payloads depend on these contracts.
- [ ] Define batched mutation and unchanged-value notification behavior.
- [ ] Add observable list/map updates, element bindings and value-based Checkbox
  overloads under the shared collection contract.
- [ ] Define remaining keyboard/button/modifier event payloads and arbitrary
  Table/Tree cell-value adapters.

## Remaining validation

- [ ] Verify physical SDL2/X11 pointer, wheel and keyboard interactions, clipped
  controls, focus traversal and page changes across multiple display sizes.
- [ ] Verify real IME composition, accessibility, themes and HiDPI layout/text
  rendering; compile-time font selection and dummy-display tests do not cover
  these behaviors.
- [ ] Verify rendering, input and teardown on an embedded display/input target.
- [ ] Verify supported desktop hosts beyond Linux.
- [ ] Investigate X11 application/cookbook startup crashes under Xvfb.

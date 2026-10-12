# Declarative Components
|Generic|Qt|lvgl|wx|
|-------|-------|----|--|
|[Application](#application)|QApplication|View|App|
|[Window](#window)|QMainWindow|screen_active|wxFrame|
|[HBox](#box-layouts)|QHBoxLayout|flex_flow|wxBoxSizer|
|[VBox](#box-layouts)|QVBoxLayout|flex_flow|wxBoxSizer|
|[Divider](#divider)|QFrame|—|—|
|[Splitter](#splitter)|QSplitter|—|—|
|[Label](#label)|QLabel|label|wxStaticText|
|[Button](#button)|QPushButton|button|wxButton|
|[ProgressBar](#progressbar)|QProgressBar|—|—|
|[Text / Html / MarkDown](#text)|QLabel|—|—|
|[Canvas / ImageResource](#canvas)|QWidget / QPainter / QImage|—|—|

Node IDs and Qt button text copy their string arguments, including temporary
`std::string` values. An empty explicit ID is distinct from no ID; `id(nullptr)`
clears the ID. Factory return values are borrowed construction references: use
fluent modifiers immediately, and keep application state rather than node
references across redraws.

Qt, LVGL and wx native updates participate in the common retirement protocol.
Internal forwarding is valid only while reconciliation still owns the old
declaration; the old declaration is then reclaimed. This does not extend the
lifetime of borrowed factory references.

Reconciliation compares type and explicit ID within the matched parent/view
scope. Placed grid nodes also include row, column and both spans in identity.
Grid declarations without both coordinates are discarded and reclaimed. Reversed
containers preserve native order across repeated redraws; out-of-order children
do not consume ordinary sibling offsets. Non-movable nodes are never requeued
to make room for a neighboring move. Terminal children remain unmounted and
are reclaimed when their declaration is replaced.

Construction ranges unwind their scope on normal completion, `break`, `return`,
and exceptions. If a top-level `content()` throws before reconciliation starts,
the previous declaration tree remains mounted and incomplete declarations are
released. Recovery from failures during native reconciliation is still pending.

Qt applications create a `QApplication` when none exists and otherwise borrow the
existing one, which must outlive the CPPUI application. Application teardown
releases declarations and its native roots before destroying an owned Qt
application. Virtual Qt views do not own a native widget. Qt widgets/layouts use
typed raw native observers, clear them on parent-driven deletion, and disconnect
only their own signal connections. Redraw retains other application listeners.
A button invocation copies its callback and clears the focused widget before
calling user code, matching PUI's focus/commit ordering. Focus-out handlers may
synchronously redraw; the active callback remains valid and the next click uses
the updated callback. External native
subtree deletion is safe for teardown; automatic remount after that deletion is
not yet supported.

State reads during a successful build replace that view's dependencies. Values
that are no longer read stop triggering redraws. Assigning, incrementing, or
decrementing any State during a view builder throws
`StateMutationInViewBuilderError` before changing its value. Mutate state in event
handlers or application code outside construction. Redraw remains synchronous
until the queued completion/error contract is chosen.

## Application

Qt `Application::quit()` queues event-loop exit so the current handler can return
first. It can be called before `run()`; application destruction cancels any
pending quit. Quitting a borrowed QApplication exits its shared event loop but
does not destroy it. Application operations, state mutation and redraw belong on
the GUI thread; marshal background work there before accessing CPPUI state.

## ProgressBar

Include `cppui/qt/progressbar.hpp` and use `ProgressBar(progress, maximum = 1.0)`.
Both numbers are multiplied by 100 and truncated toward zero to match PUI:
`ProgressBar(0.125, 0.25)` sets native value 12 and maximum 25. Redraw updates
retained widgets. A zero maximum uses Qt's busy indicator; other range behavior
follows `QProgressBar`. Non-finite or unrepresentable values throw
`std::out_of_range` during construction. A state read passed as an argument
subscribes the surrounding view; this component does not require a writable binding.

## Box layouts

Qt boxes use 11 pixels of outer padding when their physical parent is not another
box layout, and zero padding when nested in a box. Native QBoxLayout margins are
zero to avoid adding a second layer of spacing. Configurable padding and weights
still await the common layout API.

## Divider

Include `cppui/qt/divider.hpp` and use `Divider()`. Its sunken QFrame is horizontal
inside a VBox, vertical inside an HBox (including through virtual views), and
frameless otherwise. Dividers are terminal and non-movable; removal releases the
native frame using the normal deferred-deletion lifecycle.

## Canvas

Include `cppui/qt/canvas.hpp`. A painter receives a `CanvasPainter&` valid for
that invocation. It supplies drawing operations and the current logical `width`,
`height`, and `pixel_density`; it is separate from the borrowed node declaration.

```cpp
CPPUI::Canvas([](CPPUI::CanvasPainter & canvas) {
    canvas.drawRect(10, 10, 50, 40, 0xCCCCCC, 0xFF00FF, 2);
    canvas.drawLine(0, 0, 80, 60, "yellow");
    canvas.drawCubicBezier({10, 80}, {25, 20}, {75, 80}, {90, 20}, 0x00FFFF, 2);
});
```

Callbacks can return void or bool. Returning true requests another native paint;
false/void waits for an ordinary update. Read reactive State during `content()`
and capture its value in the painter so the surrounding view tracks the read.
Redraw reuses the native widget and replaces its painter. Active callbacks are
copied before invocation, and queued painting becomes inert on removal.

Drawing includes `drawText`, `drawLine`, `drawPolyline`, `drawCubicBezier`,
`drawPolygon`, `drawRect`, `drawEllipse` and `drawImage`. Coordinates are logical
pixels; each operation restores painter state. Colors accept `0xRRGGBB`, color
names or strings such as `"#ff0000"`. Use `{}` for an absent color: shape fill and
stroke are disabled when absent; text keeps the current pen. Rectangles take
two corners, ellipses take center/radii, and coordinate sequences use
`std::vector<CanvasPoint>` or initializer lists.

`drawText(x, y, text, w = -1, h = -1, size = 12, color = {}, rotate = 0,
anchor = Anchor::LEFT_TOP)` uses canvas dimensions for negative w/h and keeps
the native font for negative size. `Anchor` has the reference's nine positions;
it selects the text rotation pivot while x/y is the unrotated text origin.

`ImageResource::load(path)` owns its image data. `crop(x, y, width, height)` and
`scale(width, height, keepAspectRatio = true, quality = 0)` return new values;
zero quality means fast scaling, other values smooth. Missing files produce an
empty resource (`isNull()`); drawing it does nothing. Image resources can also
be included independently through `cppui/qt/image_resource.hpp`.
`drawImage(image, x = 0, y = 0, width = -1, height = -1, src_x = 0, src_y = 0,
src_width = -1, src_height = -1, opacity = 1)` infers negative dimensions from
the source region. The resource exposes `width()` and `height()`.

Paint contexts must not escape their callback. As with native Qt paint handlers,
callbacks must not throw through Qt, run nested event loops, or synchronously
destroy the painting widget or its ancestors. Framework redraw/removal may run
during the callback; normal deferred native deletion occurs after it returns.
Input/drag/drop events, shared layout/style modifiers and Shapely integration
remain pending. The cookbook uses a private size/background adapter meanwhile.

## Text

Include `cppui/qt/text.hpp` for `Text(text, selectable = false)`,
`Html(text, selectable = false)`, and `MarkDown(text, selectable = false)`.
They own their strings and use a wrapping, top-aligned QLabel with explicit
plain, rich, or Markdown format. Text and selection flags update on retained
widgets. `Label(text, selectable = false)` also supports mouse text selection,
while keeping its literal plain-text format and normal single-line layout.

## Splitter

Include `cppui/qt/splitter.hpp` and use `Splitter(vertical = false)` as a child
construction scope. It defaults to horizontal; `Splitter(true)` is vertical.
Redraw updates orientation while retaining the native splitter and its panes.
Unchanged redraw preserves user-adjusted pane sizes. Keyed children can move,
be inserted or removed, including flattened virtual subviews. Nested splitters
are supported; removed panes follow the normal deferred-deletion lifecycle.

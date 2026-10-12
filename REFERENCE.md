# Declarative Components
|Generic|Qt|lvgl|wx|
|-------|-------|----|--|
|[Application](#application)|QApplication|View|App|
|[Window](#window)|QMainWindow|screen_active|wxFrame|
|[HBox](#box-layouts)|QHBoxLayout|flex_flow|wxBoxSizer|
|[VBox](#box-layouts)|QVBoxLayout|flex_flow|wxBoxSizer|
|[Divider](#divider)|QFrame|—|—|
|[Label](#label)|QLabel|label|wxStaticText|
|[Button](#button)|QPushButton|button|wxButton|
|[ProgressBar](#progressbar)|QProgressBar|—|—|
|[Text / Html / MarkDown](#text)|QLabel|—|—|

Node IDs and Qt button text copy their string arguments, including temporary
`std::string` values. An empty explicit ID is distinct from no ID; `id(nullptr)`
clears the ID. Factory return values are borrowed construction references: use
fluent modifiers immediately, and keep application state rather than node
references across redraws.

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

## Text

Include `cppui/qt/text.hpp` for `Text(text, selectable = false)`,
`Html(text, selectable = false)`, and `MarkDown(text, selectable = false)`.
They own their strings and use a wrapping, top-aligned QLabel with explicit
plain, rich, or Markdown format. Text and selection flags update on retained
widgets. `Label(text, selectable = false)` also supports mouse text selection,
while keeping its literal plain-text format and normal single-line layout.

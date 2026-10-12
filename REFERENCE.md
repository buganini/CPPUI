# Declarative Components
|Generic|PySide6|lvgl|wx|
|-------|-------|----|--|
|[Application](#application)|QApplication|View|App|
|[Window](#window)|QMainWindow|screen_active|wxFrame|
|[HBox](#hbox)|QHBoxLayout|flex_flow|wxBoxSizer|
|[VBox](#vbox)|QVBoxLayout|flex_flow|wxBoxSizer|
|[Label](#label)|QLabel|label|wxStaticText|
|[Button](#button)|QPushButton|button|wxButton|

Node IDs and Qt button text copy their string arguments, including temporary
`std::string` values. An empty explicit ID is distinct from no ID; `id(nullptr)`
clears the ID. Factory return values are borrowed construction references: use
fluent modifiers immediately, and keep application state rather than node
references across redraws.

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
A button invocation copies its callback before calling user code so synchronous
redraw/removal cannot destroy the callable while it is executing. External native
subtree deletion is safe for teardown; automatic remount after that deletion is
not yet supported.

# Declarative Components

This table records CPPUI implementations, not the full capabilities of each
toolkit. `—` means **not yet implemented in CPPUI**. It does not mean the toolkit
cannot provide the feature. Partial implementations and specific native limits
are described in the component sections and backend plans.

|Generic|Qt|lvgl|wx|
|-------|-------|----|--|
|[Application](#application)|QApplication|View|App|
|[Window](#window)|QMainWindow|screen_active|wxFrame|
|[HBox](#box-layouts)|QHBoxLayout|flex_flow|wxBoxSizer|
|[VBox](#box-layouts)|QVBoxLayout|flex_flow|wxBoxSizer|
|[HVBox / VHBox](#adaptive-box-layouts)|QBoxLayout|flex_flow|wxBoxSizer|
|[Grid / Stack](#grid-and-stack)|QGridLayout / QStackedLayout|Native grid / shared native grid cell|wxGridBagSizer / wxPanel + overlay wxSizer|
|[Spacer](#spacer)|QSpacerItem|transparent flex item|wxSizer stretch spacer|
|[Divider](#divider)|QFrame|flex-axis separator|wxStaticLine|
|[Scroll](#scroll)|QScrollArea|scrollable object (basic)|wxScrolledWindow|
|[Splitter](#splitter)|QSplitter|—|nested wxSplitterWindow|
|[Label](#label)|QLabel|label|wxStaticText / read-only wxTextCtrl|
|[Button](#button)|QPushButton|button|wxButton|
|[TextField](#textfield)|QLineEdit|single-line lv_textarea|wxTextCtrl|
|[MaskedField](#maskedfield)|QLineEdit password mode|lv_textarea password mode|wxTextCtrl password style|
|[ComboBox / ComboBoxItem](#combobox)|QComboBox|dropdown / textarea|wxComboBox|
|[RadioButton](#radiobutton)|QRadioButton|checkbox (radio mode)|wxRadioButton|
|[Checkbox](#checkbox)|QCheckBox|checkbox|wxCheckBox|
|[ProgressBar](#progressbar)|QProgressBar|bar / busy animation|wxGauge|
|[Text / Html / MarkDown](#text)|QLabel|wrapping label / raw markup fallbacks|wxStaticText / wxHtmlWindow / raw text fallback|
|[Canvas](#canvas)|QWidget / QPainter|canvas / software drawing|wxPanel / wxGraphicsContext|
|[Image](#image)|QLabel / QPixmap|lv_image / owned decoded pixels|wxStaticBitmap / wxImage|
|[Menus / Toolbars](#menus-and-toolbars)|QMenuBar / QMenu / QToolBar / QAction|—|wxMenuBar / wxMenu / wxMenuItem (no toolbar yet)|
|[MDI](#mdi)|QMdiArea / QMdiSubWindow|—|—|
|[TimelineView](#timelineview)|QTimer / virtual node|timer / virtual node|wxTimer / virtual node|
|[ImageResource](#canvas)|QImage|—|wxImage|
|[Tabs / Tab](#tabs)|QTabWidget / QWidget|tabview / virtual node|wxNotebook / wxPanel|
|[Views](#views)|QStackedWidget|Retained containers|wxSimplebook|
|[Table / TableNode](#table)|QTableView / QAbstractTableModel|lv_table + cell editors|wxGrid / wxGridTableBase|
|[Tree / TreeNode](#tree)|QTreeView / QAbstractItemModel|—|wxDataViewCtrl / wxDataViewModel|
|[Modal](#modal)|QDialog|—|—|
|[Dialogs](#dialogs)|QFileDialog / QMessageBox / QInputDialog|—|wxFileDialog / wxMessageDialog / wxTextEntryDialog|

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

wx applications create a wxApp when needed and otherwise borrow the initialized
host runtime. `Wx::View::sync()` mounts immediately; State changes queue and
coalesce redraws. Removed views cancel pending handlers and queued exceptions
are reported to stderr. Native event contexts transfer on reuse and snapshot
callbacks, including callbacks which synchronously remove themselves. Controls
are destroyed after dispatch; sizer ownership and child-window ownership are
released separately. External native deletion clears observers; automatic
remount remains outside the supported contract. HBox/VBox use native sizers,
PUI expansion priorities, four-sided padding and explicit weight overrides.
Omitted dimensions, colors and fonts reset on redraw. Label supports selectable
read-only text; changing that native creation mode remounts it. Window supports
one physical content child, title/icon/size and initial maximize and
fullscreen. Use a layout to group content. Only Linux/GTK is runtime-verified.

LVGL views, including standalone views, release their owned declarations. Native
objects remain parent-owned; deletion clears the primary observer and secondary
handles such as Button's label. Native event contexts transfer on reuse without replacing listeners during
active dispatch; teardown detaches only CPPUI's own event callbacks. Active click callbacks are copied before invocation; exceptions are
reported to stderr at the native event boundary. External native deletion is
safe for teardown, but does not establish automatic subtree remount support.
Window borrows the display-owned screen and only clears its content.

State reads during a successful build replace that view's dependencies. Values
that are no longer read stop triggering redraws. Assigning, incrementing, or
decrementing any State during a view builder throws
`StateMutationInViewBuilderError` before changing its value. Mutate state in event
handlers or application code outside construction.

Qt `redraw()` requests one update on the GUI event queue; repeated requests and
State writes coalesce. `sync()` updates that view immediately and propagates
exceptions to its caller. Tests can use `sync()` for a particular view, or process
Qt events to settle subscribed subviews and lifecycle notifications. A request
made during reconciliation schedules a follow-up, never a nested update.
`setup()` runs once before the first build and may initialize State.
`run()` synchronizes immediately before starting the event loop.

Queued render exceptions are reported to stderr. A failed declaration build
keeps the mounted UI and the view can retry on a later request; failures after
native reconciliation starts still need a recovery contract. Removing a view
cancels its queued work. Core and LVGL redraws remain synchronous.

## Optional arguments

Independent widget options use named fluent setters in C++11, corresponding to
PUI's keyword arguments: `Button("Save").disabled()` and
`TextField(model).placeholder("Enter text")`. Required values and model bindings
remain constructor arguments. Existing calls keep their defaults without adding
positional placeholders or overloads for every combination of options.

Setters copy their values into the current declaration and apply them during
reconciliation. A fresh declaration that omits a setter restores its default;
repeated calls on one declaration use the last value. Reading a State in a
setter, such as `.disabled(!canSave)`, tracks the view's reactive dependency.
Call widget-specific setters before common modifiers such as `.id()` or
`.layout()`, which return `Node&` rather than the concrete widget type.
The terminal `.click(callback)` call returns void and goes last.

Button, Checkbox, RadioButton and ComboBox all use `.disabled(bool = true)` on
Qt, wx and LVGL. The previous `.enabled(...)` setter has been removed; migrate
`.enabled(value)` to `.disabled(!value)`. Disabled selection controls still
follow bound model changes without firing click/change callbacks.

Use `Optional<T>` when absence has its own meaning, such as an unset dimension,
an unselected page or a cancelled dialog. A defaulted option does not itself
need `Optional<T>`: `disabled` defaults to false, and an omitted or empty
placeholder both mean no hint. Existing positional layout/style modifiers keep
their `Optional<T>` arguments and `{}` omission syntax below.

`Optional<T>` stores its value inline in aligned union storage plus a presence
flag. An empty Optional does not construct `T`; holding, copying or moving a
scalar value requires no separate heap allocation. The Optional reserves room
for `T` even while empty. Strings, containers and other payloads can still
allocate their own contents.

Copies own independent values. A successful move empties the source; transfer
uses a copy when `T` has a potentially throwing move and supports copying.
Move operations are `noexcept` only when the selected transfer constructor is
non-throwing. Assignment first constructs its incoming value, then replaces the
old one: failure to construct the incoming value preserves the destination;
failure during the final replacement leaves the destination empty and usable.
Payload pointers/references do not follow moves to another Optional and must
not be retained across replacement, reset or destruction.

## PUI modifiers

Nodes expose PUI's `.layout(...)`, `.style(...)` and `.qt(...)` APIs. These are
ports of [PUI's node modifiers](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/PUI/node.py)
and [Qt parameter application](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/PUI/PySide6/base.py).
C++ supports typed named arguments in any order on Qt, wx and LVGL:

```cpp
namespace layout = CPPUI::layout;
namespace style = CPPUI::style;

CPPUI::Label("Hello")
    .layout(layout::width(200), layout::weight(1), layout::padding(8, 12))
    .style(style::color(0xffffff), style::bgColor(0x222222), style::fontSize(16));

CPPUI::Label("Body").style(style::fontFamily("Sans"));
```

The layout argument helpers are `width(int)`, `height(int)`, `weight(int)`,
`padding(...)` and `margin(...)`. The style helpers are `color(int)`,
`bgColor(int)`, `fontSize(int)`, `fontWeight(int)` and `fontFamily(const char*)`.
`padding`/`margin` accept an `Insets` or one to four integers in the order
described below. `fontFamily` borrows a null-terminated string without copying
its contents. Use a string literal or keep the caller's buffer alive at the same
address for the entire node declaration lifetime, including while retained or
hidden. For a caller-owned `std::string`, pass `.c_str()` and keep that string
alive without operations that invalidate its buffer. Do not pass a temporary
string's `.c_str()`.

For font family, `nullptr` means omitted and `""` means an explicit empty value.
Neither argument form copies or owns the supplied text; native toolkits may
copy it when applying the style. Owning results such as dialog text still use
`OptionalText` (`Optional<std::string>`).

Each helper produces a distinct argument type. The modifier dispatches by type,
without string keys, maps or runtime key lookup; numeric and inset argument
wrappers require no heap allocation. Node fields use inline `Optional<T>`
storage, so numeric and inset modifiers need no additional heap allocation in
either argument form. Font family is stored as a nullable borrowed pointer.
Wrong argument domains (such as `style::color` in `.layout()`) fail to compile.
Named and positional arguments cannot be mixed within one call, but separate
calls can use either form. Repeated keys apply left to right, with the last value
winning. A negative dimension/weight/font size rejects the whole call before
changing any fields.

The existing positional arguments follow the Python keyword order; `{}` omits
an argument. The names below describe the order, not C++ assignment syntax:

```cpp
.layout(width = {}, height = {}, weight = {}, padding = {}, margin = {})
.style(color = {}, bgColor = {}, fontSize = {}, fontWeight = {}, fontFamily = nullptr)
.qt(const CPPUI::QtOptions & options)
```

Sizing follows each component's defaults and its parent's native layout rules
unless overridden. Qt and wx TextField controls expand horizontally at their
native height; Spacer consumes flexible space along its HBox/VBox's main axis.
LVGL uses component sizes and native flex layout, with VBox filling unspecified
child widths. Exact sizes depend on the toolkit, platform, font and theme.

Use `.layout()` for intentional overrides: width and height supply native size
hints or dimensions, weight controls flexible space along the parent's main
axis, and padding adds spacing. Start with the defaults for ordinary controls;
see the backend mappings below when a design needs more control.

Dimensions, weight, colors and font size/weight use `Optional<int>`; padding and
margin use `Optional<Insets>`, and font family uses a borrowed `const char*`.
Qt option maps are copied. Repeated calls merge supplied values on that declaration.
A fresh declaration without a formerly supplied property restores the native
default during redraw. Zero is an explicit value, including black (`0x000000`),
zero size, zero weight and `Insets(0)`. Dimensions/weights/font sizes reject
negative values. Modifiers return the borrowed `Node&` for immediate chaining.

```cpp
for(auto column: CPPUI::VBox().layout({}, {}, 1, CPPUI::Insets(12))) {
    CPPUI::Label("Heading").style(0x336699, {}, 20);
    CPPUI::Label("Body").layout(200, 40).style({}, {}, {}, {}, "Sans");
}
```

`Insets` accepts one value (all sides), two (vertical, horizontal), three
(top, horizontal, bottom), or four (top, right, bottom, left). With positional
arguments, pass an explicit `Insets(...)`; C++ cannot implicitly convert an
integer through both Insets and Optional. Named inset helpers avoid these
conversions. Both font-family forms accept a string literal directly.
**Reference limitations:** `margin` and `fontWeight` are stored but not applied
by the pinned PUI Qt implementation. CPPUI's Qt backend keeps that behavior;
other backends apply properties according to their native support.
Virtual/model-only nodes have no native size or appearance to modify.

`.wideLayout(minWidth, ...)` and `.wideStyle(minWidth, ...)` apply the same
positional or typed named arguments when the nearest non-virtual parent's
content width is at least `minWidth`. Unknown parent sizes do not match.
Reading that size subscribes the containing view, so resizing rebuilds the
declaration from its base settings and restores them below the threshold.
Calls apply in declaration order: the last matching supplied value wins,
regardless of threshold order. Omitted values leave earlier settings unchanged.
Font family retains the borrowed `const char*` contract above. These methods
keep node identity and evaluate directly during construction, without storing
a breakpoint table. Normal native layout sizing and overflow limits apply.

```cpp
CPPUI::Label("Title")
    .style(CPPUI::style::fontSize(12), CPPUI::style::color(0x777777))
    .wideStyle(600, CPPUI::style::fontSize(18), CPPUI::style::color(0x228855))
    .layout(CPPUI::layout::padding(2))
    .wideLayout(600, CPPUI::layout::padding(12));
```

Qt dimensions override preferred `sizeHint()`, not minimum/maximum dimensions;
a parent can allocate a different actual size. Explicit dimensions select the
Preferred size policy unless `.qt(...)` overrides it. Padding sets widget
contents margins; weight sets a physical HBox/VBox child's stretch, including
through a virtual view. Colors are RGB integers and font sizes are points.

Include `cppui/qt/options.hpp` (also included by Qt widget headers) for the
supported native options, with names matching PUI's keyword keys:

```cpp
CPPUI::QtOptions options;
options.HorizontalPolicy = QSizePolicy::Fixed;
options.VerticalPolicy = QSizePolicy::Preferred;
options.SizeConstraint = QLayout::SetMinimumSize;
options.StyleSheet = CPPUI::QtStyleSheet{{"background-color", "#eeeeee"}};
CPPUI::VBox().layout(200).qt(options);
```

`StyleSheet` is an optional map of CSS properties and overrides generic `.style()`
properties. Like PUI, a later `.qt(...)` call replaces a supplied StyleSheet map
as a whole; omission keeps it and an explicit `QtStyleSheet{}` clears it. Other
supplied Qt option fields merge individually. `SizeConstraint` applies to the
widget's native QLayout; requesting it on a widget without a layout throws. `Style` accepts a borrowed `QStyle*` for
widgets; it must outlive them. Omitting Qt options on a replacement declaration
restores defaults. Native layout pointers are named `qtlayout` to leave
`.layout(...)` available. Non-Qt nodes accept `.qt(...)` as a no-op; QtOptions
itself requires Qt headers.

LVGL dimensions set object sizes in pixels, weight sets flex grow, padding sets
the four native padding properties, and colors set text/background colors.
Omission restores prior local styles or theme defaults. `fontSize` selects the
nearest compiled Montserrat bitmap font, approximating points at 96 dpi; it
cannot produce arbitrary exact sizes. `fontFamily` accepts empty/default or
`"Montserrat"`; other families throw. Font data remains owned by the configured
LVGL runtime. Runtime font loading and display-density mapping remain pending.
wx applies layout sizing, padding/weight and font/color modifiers, resetting
omitted values to native defaults. `.qt()` is specific to the Qt backend.

## Application

Portable applications can include `cppui/backend.hpp` and select exactly one of
`CPPUI_BACKEND_QT`, `CPPUI_BACKEND_WX` or `CPPUI_BACKEND_LVGL` in the build.
The `CPPUI::qt` and `CPPUI::wx` CMake targets set their selector automatically.
For desktop LVGL, include `cmake/CPPUILVGLDesktop.cmake`, set
`CPPUI_LVGL_SOURCE_DIR` to the LVGL 9.6.0 checkout and call
`cppui_add_lvgl_desktop(lvgl_sdl2)` or `cppui_add_lvgl_desktop(lvgl_x11)`.
Its `CPPUI::lvgl` target supplies the selector, SDK, desktop configuration and
driver fixes. Both the cookbook and standalone examples use this setup.
Each backend owns its header selection, `CPPUI::Backend::View`/`Views` aliases,
display name and `CPPUI_HAS_*` feature flags. Use capability flags for optional
components; application code does not need native toolkit types or startup calls.

Each backend's `base.hpp` exposes a constant string: `CPPUI::Qt::backendName`
(`"Qt"`), `CPPUI::Wx::backendName` (`"wxWidgets"`) or
`CPPUI::LVGL::backendName` (`"LVGL"`). These are C++11 `constexpr char[]` values;
applications can use them in titles or diagnostics without creating a runtime.

Qt and wx `Application::title(text)` set the native application display name
when nonempty and supply the default title for Windows under that logical root.
`Window().title(text)` takes precedence, including an explicit empty string.
Without either title, a retained window keeps its native title. Like the other
application-wide properties, the display name also affects a borrowed runtime
and is not restored on teardown.

Qt `Application(argc, argv, iconPath)` optionally copies an icon path and sets
the application-wide default window icon once, during its first synchronization,
following PUI. Omitting the path (or passing an empty string) preserves a borrowed
QApplication's icon. An explicit path also applies to a borrowed QApplication;
it is an application-wide setting and is not restored on CPPUI teardown. Later
redraws leave native icon changes alone.

`.style(...)` and `.qt(...)` also support application-wide font/colors,
`StyleSheet` and `Style`. QApplication takes ownership of an explicitly supplied
`QStyle*`, unlike widget-level Style; do not delete it or share its ownership.
These global settings also affect a borrowed QApplication and remain after the
CPPUI Application is destroyed. Widget-only size policies and SizeConstraint
are rejected. Omitting application styling initially preserves a borrowed
application's existing stylesheet.

Qt `Application::quit()` queues event-loop exit so the current handler can return
first. It can be called before `run()`; application destruction cancels any
pending quit. Quitting a borrowed QApplication exits its shared event loop but
does not destroy it. Application operations, state mutation and redraw belong on
the GUI thread; marshal background work there before accessing CPPUI state.

LVGL `Application::sync()`/`run()` lazily initializes LVGL and creates an 800x600
SDL2/X11 display plus pointer/keyboard input when no display exists. Construction
alone does not open a window. `Application::title()` supplies a fallback title;
`Window::title()` overrides it for an application-owned desktop host. X11 builds
use `cmake/CPPUILVGLX11.cmake` for the pinned driver's lifecycle and window bridge.
`Window::size(width, height)` sets a positive pixel size for that owned desktop
window and its LVGL display. Unchanged declarations preserve a user's resized
window; changing the declared size applies the new dimensions. Omitting the
size preserves the current dimensions. Borrowed displays keep their resolution,
including embedded framebuffers; their owner controls sizing.
Application teardown destroys its declarations before its input, display and
owned LVGL runtime. An existing display/runtime is borrowed; its host, input and
focus group remain owned by the caller and must outlive the application.

LVGL `Application::start()` pumps timers while at least one LVGL display exists.
Closing the last display returns from the loop; closing one of several displays
keeps it running. Delays use LVGL's configured delay callback. Native deletion
invalidates CPPUI handles, so declaration teardown remains safe after a display
closes. The caller still owns input-device cleanup and `lv_deinit()`.

## Table

Include `cppui/qt/table.hpp`, `cppui/wx/table.hpp` or `cppui/lvgl/table.hpp`.
`Table(autofit = true)` contains row
`TableNode()` scopes, each containing cell `TableNode(value)` declarations.
Cells accept `std::string` and arithmetic values, `State<T>` or `Binding<T>`.
Bindings make a cell editable; an unbound cell can opt into editing with
`.set(std::function<void(const T &)>)`. An explicit setter does not override a
binding. Qt performs native scalar conversion; arbitrary user-defined values
remain deferred. External typed adapters are described below.

`.display(text)` (also accepting a string binding) overrides the displayed value
without changing the editor's raw value. `.choices(std::vector<T>)` supplies
values with default labels; `.choices(std::vector<std::pair<T, std::string>>)`
supplies separate labels in declaration order. Editors preserve value types and
duplicate labels, commit once on activation, and leave unknown values unselected. No selection does not overwrite the model. `.click()` and
`.dblclick()` use no-argument callbacks on each cell.

Qt, wx and LVGL cell declarations accept `.prefix(text)`, `.suffix(text)` and
`.placeholder(text)`. Prefix/suffix surround the resolved display text or choice
label, and appear beside the field while editing; the binding always stores the
raw value. Placeholders are native empty-field hints (Qt line/choice editors and
wx/LVGL text editors; wxChoice and LVGL dropdowns have no empty-field hint).
Omitted options reset to empty on a fresh declaration. Composite editor widgets own their labels and input, with keyboard
commit/cancel and choice activation routed through the outer editor.

Table's `.columnHeader(labels)` and `.rowHeader(labels)` show optional headers;
omitting them hides the headers on redraw. `.columnCount(n)` and `.rowCount(n)`
override inference, accepting zero and rejecting negatives. Otherwise counts
cover the longest row and any supplied headers. Ragged/missing cells are blank,
inert and not editable. Autofit resizes columns to contents after synchronization.

The native view/model/delegate are reused and owned by Qt. Each redraw resets
the model, invalidating its old indexes/selection as in PUI's declarative Table.
The model copies cell data/callbacks and never stores declaration pointers;
retirement releases old captures and removal disables edits before deferred
native deletion. Setters/clicks may redraw or remove their own table, and callback
exceptions are reported to stderr. Bindings still borrow their model storage.
Rows/cells are declaration-only children: use direct TableNode scopes, rather
than nested retained views or native widgets inside a table.

LVGL uses one native `lv_table` with an overlaid editor. Single-click selects and
invokes the cell callback; double-click or Enter starts editing. Text and numeric
cells use `lv_textarea`; booleans and choices use `lv_dropdown`. Enter or focus
loss submits, Esc cancels, and invalid numeric input leaves the model unchanged.
Choosing an option writes its raw value, including when labels are duplicates.
An unknown choice displays its raw value without selecting an option; leaving
without choosing preserves it. Redraw cancels an active draft before replacing
bindings/adapter snapshots. Native callbacks keep their state alive across
synchronous redraw or removal.

LVGL headers occupy the first row/column with a distinct background and scroll
with the cells. They are not frozen headers. Counts including headers are limited
to 65,535 per axis by native selection indexes. Autofit measures native text and
padding; `autofit=false` retains native column widths. Editor placement follows
native measured row heights and scrolling, using LVGL 9.6's private table geometry.
LVGL copies displayed strings into its native cells on redraw and after edits;
it does not provide a virtualized/on-demand adapter view. Plain-storage changes
still require a redraw to refresh other cells or tables.

Reference: [PUI Table](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/PUI/PySide6/table.py)
and [ragged-table cases](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/table_ragged.py).

### Table adapters

`cppui/table_adapter.hpp` provides PUI's `BaseTableAdapter<T>` interface with
`data`, `editData`, `setData`, `editable`, row/column counts and headers, and
`clicked`/`dblclicked` callbacks. Pass `std::shared_ptr<YourAdapter>` to
`Table(adapter, autofit = true)`. `T` is an arithmetic type or `std::string`;
`editData` defaults to `data`, cells default to read-only, and headers default to
`Col n` / `Row n`. Data, edit data, counts and headers are const methods; setters
and callbacks can mutate the source. Each backend converts edits back to `T` before
calling `setData`. Display and editor values remain separate.

Header methods return `OptionalText`: absent entries use any declaration header
fallback, and an axis with no supplied labels is hidden. An explicitly empty
string is a supplied blank label. Adapter counts determine the grid; declarative
row/column count overrides and TableNode children apply only without an adapter.

Tables and native callbacks retain the adapter with shared ownership. The adapter
must own its data or explicitly borrow storage that outlives it. This does not
change borrowed CPPUI factory returns. Callbacks copy their owning handle before
invoking application code, so an edit/click may redraw, replace or remove its
own table. Removal clears the model's adapter references before deferred native
deletion; inactive models cannot dispatch edits.

Each declaration reads adapter dimensions, headers, editability and cell values
during construction, tracking State dependencies and validating before mounting.
Native edit reads use the current adapter; Qt/wx also read it for display, while
LVGL retains display strings until redraw or an edit. Plain storage changes require
an explicit view redraw; State assignment schedules it automatically when the
adapter read that State during construction. Structural refresh resets the
native model and invalidates old indexes. Automatic collection observation and
arbitrary mixed/custom cell types remain separate work.

The [shared table adapter recipe](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/table_adapter.py)
is ported in `cookbook/table_adapter.cpp`: two tables share one matrix; its first
three rows are editable and its row headers are hidden. CPPUI assigns a copied
State matrix after editing, replacing PUI's nested mutation plus `emit()`.

## Tree

Include `cppui/qt/tree.hpp`. `Tree()` contains nested `TreeNode(text)` scopes;
labels accept `std::string`, `State<std::string>` or `Binding<std::string>`.
The native view has one read-only column and a hidden header. Virtual views can
group nodes without adding an item to the native hierarchy.

Tree supports `.expandAll()`, `.collapseAll()` and `.expandable(bool)`. During
construction these apply after the children synchronize; on a mounted current
declaration they act immediately. Omitting these operations preserves native
expansion and expandability, following PUI. TreeNode's `.click()`, `.dblclick()`,
`.expand()` and `.collapse()` take no-argument callbacks, replaced on redraw.

The Qt-owned model retains internal item records across matched declarations.
Qt indexes refer to those records, never to retired declarations. Reordering
keyed nodes within their matched parent scope preserves persistent indexes,
selection and expansion; removal invalidates their indexes. Moving between
different parent scopes creates new items under the normal reconciliation rules.
Model layout notifications publish a snapshot after reconciliation, with
persistent-index remapping. Independently redrawn subviews queue that refresh
on the GUI event loop; process events before querying their updated model.

Removal disables callbacks and releases item records and captures; native parent
deletion clears declaration observers. Callback exceptions are reported to stderr,
and callbacks may redraw or remove the tree. Factory returns remain borrowed
construction references. External adapters use the interface below; arbitrary
display value types and drag/drop payloads remain deferred.

Reference: [PUI Tree](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/PUI/PySide6/tree.py)
and [tree synchronization cases](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/tree_sync.py).

### Tree adapters

`cppui/tree_adapter.hpp` provides `BaseTreeAdapter<T>` with PUI's `parent`,
`child`, `data`, `rowCount`, `clicked`, `dblclicked`, `expanded` and `collapsed`
methods. `Handle` is `std::shared_ptr<T>`; it refers to application data, not a
CPPUI declaration. Getters are const methods, and `data` returns an owned
`std::string`, matching the existing TreeNode label contract.
Pass `std::shared_ptr<YourAdapter>` to `Tree(adapter)`; expansion modifiers work
as for the declarative tree.

A null handle denotes the invisible root. `child({}, row)` enumerates its visible
children; `rowCount({})` counts those children. Each non-root child's `parent`
must agree with the containing node. An optional application root above the
visible children stays invisible. Counts must be nonnegative, and visible data
objects must be non-null, unique and acyclic. Bad hierarchies throw during
construction, leaving the mounted tree unchanged. CPPUI derives native parent
indexes and their row numbers from this hierarchy; it does not copy PUI's
external adapter bug that always uses parent row zero.

The adapter and its data handles are retained through native callbacks. Own
children downward and keep parent links non-owning (the cookbook uses weak
parent links) to avoid data cycles. Removing an item releases its handles after
index remapping and active callbacks finish; removing/replacing the tree releases
the source. Factory-returned CPPUI declarations remain borrowed as before.

Each declaration snapshots the complete external hierarchy during construction,
then uses the existing Tree model and reconciliation to publish it. This is an
eager adapter, not lazy loading for very large trees. State reads in getters
subscribe the surrounding view. Plain data changes need an explicit redraw or
a State revision update. Stable source/data handles preserve indexes, selection
and expansion through same-parent reorder; removed nodes and replaced adapters
invalidate indexes. Moving a node to another parent follows the existing
reconciliation rule and creates a new item. Do not combine an adapter with
manual TreeNode children in the same Tree.

`cookbook/tree_adapter.cpp` ports [PUI's adapter recipe](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/cookbook/tree_adapter.py):
two views share the same application hierarchy, one starts expanded, and Add
item appends a subtree to both. Drag/drop remains tied to the deferred common
input-event contract. Both adapter interfaces are toolkit-independent; native
Table and its adapter cookbook run on Qt, wx and LVGL. Tree and its adapter
cookbook run on Qt/wx; the LVGL Tree wrapper is not yet implemented.

## Image

Include `cppui/qt/image.hpp` for `Image(path)`. The clickable image label owns its
path and pixmap, reuses the native label, and reloads when the path or file
modification time changes. It uses the image's natural size. Click replacement,
removal and native lifetime follow Label. A missing path throws during declaration
building, preserving the mounted image on a failed redraw; undecodable existing
files produce an empty Qt pixmap. `.layout(width, height)` scales the source
pixmap with its aspect ratio preserved; with one dimension, the other is inferred.
The original pixels remain cached so changing or omitting dimensions rescales
from the source. Positive weight enables native scaled contents. The shared
cookbook uses `.layout(400)` and PUI's public-domain Rijksmuseum windmill image.

Include `cppui/lvgl/image.hpp` for the same `Image(path)` API on LVGL 9.6.
The native image retains decoded pixels across redraws, visibility changes and
keyed moves. Width-only or height-only sizing infers the other dimension; two
dimensions fit the image inside that box with its aspect ratio preserved.
Positive weight enables native stretching into the allocated area. Omitting
dimensions/weight restores natural sizing and aspect-preserving drawing, subject
to the parent layout's allocation. PNG transparency and `.click()` are supported.

LVGL file images are decoded during declaration building. Missing, unsupported
or corrupt files throw before reconciliation, preserving the mounted tree.
Matching path, file identity, size and modification timestamp reuse an owned
source; replacement loads new pixels. Live images of the same unchanged file
share decoding, with only weak cache entries after removal. Native objects retain
the pixels until deletion, including parent-subtree teardown. Images occupy their
full decoded size in memory; no borrowed native-descriptor overload is exposed.

The SDL2/X11 configuration enables LVGL's bundled LodePNG and TJpgDec decoders
for PNG and baseline JPEG (`.jpg`/`.jpeg`). Other applications must enable suitable
decoders plus `LV_USE_FS_STDIO`, a nonzero `LV_FS_STDIO_LETTER`, and an empty
`LV_FS_STDIO_PATH` for the host-path API. Progressive JPEG and other formats need
additional LVGL decoders. This widget does not add Canvas `ImageResource` or
`drawImage`; those remain separate work.

## Menus and toolbars

Include `cppui/qt/menu.hpp` and `cppui/qt/toolbar.hpp`. `MenuBar()` contains
`Menu(text)` and `MenuAction(text)` scopes/actions; menus can nest.
`ToolBar()` contains `ToolBarAction(text, iconPath)` and ordinary widgets.
Actions use `.trigger(callback)` with no arguments. The path is optional;
omission retains a toolbar action's previous icon, following PUI.

MenuBar and ToolBar are out-of-order Window children and coexist with central
content and Modal. Native parents own menus/actions; keyed moves reuse them.
Callback replacement disconnects only CPPUI's connection, snapshots active
callbacks and reports trigger exceptions to stderr. Toolbar widget actions
retain their widgets across moves and are reclaimed when the widget is removed.
The cookbook gallery uses these public menu components.

## MDI

Include `cppui/qt/mdi.hpp` for `MdiArea()` and `MdiSubWindow()`. The area owns
native subwindows and preserves their geometry across unchanged redraws. Like
[PUI](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/PUI/PySide6/mdi.py),
a subwindow displays its first physical child; use a VBox/HBox to group content.
Additional children are hidden and parent-owned for safe teardown. Virtual views
flatten normally. Closing an explicit subwindow hides it without changing State;
a redraw does not reopen it.

A plain widget directly in an MdiArea uses Qt's automatic subwindow wrapper.
[Qt leaves that wrapper behind when removing only its content](https://doc.qt.io/qt-6/qmdiarea.html#removeSubWindow);
CPPUI also removes and deletes the empty wrapper. Keyed moves keep the content
alive while its native ownership transfers. External native deletion clears
observers for teardown; automatic remount after external deletion is deferred.

## TimelineView

Include `cppui/qt/timeline.hpp`. `TimelineView(ttlSeconds)` is a virtual scope:
its children occupy their normal parent slots, and a Qt timer requests a queued
redraw of the owning view. Reconciliation transfers the timer to the current
declaration; removal or root teardown cancels it. Following the pinned PUI
implementation, the interval is fixed when mounted, even if a later declaration
passes a different value; use a new identity to restart it. Intervals are finite,
nonnegative seconds rounded up to Qt milliseconds; zero runs when the event loop
is idle. The timer and view remain on the GUI thread.

LVGL uses the same `TimelineView(ttlSeconds)` scope via `cppui/lvgl/timeline.hpp`.
Its LVGL timer redraws the owning View synchronously on the UI thread. The
mounted interval is retained across redraws and keyed moves; a new identity
restarts it. Intervals round up to milliseconds, with a minimum of 1 ms for zero,
and must fit a signed 32-bit millisecond count. Removal, view teardown and native
parent/display deletion cancel the timer. Render exceptions are reported to
stderr without escaping into LVGL; subsequent ticks can retry. Mount the scope
inside a Window or native container, and release views before deinitializing LVGL.

## Window

Qt `Window().title(text).icon(path).size(width, height)` updates a retained native
window. Its native layout uses `SetNoConstraint`, so growing content stays within
the window instead of forcing it to expand beyond the declared or user-set size.
Text and paths are owned strings. An omitted title uses the nonempty root
Application title, or preserves the current native title if neither is supplied.
An explicit empty title clears it. Omitted/empty icons preserve the current icon.
An unchanged declared size preserves user resizing; changing it, or declaring it
again after omission, resizes the window. `.maximize()` and `.fullscreen()` apply
only at native creation, including for Modal. Calling either setter on a
mounted declaration stores a request without changing the live window.
Redraws preserve native state; use explicit native interop for live changes. A window mouse press clears a
focused line editor so it can commit. Keypress payloads remain deferred.

## Binding

The shared State Binding cookbook demonstrates an external string getter/setter
and the TextField change callback with a scrollable latest-100-message log.
It publishes a State revision after commit; getters never mutate State during
view construction.

`cppui/binding.hpp` provides a copyable C++11 `Binding<T>` with `get()` and
`set(value)`. Construct it from a `State<T>&`, or supply a getter and setter:

```cpp
CPPUI::Binding<int> binding(counter);
CPPUI::Binding<int> external([&model]() { return model.value; },
                            [&model](const int & value) { model.value = value; });
```

The adapter owns its callable captures and borrows the referenced State/model,
which must outlive its uses. State reads retain ordinary view dependency tracking;
writes retain State's mutation checks and request the subscribed views' redraws. Rebinding during a
build replaces those dependencies. Getters/setters are snapshotted for invocation,
so a setter may cause the declaration holding its adapter to be reclaimed.
Exceptions propagate to the caller; conversion/validation belongs in the setter.
External storage does not automatically notify views: redraw explicitly, or read
reactive State in the getter. Change subscriptions, explicit notification and
text conversion helpers remain pending; this adapter does not change State's
copy/move semantics.

## Button

Include `cppui/qt/button.hpp`, `cppui/wx/button.hpp` or `cppui/lvgl/button.hpp`.
`Button(text).click(callback)` owns its text and invokes the current callback.
`.disabled(bool = true)` controls native user interaction; the default is false. Disabling
and re-enabling reuse the native button; omitting the setter on a fresh
declaration restores enabled state. This follows PUI `d7dc23a`, with LVGL using
its native disabled state.

## TextField

Include `cppui/qt/textfield.hpp`, `cppui/wx/textfield.hpp` or
`cppui/lvgl/textfield.hpp` for `TextField(model, editModel)`. Both arguments
use `Binding<std::string>` or `State<std::string>&`; the draft binding is optional.
Typing updates the draft and `.input(callback)`, while focus loss/Enter commits
the model and calls `.change(callback)`. These callbacks take no arguments;
typed event payloads remain deferred. Redraw/rebinding preserves an active draft,
and commit uses the current binding. A successful commit reads the model back,
so an adapter that rejects or normalizes input controls the displayed value.

`.placeholder(text)` sets the native empty-field hint on Qt, wx and LVGL,
following PUI `184ffcd`. It owns the string, updates on redraw and clears when
given an empty string or omitted on a fresh declaration. Changing the hint
preserves the model, active draft and cursor and emits no input/change callbacks.
Visibility while focused follows the native toolkit. The shared cookbook's
State Model field uses `"Enter text"`.

Programmatic updates suppress input/change feedback. The draft is synchronized
when idle only if its value differs, avoiding repeated queued redraws. Callback
and setter failures report to stderr; a throwing setter leaves the draft visible.
Adapters own validation/conversion policy; generic numeric conversion and change
subscriptions remain undecided. The shared recipe currently demonstrates string
models and is marked partial for PUI's dynamic numeric conversion/event payloads.

LVGL uses a single-line `lv_textarea`. Enter (`LV_EVENT_READY`) or focus loss
commits an edited value once; Enter followed by focus loss does not commit it
again. Idle draft synchronization runs on LVGL's next timer pass, avoiding
reentrant reconciliation. Pending synchronization is cancelled when the field
is removed or its native object is deleted. Active drafts survive page hiding,
redraw and binding replacement. Setters and callbacks may remove their field or
destroy its owning view.

The SDL2/X11 cookbooks attach keyboard input to the default LVGL focus group.
Applications hosting LVGL must likewise create a default group before mounting
controls and attach their keypad input device to it; the TextField does not own
the host's input devices or group.

## MaskedField

The same backend `textfield.hpp` headers provide `MaskedField(model, editModel)`.
It accepts the string bindings, optional draft, placeholder, input/change
callbacks and layout options of TextField. Qt uses QLineEdit password echo, wx
uses wxTE_PASSWORD and LVGL uses textarea password mode. Models and draft
bindings contain the original text; native rendering masks it. Switching between
TextField and MaskedField replaces the native control. The shared cookbook shows
single-model and buffered passwords, lengths and a clear button on all backends.

## Scroll

Include `cppui/qt/scroll.hpp`. `Scroll(vertical, horizontal)` takes
`Optional<bool>` policies: absent means as-needed, true always-on, false off.
Defaults are vertical as-needed and horizontal off. Policies are applied only
when the native widget is created, following PUI. Scroll displays its first
physical child, with a layout used to group content. Additional children stay
hidden and native-parent-owned; content replacement detaches surviving widgets
before QScrollArea can delete them. Like PUI's add/remove contract, removing the
first child does not automatically promote an unchanged ignored sibling. Use one
layout child and change its contents, or give replacement content a new identity.

`.scrollX(position)` / `.scrollY(position)` use finite doubles: positive zero
means start anchoring, and negative zero (`CPPUI::ScrollEnd`, also
`Qt::Scroll::END`) means end anchoring. Range changes restore the saved distance
from that edge. As in the pinned reference, pre-sync captures the current native
position, and a reused Scroll retains its previous anchor/offset rather than
reapplying newly declared offsets. Therefore these methods set anchor/offset
metadata, not an unconditional immediate jump; pre-sync can overwrite their
numeric offset before child layout, including on initial construction.

User movement within ten pixels of an edge switches anchoring; if both edges
are that close, the current anchor is kept. Resize handling reads the mounted
native child, never a retired/new declaration. Axes declared disabled in the
current declaration fit the child's size hint, even though scrollbar visibility
policies are initial-only. All observers belong to the native scroll widget and
end with it.

For basic LVGL scrolling, include `cppui/lvgl/scroll.hpp`. `Scroll(vertical = {},
horizontal = false)` contains ordinary native children and supports fixed viewport
sizes through `.layout(...)`; a disabled axis fits unspecified child dimensions
to the viewport. False disables an axis, true forces its scrollbar, and `{}`
allows automatic visibility. LVGL has one scrollbar mode for both axes, so
forcing one axis while leaving the other automatic throws instead of silently
changing the policy. Independent mixed modes, `.scrollX()`/`.scrollY()` and signed
end anchoring remain unimplemented. This public component serves the gallery
sidebar; the full Scroll recipe is still Qt-only.

## ComboBox

Include `cppui/qt/combobox.hpp`, `cppui/lvgl/combobox.hpp` or `cppui/wx/combobox.hpp`.
`ComboBox(editable, model)` accepts an integer index binding or a string value
binding; `ComboBox(editable, index, text)` accepts both. State values adapt
automatically. Its children are
`ComboBoxItem(label, value)`; value defaults to label and may explicitly be empty.
Labels and values are owned strings, with labels used as default node IDs.
Use explicit IDs to distinguish duplicate labels or retain a renamed item.

`.disabled(bool = true)` defaults to false and follows PUI `d7dc23a`. Disabling preserves
selection, editable/non-editable mode and native identity; programmatic model
updates still apply without change callbacks. Omitting it on a fresh declaration
restores enabled state.

Index binding takes precedence for displayed selection. Negative/out-of-range
indices produce no selection; an index-change callback for -1 writes empty text,
never the last item's value. A text-only binding matches stored values. Editable
selection displays the item's label. Typing free text clears the index binding
to -1 and writes the text binding; with no text binding, the native widget retains
the edit buffer. Unbound selection follows the selected item's key through
reorders and label changes, including items in independently redrawn subviews.

LVGL uses a dropdown for non-editable selection and a textarea with a dropdown for
editable input. Typing filters suggestions by label prefix (ASCII case-insensitive)
while retaining original indices/raw values. Opening the arrow restores the full
list. Empty lists disable the selector, leaving the editor usable. Labels
containing newlines display on one
line because LVGL uses newlines as option separators. Switching editable mode
remounts the native control. Create a default LVGL focus group before mounting
controls to enable keyboard navigation.
`.change(callback)` has no arguments. A selected item's label-change signal is
ignored, so selection writes the stored value and calls the callback once.

Unchanged declarations preserve user edits; changing/reordering items refreshes
labels/data and reapplies bound selection without feedback writes. Native
callbacks read owned item values and snapshot bindings before user code. Raw
non-string values and typed event payloads remain deferred.

## RadioButton

Include `cppui/qt/radiobutton.hpp` or `cppui/lvgl/radiobutton.hpp` for
`RadioButton(text, value, model)`, accepting
`Binding<T>` or `State<T>&`. Values are copied and must support equality.
Construction reads the model for dependency tracking; native clicks commit the
current value before the current `.click()` callback. Programmatic updates do
not write the model. Native ownership and callback replacement follow Checkbox.

PUI uses Qt's automatic exclusivity among radio buttons sharing a physical
parent. Separate layouts form separate groups. This also preserves a reference
quirk: setting the model to an unmatched value after selection can leave the
last radio checked, because Qt refuses to uncheck its only selected button.
An initially unmatched model has no selected radio.

LVGL uses its checkbox widget with native radio-button mode and a circular
indicator. Radio buttons sharing a physical parent form an exclusive group;
use the same model within that group and separate layouts for independent
groups. An unmatched model clears selection. Native Up/Right key selection
commits the value without emitting `.click()`; clicking an already selected
radio keeps it selected and commits once. Focus-group setup belongs to the host.

## Checkbox

Include `cppui/qt/checkbox.hpp` or `cppui/lvgl/checkbox.hpp` and use `Checkbox(text, Binding<bool>)` or pass
`State<bool>&` directly. Model reads during construction subscribe the view;
programmatic updates reuse the native control without writing the model back.
User changes write once through the current adapter, and `.click(callback)` runs
after the checked value changes. Rebinding replaces the old dependency and
setter. Callback captures are released on redraw/removal; setter exceptions are
reported to stderr at the native boundary.

List/map membership can be expressed as `Binding<bool>` getters/setters. The
shared Checkbox recipe matches PUI by adding/removing list values or dictionary
keys, assigning a modified copy to State to notify observers. Automatic nested
collection observation and a third value argument remain pending.

## ProgressBar

Include `cppui/qt/progressbar.hpp` or `cppui/lvgl/progressbar.hpp` and use
`ProgressBar(progress, maximum = 1.0)`.
Both numbers are multiplied by 100 and truncated toward zero to match PUI:
`ProgressBar(0.125, 0.25)` sets native value 12 and maximum 25. Redraw updates
retained widgets. A zero maximum uses Qt's busy indicator; other range behavior
follows `QProgressBar`. Non-finite or unrepresentable values throw
`std::out_of_range` during construction. A state read passed as an argument
subscribes the surrounding view; this component does not require a writable binding.

LVGL also uses integer hundredths. With a positive scaled maximum, its native bar
clamps progress to `[0, maximum]`. A maximum that truncates to zero runs a moving
segment animation; unchanged redraws preserve its position. Returning to a
positive maximum stops the animation and applies the declared progress. Native
deletion also cancels it. Negative maxima and non-finite or unrepresentable
numbers throw `std::out_of_range` before changing mounted UI. The bar uses the
LVGL theme and does not draw a percentage label.

## Box layouts

Qt and LVGL boxes use 11 pixels of outer padding when their physical parent is
not another layout, and zero padding when nested. Native QBoxLayout margins are
zero to avoid adding a second layer of spacing. Use `.layout(...)` to override
padding/dimensions and set each child's weight. Redraw updates stretch and resets
omitted properties, including children rebuilt by independent virtual views.
LVGL boxes use a six-pixel gap and stretch children across the other axis unless
that dimension is explicit; dividers retain their one-pixel thickness.

## Grid and Stack

`cppui/qt/layout.hpp` includes `Grid()` and `Stack()` (also available through
`grid.hpp` and `stack.hpp`). Grid uses the existing node fields `grid_row`,
`grid_column`, `grid_rowspan` and `grid_columnspan`. Both coordinates must be
nonnegative; absent/zero spans use one cell. Grid coordinates/spans participate
in reconciliation identity, so changing them replaces that cell. A layout is
required to group several controls in a cell. The cookbook assigns these public
placement fields directly.

Qt Stack uses StackAll mode and reverses initial declaration order, placing
the last declared layer on top. All layers share the available geometry.
Unchanged redraws retain the raised layer. Insertions/removals and keyed moves
let QStackedLayout choose its current layer, as in PUI; moving the current layer
can raise its neighbor. Grid and Stack share HBox/VBox's nested
padding rules. PUI ignores Spacer inside these two containers; CPPUI also skips
its native insertion and explicitly reclaims the unmounted item.

LVGL exposes `Grid()` through `cppui/lvgl/layout.hpp` or `grid.hpp`
(`LV_USE_GRID` required), with the same coordinate/span fields and replacement
rules. Automatic axes measure visible cells and include their margins. Single
cells establish each track's size; any additional space needed by a spanning
cell is shared across its tracks. This also handles grids containing only
spanning cells, which native `LV_GRID_CONTENT` alone does not measure. Children
retain their natural size along automatic axes. Allocated axes use equal
fractional tracks, stretching cells across their assigned rows/columns and
gaps, less margins. Explicit Grid dimensions, parent allocation and Grid flex
weight select allocated axes; child flex weights do not set track proportions.
The default row/column gap is 6 pixels. Nested padding follows HBox/VBox.
Hidden cells retain native state without contributing to track sizing; removed
or hidden trailing cells allow automatic dimensions to shrink. Unplaced
declarations are reclaimed, and placed Spacers are ignored by layout/input.
The shared Grid cookbook runs on both SDL2 and X11.

LVGL exposes `Stack()` through `cppui/lvgl/layout.hpp` or `stack.hpp`, using
one native grid cell shared by all layers (`LV_USE_GRID` required). The last
declared layer draws on top and receives overlapping pointer input first;
keyed moves update that order while retaining native widgets and editor state.
Hidden layers retain their state and do not contribute to automatic sizing or
pointer hit testing. Automatic axes fit the largest visible layer plus padding
and margins, with children retaining their natural size at the padded origin.
Axes allocated by an explicit size, a parent layout or flex weight stretch
layers to the available space, less each layer's margins. Resetting an axis to
automatic, hiding or removing a large layer allows it to shrink again.
Stack shares HBox/VBox's nested padding rules; Spacer is a zero-size ignored
layout item and does not intercept input. Both SDL2/X11 run the shared Stack
cookbook.

## Spacer

Include `cppui/qt/spacer.hpp` or `cppui/lvgl/spacer.hpp` (also included by each
backend's `layout.hpp`) and use `CPPUI::Spacer()` inside an HBox or VBox:

```cpp
for(auto row: CPPUI::HBox()) {
    CPPUI::Label("Start");
    CPPUI::Spacer();
    CPPUI::Label("End");
}
```

The invisible, non-interactive item fills the space remaining along its physical
parent's layout axis. Multiple spacers share it equally. The parent must have
available space; a container sized only to its content supplies no extra space.
Resize and retained redraw recompute the gap. Like
[PUI's Spacer](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/PUI/PySide6/layout.py),
the declaration is terminal and non-movable; matching positions reuse their
native objects. Native deletion clears its observer, and declaration teardown
detaches that observer before a parent deletes its children. Qt uses QSpacerItem
with Expanding policy on the box axis and Preferred on the other axis. Its
layout owns mounted items; direct removal deletes detached items, and layout
destruction invalidates the declaration's observer. `.layout({}, {}, weight)`
changes the default weight of one. Fixed-size spacing remains unimplemented
for Qt's non-widget spacer, as in the pinned PUI reference.

## Divider

Include `cppui/qt/divider.hpp` and use `Divider()`. Its sunken QFrame is horizontal
inside a VBox, vertical inside an HBox (including through virtual views), and
frameless otherwise. Dividers are terminal and non-movable; removal releases the
native frame using the normal deferred-deletion lifecycle.

For LVGL, include `cppui/lvgl/divider.hpp`. The same terminal, non-movable
declaration draws a one-pixel gray separator across its physical flex parent: a
horizontal line in a column, a vertical line in a row, and no line outside flex
layouts. It has no interactive behavior; native parent deletion reclaims it.

## Dialogs

Include `cppui/qt/dialog.hpp`. These functions run a modal Qt dialog and return
after it closes, following [PUI's dialog wrappers](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/PUI/PySide6/dialog.py):

| Function | Arguments in order (defaults) | Result |
| --- | --- | --- |
| `OpenDirectory` | title (`"Open Directory"`), directory (`""`) | `OptionalText` |
| `OpenFile` | title (`"Open File"`), directory (`""`), types (`""`) | `OptionalText` |
| `OpenFiles` | title (`"Open Files"`), directory (`""`), types (`""`) | `Optional<std::vector<std::string>>` |
| `SaveFile` | default value (absent), title (`"Save File"`), directory (`""`), types (`""`) | `OptionalText` |
| `Information`, `Warning`, `Critical` | message (function name), title (function name + `" Dialog"`) | `void` |
| `Confirm` | message (`"Confirm"`), title (`"Confirm Dialog"`) | `bool` (true only for Yes) |
| `Prompt` | prompt (`"Input"`), title (`"Input Dialog"`), default value (`""`) | `OptionalText` |

File filters accept `|` separators, for example `"Text files (*.txt)|All files (*)"`.
SaveFile uses its default value as the initial path when directory is empty;
it accepts an OptionalText or string. Strings and results use UTF-8.

`Optional<T>` is a C++11 value type in `cppui/optional.hpp`; `OptionalText` aliases
`Optional<std::string>`. Default construction means no value. Test it with
`if(result)` or `hasValue()`, then use `value()`, `*result`, or `result->...`.
Empty access throws `std::logic_error`. `valueOr(fallback)` supplies a default;
`reset()` releases the value. Copies own independent values; moves transfer them.

```cpp
auto result = CPPUI::Prompt("Name", "Edit name");
if(result) {
    // Accepted, including an empty string. Cancellation has no value.
    name = *result;
}
```

All cancelled file/directory dialogs return no value. This intentionally
normalizes PUI's empty-string directory cancellation to the agreed Optional
contract. Message dialogs own and release their native dialog before returning.
Call dialog helpers on the GUI thread from event handlers, outside builders.
Native desktop dialog integration still needs real-display validation; automated
tests exercise Qt's widget dialogs, including their nested event loops.

## Modal

Include `cppui/qt/modal.hpp` and declare it inside an Application or QtView.
`Modal(status, offValue = T{})` accepts a `State<T>&`
or `Binding<T>`, with a value convertible to bool. True shows a modal QDialog;
false hides it. Choose an off-value that converts to false. For example:

```cpp
for(auto window: CPPUI::Modal(modalOpen).title("Modal Window").size(400, 240)
        .open([]() { /* opened */ }).close([]() { /* closed */ })) {
    CPPUI::Button("Close Modal").click([&]() { modalOpen = false; });
}
```

Following [PUI's Modal](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/PUI/PySide6/modal.py),
the declaration is physical and out of order: it does not consume a box,
splitter, tab-page, or Window central-content slot. Its zero-margin VBox groups
ordinary children. The nearest physical Qt parent owns the native dialog;
top-level dialogs are application roots. Redraw retains the dialog and replaces
its binding and callbacks. Native close, Escape/reject, and accept write the
current off-value; hiding through the binding does not perform another write.
Removing the declaration hides and detaches it without writing the borrowed
model during teardown.

`.open(callback)` and `.close(callback)` notify once per visibility transition,
including close on declaration removal. Notifications snapshot the callback at
the transition and run on the owning view's Qt event context after reconciliation,
so handlers can mutate State or remove the modal. Destroying that view cancels
pending notifications and releases their captures; teardown does not invoke
callbacks into a destroyed model. Exceptions from notifications or native-close
binding writes are reported to stderr rather than escaping through Qt. Bound
State changes request a queued redraw.

`.title(text)` owns UTF-8 text; omission preserves the current title. `.size(w,h)`
resizes only when the declared size changes, preserving a user's resize across
unchanged redraws. Omitting and later restoring the size declares it anew.
`.maximize(bool = true)` and `.fullscreen(bool = true)` set initial flags during
construction. Calls on mounted declarations do not change live native state.
Initial flags are not reapplied on redraw; native interop can change live state. Keep state, not borrowed node
references, between redraws. Window-manager behavior still needs real-display
validation beyond the automated offscreen tests.

## Tabs

Include `cppui/qt/tab.hpp`, `cppui/lvgl/tab.hpp`, or `cppui/wx/tab.hpp` for the
selected backend. Use normal construction scopes:

```cpp
for(auto tabs: CPPUI::Tabs()) {
    for(auto first: CPPUI::Tab("Tab 1").id("first")) {
        CPPUI::Label("Content 1");
    }
    for(auto second: CPPUI::Tab("Tab 2")) {
        CPPUI::Label("Content 2");
    }
}
```

Qt follows [PUI's selection contract](https://github.com/buganini/PUI/blob/65f2c02/PUI/selection.py):
`Tabs()` keeps an internal selection; `Tabs(Binding<OptionalText>)` or
`Tabs(State<OptionalText>&)` accepts an external model. Use
`Tabs(Side::BOTTOM, model)` to combine placement and binding. The selection key is a Tab's
non-empty `.id()`, falling back to its label. The first matching key wins.
Native selection and `.select(key)` write through the current model. Binding
updates, insertion, removal and reordering do not write back during reconciliation.

An absent or unknown key displays the first tab without changing the external
model; an empty container selects no page. The internal model initially records
the first available tab's key. Clearing it with `.select()` displays the first tab;
unknown keys remain stored and select a matching page if it returns. Empty-string
keys are allowed for Tabs, including a Tab with an empty label. As with Views,
`select()` must use the current mounted declaration, not a saved factory reference.

Qt `Tab(label)` is a physical, zero-padding VBox page. Empty Tabs still have a
page, and multiple controls or virtual subviews stay together on that page.
Direct children of Tabs must be Tab declarations; invalid children are rejected
before native reconciliation. Keyed moves and selection changes preserve native
pages, local view state and controls. Removed pages are reclaimed normally.

`Tabs(tabposition = Side::TOP)` accepts the shared `CPPUI::Side` enum:
`TOP`, `RIGHT`, `BOTTOM`, and `LEFT` (`cppui/side.hpp`). Raw strings are no
longer accepted. Qt and LVGL update native placement on redraw while preserving
pages; wx recreates its notebook when the side changes.

LVGL still uses virtual Tab declarations: each physical descendant creates a
native tabview page, so group multiple controls in an explicit VBox or HBox.
Empty virtual Tabs create no page. Its legacy label update uses direct declaration
indexes: two physical children in Tab("A") followed by one in Tab("B") display
A, B, B. Keyed moves retain child widgets and selection; removing the selected
page clamps its index. Empty tabviews retain their native container. The optional
selection API remains to port to LVGL. The optional
selection API also remains to port to wx; wx already owns one panel per Tab.

## Views

Include `cppui/qt/views.hpp`, `cppui/wx/views.hpp` or `cppui/lvgl/views.hpp`.
`Views()` owns an internal selection, initially empty;
`Views(Binding<OptionalText>)` or `Views(State<OptionalText>&)` uses an
external model. Every direct child must have a unique, non-empty `.id()`.
Each child receives a native page, including virtual subviews that render zero
or multiple controls. Switching pages preserves native controls, view instances,
and local state. Keyed reordering retains pages; removal destroys them.
Qt uses QStackedWidget with a VBox page per child. wx uses wxSimplebook with a
wxPanel and zero-padding VBox sizer per child, so empty and multi-root subviews
remain a single page there too. Both reserve an empty native page for no selection.
LVGL uses a retained VBox container per child and hides every page when selection
is empty or unmatched. Each page fills the Views container.

Set the binding or call `.select("editor")` on the current mounted Views
declaration. `.select()` or an empty `OptionalText` clears the selection.
Unknown keys display an empty page without changing the model; adding the
matching page later shows it. Empty strings are rejected. Validation and page
grouping happen before native reconciliation, so invalid IDs preserve the
mounted content.

Unlike Python's retained handles, CPPUI factory references are borrowed and
must not be saved across redraws. Keep state or resolve the current declaration
from the owning view when invoking `select()`. The cookbook demonstrates both
internal selection and an optional State model with retained editor subviews.
The Views cookbook page is enabled for Qt, wx, SDL2 and X11. Its TextField and
Change text button update retained local state on every backend.

## Canvas

Include `cppui/qt/canvas.hpp`, `cppui/wx/canvas.hpp` or `cppui/lvgl/canvas.hpp` for the selected backend.
A painter receives a `CanvasPainter&` valid for
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

All three backends include `drawText`, `drawLine`, `drawPolyline`, `drawCubicBezier`,
`drawPolygon`, `drawRect` and `drawEllipse`; Qt and wx also have `drawImage`. Coordinates are logical
pixels. Colors accept `0xRRGGBB`, color names or strings such as `"#ff0000"`.
Use `{}` for an absent color: shape fill and stroke are disabled when absent;
text uses the frame's default foreground. Rectangles take
two corners, ellipses take center/radii, and coordinate sequences use
`std::vector<CanvasPoint>` or initializer lists.

Qt caches pens/brushes and wx caches graphics pens/brushes/fonts within each
paint invocation. Each primitive chooses its own styles; text and images restore
their temporary transforms and opacity changes. Qt restores native painter state
when the `CanvasPainter` is destroyed, including during exception unwinding.
wx uses a fresh graphics context for each widget paint; its graphics pen, brush
and font wrapper objects are not part of the native graphics-state stack.
Caches do not retain resources across frames or widgets. The newer PUI guards
that skip repeated pen/brush setters were measured separately in C++; they are
not adopted because the native gains were small or inconsistent.
See the [benchmark decision](doc/canvas-performance.md).

All three `CanvasPainter` implementations expose `push()` and `pop()` inside the
paint callback. Qt saves/restores full native state (pen, brush, font, transform,
clip and opacity), including nested scopes. wx saves/restores the most recently
selected CPPUI graphics pen and brush, initially null; it does not save transforms,
clipping or fonts. When constructing a painter around a preconfigured borrowed wx
graphics context, choose its styles through CPPUI before pushing: wx provides no
getters for arbitrary existing graphics pens/brushes. LVGL scopes are compatibility
no-ops because every recorded command owns its style.

Balance scopes within the callback; Qt/wx throw `std::logic_error` for an unmatched
`pop()`. Qt unwinds any still-open native scopes at frame teardown, including on
exceptions. Native access is not intercepted: finish raw drawing and pop before
resuming CPPUI helpers. A native operation should set its own styles explicitly.
CPPUI painter objects remain borrowed for this invocation only.

`drawText(x, y, text, w = -1, h = -1, size = 12, color = {}, rotate = 0,
anchor = Anchor::LEFT_TOP)` uses canvas dimensions for negative w/h and keeps
the native font for negative size. `Anchor` has the reference's nine positions;
it selects the text rotation pivot while x/y is the unrotated text origin.

The LVGL 9.6 backend uses an owned ARGB8888 buffer at native pixel resolution
(`pixel_density == 1`). Native size changes replace the buffer and request a
frame; the initial size is 100x100 until a parent/layout overrides it. Its
software renderer approximates curves and ellipses with line segments and uses
even/odd polygon fills, including concave and self-intersecting polygons.
Pixel-identical Qt antialiasing and stroke joins are not guaranteed.

LVGL text uses the object's native bitmap font, scaled to `size` pixels of line
height (Qt uses font points); negative size preserves the native font. Text
rotation and all nine anchors are supported. Colors accept RGB integers,
`#RGB`, `#RRGGBB`, `#AARRGGBB`, the 16 basic HTML color names, `grey`, `magenta`,
`cyan`, `orange`, and `transparent`, case-insensitively. Other names throw
`std::invalid_argument`; use RGB values for portable colors outside that set.
Font loading, LVGL ImageResource/drawImage, and display-density scaling remain
pending. Basic drawing needs LVGL Canvas and its software drawing support,
without ThorVG or another vector graphics dependency.

On Qt, `ImageResource::load(path)` owns its image data. `crop(x, y, width, height)` and
`scale(width, height, keepAspectRatio = true, quality = 0)` return new values;
zero quality means fast scaling, other values smooth. Missing files produce an
empty resource (`isNull()`); drawing it does nothing. Image resources can also
be included independently through `cppui/qt/image_resource.hpp`.
`drawImage(image, x = 0, y = 0, width = -1, height = -1, src_x = 0, src_y = 0,
src_width = -1, src_height = -1, opacity = 1)` infers negative dimensions from
the source region. The resource exposes `width()` and `height()`.

Paint contexts must not escape their callback. As with native Qt paint handlers,
Qt callbacks must not throw through Qt, run nested event loops, or synchronously
destroy the painting widget or its ancestors. Framework redraw/removal may run
during the callback; normal deferred native deletion occurs after it returns.
LVGL records each callback's drawing commands before native rendering. Redraw
or removal during the callback discards that obsolete frame safely. Exceptions
are caught at the LVGL timer boundary and reported to stderr; a throwing user
callback leaves the previous frame intact. False/void pauses the paint timer;
true requests a frame on the next LVGL refresh interval. Teardown cancels the
timer and releases captures, and the native object retains its buffer until
deletion, including when an ancestor owns that deletion. Run callbacks on the
UI thread without nested event loops or display/runtime teardown.
Use `.layout(width, height).style({}, backgroundColor)` for the canvas size and
background; the shared cookbook uses `.layout(100, 100).style({}, 0x555555)`.
Qt, wx and LVGL provide `.mousemove(callback)`, `.mouseleave(callback)` and
`.wheel(callback)`. Move/wheel callbacks receive `const CanvasPointerEvent&`
from `cppui/canvas_input.hpp`; leave takes no arguments. `x`/`y` are local logical
pixels. `angleX`/`angleY` preserve native wheel rotation units (normally 120 per
notch); Qt also supplies native `pixelX`/`pixelY` when available, while wx and LVGL leave
pixel deltas zero. Positive vertical deltas point up. These two delta units are
separate; consumers may use direction when exact distance is unnecessary.
Callbacks are replaced on redraw, omitted callbacks are cleared, and dispatch
copies the callable before invocation so it may redraw/remove its own Canvas.
Exceptions are reported to stderr. A wheel handler consumes the event; without
one, native scrolling can handle it. Other pointer events and modifiers remain to port. Qt/wx also support the
portable drag-enter/drop contract below.

LVGL input uses native hit testing, including clipping and overlays. SDL2 pointer
wheel samples (`LV_SDL_MOUSEWHEEL_MODE_CROWN`) and X11 encoder samples map one
vertical step to 120 angle units;
window exit clears hover. The cookbook installs the LVGL 9.6 input bridge through
`cmake/CPPUILVGLInput.cmake`. Other hosts must include that file and call
`cppui_lvgl_input_bridge(lvgl "${LVGL_SOURCE_DIR}")`
after creating their LVGL target. It builds corrected source copies without
changing the SDK checkout or replacing driver callbacks. Create input devices
before mounting Canvas, or redraw after adding devices. Input callbacks run on
the UI thread; they may redraw/remove Canvas, but must not destroy the input
device, display or LVGL runtime during native input dispatch.

Canvas `.showFps(bool = true)` displays the rate of completed native draws, with
text updates at most twice per second. It defaults to false and resets when
omitted on redraw. The overlay itself does not request another frame. FPS
measures Canvas drawing, not compositor presentation. HeavyCanvas includes
"Run continuously (FPS)" and "Full-frame changes" controls. The latter changes
explicit command colors each frame while preserving geometry and data.

The Qt/wx/LVGL HeavyCanvas cookbook uses these callbacks for hover inspection and
cursor-anchored wheel zoom over 1,000–1,000,000 synthetic sessions. It draws OHLCV,
Bollinger bands, K/D and RSI and provides regenerate/reset controls. Data is
repeatable for a seed within the C++ implementation; its random sequence is not
identical to Python's. SDL2 and X11 use the same recipe and exercise their native
input queues in the cookbook smoke test. Date labels are retained through
500,000 sessions; larger datasets use zero-based indices with thousands
separators (for example `999,999`). Every session is drawn: the largest presets
are deliberate rendering stress workloads.
**Won't do — Shapely and Matplotlib:** Python-only `Canvas.drawShapely` and
`MatplotlibCanvas` integrations are excluded from CPPUI and its cookbook.

## Label

Qt and LVGL `Label(text).click(callback)` dispatch a no-argument callback.
Qt uses mouse press, as PUI does, and commits the focused editor first.
Redraw replaces or clears the handler and preserves unrelated native listeners.
The callback may redraw or remove its own declaration. Label owns its literal
text. Qt supports mouse selection and resets omitted selection/cursor/handler
properties; interactive selection remains pending on LVGL. Typed input events
remain deferred on both backends.

## Text

Include `cppui/qt/text.hpp` for `Text(text, selectable = false)`,
`Html(text, selectable = false)`, and `MarkDown(text, selectable = false)`.
They own their strings and use a wrapping, top-aligned QLabel with explicit
plain, rich, or Markdown format. Text and selection flags update on retained
widgets. `Label(text, selectable = false)` also supports mouse text selection,
while keeping its literal plain-text format and normal single-line layout.

For LVGL, include `cppui/lvgl/text.hpp` and use `Text(text)`. It owns literal
text, wraps to its available width and recalculates its content height. Native
reuse updates the text. `selectable=true` throws `std::invalid_argument` because
interactive selection is not yet implemented in CPPUI's LVGL backend.
`Html` and `MarkDown` are raw-source fallbacks using the same wrapping Text widget.
Their factories emit call-site compiler warnings through `CPPUI_UNSUPPORTED`;
including the header alone is quiet. Their `selectable` argument has the same
restriction as Text. Formatted rendering remains unimplemented. The shared Text
cookbook page is enabled on SDL2/X11, marked partial, and displays a notice above
the examples identifying these fallbacks.

## Splitter

Include `cppui/qt/splitter.hpp` and use `Splitter(vertical = false)` as a child
construction scope. It defaults to horizontal; `Splitter(true)` is vertical.
Redraw updates orientation while retaining the native splitter and its panes.
Unchanged redraw preserves user-adjusted pane sizes. Keyed children can move,
be inserted or removed, including flattened virtual subviews. Nested splitters
are supported; removed panes follow the normal deferred-deletion lifecycle.

For wx, include `cppui/wx/splitter.hpp` and use the same `Splitter(vertical = false)`
scope. It follows PUI/wx's nested native two-pane splitters to support any number
of children, including zero and one. Each child has a native pane panel; widgets,
layouts and flattened virtual views are supported, with their padding and minimum
size applied inside the pane. A hidden child hides its content but retains its pane.

Unchanged composition preserves native sashes and user-adjusted positions.
Keyed insertion/removal/reordering or orientation changes rebuild the native
splitter chain and distribute space equally, matching PUI; retained panes and
their controls keep identity and editor drafts. Native sash gravity shares later
resize deltas across the panes. Independent virtual-view redraws also reconcile
the physical container. Native MDI and nonblocking Modal remain separate wx work.

## wx controls

wx HBox centers compact children vertically; VBox aligns compact children to
the left. Children requesting cross-axis expansion still fill that axis.
Omitted background colors preserve native defaults; removing an explicit
background resets native inheritance with wxNullColour. Tabs retain expansion
priority 2 on both axes, matching the current PUI/wx behavior.

wx uses the shared Binding/State signatures for Checkbox, RadioButton, TextField
and ComboBox. TextField preserves drafts during redraw, commits once on Enter
or focus loss, and commits a focused editor before button callbacks. Programmatic
updates do not emit input callbacks. TextField expands horizontally at its native
height with no forced minimum width; explicit layout dimensions override these
defaults and reset when omitted. RadioButton uses `wxRB_SINGLE`, with grouping
provided by its model as in PUI/wx. ComboBox preserves unbound keyed selection and
editable free text, maps labels to raw string values, and remounts when its native
editable style changes. Grid uses existing row/column/span fields. ProgressBar
uses integer hundredths, clamps to its range and treats a zero maximum as range
one, matching PUI/wx rather than promising a busy indicator.

Text supports plain multiline/read-only selectable text. Html uses wxHtmlWindow's
HTML subset; its selection behavior is native. Its best client size follows the
parsed content and refreshes when that content changes; it expands on both
layout axes, subject to sibling expansion priorities. MarkDown displays the
original source as plain multiline text, following PUI/wx. Its optional
`selectable` flag uses a read-only multiline text control. Markdown formatting
remains unimplemented.
The shared Text cookbook calls all three public factories directly.

The wx MarkDown factory has a `CPPUI_UNSUPPORTED` annotation from
`cppui/diagnostics.hpp`. GCC/Clang and MSVC emit a custom deprecation warning at
each call site, explaining the raw-text fallback. Including the header alone
does not warn. Normal builds still compile; projects treating deprecation warnings
as errors must allow this diagnostic explicitly if they want the fallback.

wx Scroll retains manual offsets and end-relative anchors when content changes.
Tabs uses wxNotebook with a physical wxPanel per Tab, following PUI/wx; each
page accepts one widget or layout. Keyed reordering retains the selected page.
Menus support nested menus and replaceable callbacks. TimelineView
uses a native wxTimer and stops it on removal. Standard dialogs use the shared
Optional results; accepted empty text is distinct from cancellation. Prompt and
paths convert explicitly through UTF-8. Scripted tests cover these interactions,
including modal acceptance/cancellation and callbacks that remove themselves.

wx Canvas provides the shared primitive, text, image, opacity and repeating-paint
API through wxGraphicsContext. Paint callback replacement/removal invalidates
repeat requests; exceptions are reported at the native event boundary. Coordinates
use native logical pixels and `pixel_density` reports content scale. ImageResource
supports loading, bounded cropping and aspect-preserving scaling; invalid sources
produce a null resource. Image widgets cache source path/mtime and scale to declared
dimensions. Native pixel checks and paint self-removal pass sanitizers and Valgrind.

wx Table and Tree accept the shared declarative nodes and typed adapter handles.
Table separates raw edit values from display text and choice labels, validates
scalar conversions and cancels editors when their model generation changes.
Tree uses model-owned records for native item IDs, retains keyed selection and
expansion across redraw/reorder, and suppresses callbacks during restoration.
Adapter reads happen during construction so failed getters preserve the previous
native UI. Callback snapshots allow synchronous removal; models release adapter
ownership on teardown. Native edits, retained state and adapter lifetimes pass
sanitizers and Valgrind on wxGTK.

wx Stack owns a native panel and overlays padded children at the same size and raises later children
above earlier ones. It uses native controls and their native background/hit-test
behavior; it does not rasterize the child hierarchy.

### Environment options

`CPPUI::Options()` returns a process-local `std::map<std::string, std::string>`
initialized from `PUI_OPTIONS` on first access. The environment syntax is
`#name=value#other=value` (the first `#` is optional). Names and values use UTF-8
percent encoding; `+` remains literal, and values remain strings. The last valid
entry wins. Malformed entries emit a warning and leave valid entries intact.
`loadEnvironmentOptions()` replaces the map from the current environment.
Initialize/reload on the UI thread before sharing values with workers. This is
host input only; Web URL subscriptions and AI IPC are excluded.

### Named node references

Declare `.ref("name")` on a node and look it up with `CPPUI::ref("name")` after
mounting. Names are process-local; lookup and `binder.node()` inspection belong
on the UI thread. Missing names throw `std::out_of_range`. Keep a binder value
instead of a borrowed declaration pointer when it must survive redraws.

`binder.select(OptionalText)` queues selection on Tabs/Views (Qt, wx and LVGL).
Qt/wx Window binders also provide `maximize(bool)`, `minimize(bool)` and
`fullscreen(bool)` for live changes. Declaration setters remain initial-only.
A binder obtained on the UI thread may submit commands from a worker; commands
never run synchronously. Qt/wx use their native queues; LVGL drains a protected
queue on its normal refresh timer without calling LVGL from the worker.

A binder follows matching reconciled declarations. Renaming a ref, changing
node identity/type, or removing its node expires that binder; already queued
commands are discarded, and new commands throw `std::runtime_error`. A missing
capability throws `std::logic_error`. Duplicate names resolve to the most recently
published node; removing an older owner does not erase the current registration.
A binder does not own a native widget. Join application workers before tearing
down the backend runtime. wx/LVGL Tabs retain their existing native selection
contract; their optional State selection models remain an older porting gap.

The Refs cookbook demonstrates queued selection and desktop window operations.
State-ref publishing for Web/AI inspection is excluded from this native sync.

### Portable inbound drops (Qt/wx)

`.dragEnter(callback)` and `.drop(callback)` receive a `CPPUI::DropEvent &`.
Use `.drop(callback, {".txt", ".json"})` to filter file suffixes (ASCII case
insensitive, including compound suffixes such as `.tar.gz`). A supplied list must
be nonempty. Include `"/"` to allow directories; an omitted filter accepts both
files and directories. Directories retain their original path and
`temporary=false`; use filesystem inspection to distinguish them. CPPUI does
not enumerate or copy their contents. One invalid file or disallowed directory
rejects the entire batch before dispatch. Text and URLs are unaffected by the
file filter. Disabled targets reject drops.

Events own `files` (`DropFile{name, path, temporary}`), optional `text`, `urls`,
optional local `x`/`y`, and an `action` string (`copy`, `move`, `link`). Native
files have `temporary=false`. Call `event.accept()` explicitly, or
`event.reject(reason)`. Returning without accepting rejects the drop; exceptions
reject and report to stderr. Acceptance never reads, copies, moves or deletes a
file. The Drop cookbook reads text files in its application callback.

Qt converts MIME data for widget and scroll-area viewport events. wx negotiates
file lists, Unicode text and URI lists; drag-enter only has coordinates/action
before data transfer. Native callbacks are replaced on redraw and cleared on
removal. Events can be copied for later use, without retaining toolkit events.
LVGL desktop file-drop bridging and browser uploads are not implemented here.
Tree-specific outbound drag/model reordering remains a separate porting gap.

### Optional filesystem policy

Qt/wx/LVGL applications accept `run(const LandlockPolicy &)` and
`start(const LandlockPolicy &)`. Default entrypoints remain unrestricted.
`LandlockPolicy` contains `read`, `write`, and `execute` path vectors. Linux
fails startup if the requested policy cannot be applied. See
[Landlock semantics and startup requirements](doc/landlock.md), including
`applyLandlock(policy)` for application-controlled early initialization.

## UI callbacks

On the UI thread, call `node->getPoster()` on a current mounted widget to obtain
an independent `CPPUI::Poster` (`cppui/poster.hpp`). A worker may copy it and call
`post(callback)` or `post(callback, delayMilliseconds)`; callbacks execute on the
UI loop. Delay defaults to zero, must be finite and non-negative, and is measured
from submission using a monotonic clock. Empty callbacks are rejected. Timers may
run late when the UI is busy; queued time is deducted from the requested delay.

Posters follow retained native widgets across reconciliation. Removing the target
cancels pending execution; posting to an expired target is a no-op. Callbacks must
still keep any other captured objects alive. Stop workers before destroying the
backend runtime. Qt uses queued dispatch and precise timers, wx uses CallAfter
and native timers, and LVGL drains its thread-safe queue on the UI timer loop.
The Refs cookbook demonstrates a 500 ms delayed increment.

## Visibility

`.visible(bool = true)` hides an ordinary widget or layout without removing its
node or native control. Hidden content takes no layout space, including its
padding/margin and box stretch, and receives no normal user input. Hidden
subtrees freeze child reconciliation. Initially hidden containers defer child
creation until shown; already mounted children retain their native state. State
changes in hidden views are refreshed when the ancestor becomes visible again.
Callbacks/timers on retained controls continue. Omission on a new declaration
restores visibility. Explicit Grid coordinates are retained.

Qt uses native widget visibility and collapses Spacer stretch; wx hides the
whole padded sizer item and preserves explicitly hidden descendants when an
ancestor is shown; LVGL uses its hidden flag. Container-specific native sizing
still applies (for example, Qt Stack computes its preferred size across layers).
Window, Tab, ComboBoxItem, TableNode and virtual views do not implement this
modifier; apply it to their physical content instead. The Label cookbook includes
a Show greeting checkbox.
## Reactive size and adaptive visibility

`node.size()` returns a read-only `SizeBinding`. `.get()` returns
`Optional<Size>`: empty before the first native layout measurement, then
`Size { width, height }` in native logical pixels. Reading the binding inside
`content()` subscribes that view to size changes. A saved binding follows the
retained node across redraws. Removing the node stops observations; the binding
can still read its last measurement. Observe physical widgets/layouts, not
virtual views or item declarations without a native surface.

Qt measures the content rectangle minus layout margins; Window uses its central
widget. wx measures a sizer's allocated size or a window's client size. LVGL
measures the content area excluding padding/borders. Measurements are coalesced
and published after layout; unchanged dimensions do not trigger another redraw.
Only nodes used by `.size()`, constrained `.visible()`, adaptive boxes or
`.wideLayout()`/`.wideStyle()` install observers.
`Window::size(width, height)` sets window dimensions (owned desktop displays for
LVGL), and Qt `Modal::size(width, height)` sets modal dimensions; zero arguments
observe the actual content size.

Use `VisibilityBounds` for PUI's optional width/height limits:

```cpp
for(auto scope: CPPUI::VBox()) {
    for(auto compact: CPPUI::VBox().visible(true, {{}, 600})) {
        CPPUI::Label("Compact layout");
    }
    for(auto wide: CPPUI::HBox().visible(true, {600})) {
        CPPUI::Label("Wide layout");
    }
}
```

The fields/constructor arguments are `minWidth`, `maxWidth`, `minHeight`,
`maxHeight`, each `Optional<int>`. `{}` omits a limit; zero is an explicit bound.
Minima are inclusive and maxima exclusive. Bounds refer to the nearest physical
parent's content size, skipping virtual ancestors. Before that size is known,
constrained branches remain hidden and their children are not mounted. Passing
`false` still hides the branch regardless of bounds. Showing it later restores
its retained controls and refreshes frozen nested views.

## Adaptive box layouts

`HVBox(int minWidth)` arranges children horizontally when the nearest physical
parent's content width is at least `minWidth`, and vertically otherwise.
`VHBox(int minHeight)` arranges them vertically when that parent's content
height is at least `minHeight`, and horizontally otherwise. Unknown measurements
use the second direction. Virtual views are skipped when choosing the parent.
Thresholds use native logical pixels and compare the parent's whole content
area, not the space remaining after siblings.

```cpp
for(auto box: CPPUI::HVBox(600)) {
    CPPUI::Label("Name");
    CPPUI::TextField(name);
    CPPUI::Button("Save");
}
```

Qt, wx and LVGL change the existing native container's direction, retaining its
children, editor drafts and cursor positions. Sizing and modifiers follow
HBox/VBox. Qt and LVGL dividers/spacers follow the new direction. Like PUI,
wx Divider keeps its creation-time line orientation. These layouts do not wrap
children into multiple rows. CPPUI has no Textual backend, so PUI's separate
terminal-cell APIs (`tsize`, `tLayout`, `tStyle` and terminal bounds) are not exposed.

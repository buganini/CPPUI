# Declarative Components
|Generic|Qt|lvgl|wx|
|-------|-------|----|--|
|[Application](#application)|QApplication|View|App|
|[Window](#window)|QMainWindow|screen_active|wxFrame|
|[HBox](#box-layouts)|QHBoxLayout|flex_flow|wxBoxSizer|
|[VBox](#box-layouts)|QVBoxLayout|flex_flow|wxBoxSizer|
|[Grid / Stack](#grid-and-stack)|QGridLayout / QStackedLayout|—|—|
|[Spacer](#spacer)|QSpacerItem|transparent flex item|—|
|[Divider](#divider)|QFrame|flex-axis separator|—|
|[Scroll](#scroll)|QScrollArea|—|—|
|[Splitter](#splitter)|QSplitter|—|—|
|[Label](#label)|QLabel|label|wxStaticText|
|[Button](#button)|QPushButton|button|wxButton|
|[TextField](#textfield)|QLineEdit|—|—|
|[ComboBox / ComboBoxItem](#combobox)|QComboBox|—|—|
|[RadioButton](#radiobutton)|QRadioButton|—|—|
|[Checkbox](#checkbox)|QCheckBox|checkbox|—|
|[ProgressBar](#progressbar)|QProgressBar|—|—|
|[Text / Html / MarkDown](#text)|QLabel|plain wrapping label|—|
|[Canvas](#canvas)|QWidget / QPainter|canvas / software drawing|—|
|[Image](#image)|QLabel / QPixmap|—|—|
|[Menus / Toolbars](#menus-and-toolbars)|QMenuBar / QMenu / QToolBar / QAction|—|—|
|[MDI](#mdi)|QMdiArea / QMdiSubWindow|—|—|
|[TimelineView](#timelineview)|QTimer / virtual node|—|—|
|[ImageResource](#canvas)|QImage|—|—|
|[Tabs / Tab](#tabs)|QTabWidget / virtual node|tabview / virtual node|—|
|[Modal](#modal)|QDialog|—|—|
|[Dialogs](#dialogs)|QFileDialog / QMessageBox / QInputDialog|—|—|

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

## Application

Qt `Application(argc, argv, iconPath)` optionally copies an icon path and sets
the application-wide default window icon once, during its first synchronization,
following PUI. Omitting the path (or passing an empty string) preserves a borrowed
QApplication's icon. An explicit path also applies to a borrowed QApplication;
it is an application-wide setting and is not restored on CPPUI teardown. Later
redraws leave native icon changes alone. General application options remain
subject to the shared options API.

Qt `Application::quit()` queues event-loop exit so the current handler can return
first. It can be called before `run()`; application destruction cancels any
pending quit. Quitting a borrowed QApplication exits its shared event loop but
does not destroy it. Application operations, state mutation and redraw belong on
the GUI thread; marshal background work there before accessing CPPUI state.

## Image

Include `cppui/qt/image.hpp` for `Image(path)`. The clickable image label owns its
path and pixmap, reuses the native label, and reloads when the path or file
modification time changes. It uses the image's natural size. Click replacement,
removal and native lifetime follow Label. A missing path throws during declaration
building, preserving the mounted image on a failed redraw; undecodable existing
files produce an empty Qt pixmap. Public dimension/stretch modifiers await the
shared layout API. The cookbook uses a private 360-pixel-height adapter and the
same screenshot as PUI's Image recipe.

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

## Window

Qt `Window().title(text).icon(path).size(width, height)` updates a retained native
window. Text and paths are owned strings. Omitted title/icon preserve their
current values, following PUI; an empty icon path also leaves the icon alone.
An unchanged declared size preserves user resizing; changing it, or declaring it
again after omission, resizes the window. `.maximize()` and `.fullscreen()` apply
only at creation when used in a builder. On a current mounted declaration they
change the native state immediately, as for Modal. A window mouse press clears a
focused line editor so it can commit. Keypress payloads remain deferred.

## Binding

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

## TextField

Include `cppui/qt/textfield.hpp` for `TextField(model, editModel)`. Both arguments
use `Binding<std::string>` or `State<std::string>&`; the draft binding is optional.
Typing updates the draft and `.input(callback)`, while focus loss/Enter commits
the model and calls `.change(callback)`. These callbacks take no arguments;
typed event payloads remain deferred. Redraw/rebinding preserves an active draft,
and commit uses the current binding. A successful commit reads the model back,
so an adapter that rejects or normalizes input controls the displayed value.

Programmatic updates suppress input/change feedback. The draft is synchronized
when idle only if its value differs, avoiding repeated queued redraws. Callback
and setter failures report to stderr; a throwing setter leaves the draft visible.
Adapters own validation/conversion policy; generic numeric conversion and change
subscriptions remain undecided. The shared recipe currently demonstrates string
models and is marked partial for PUI's dynamic numeric conversion/event payloads.

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

## ComboBox

Include `cppui/qt/combobox.hpp`. `ComboBox(editable, model)` accepts an integer
index binding or a string value binding; `ComboBox(editable, index, text)` accepts
both. State values adapt automatically. Its children are
`ComboBoxItem(label, value)`; value defaults to label and may explicitly be empty.
Labels and values are owned strings, with labels used as default node IDs.
Use explicit IDs to distinguish duplicate labels or retain a renamed item.

Index binding takes precedence for displayed selection. Negative/out-of-range
indices produce no selection; an index-change callback for -1 writes empty text,
never the last item's value. A text-only binding matches stored values. Editable
text writes its string binding. `.change(callback)` has no arguments; an editable
selection can emit both Qt index and text changes, as in PUI.

Unchanged declarations preserve user edits; changing/reordering items refreshes
labels/data and reapplies bound selection without feedback writes. Native
callbacks read owned item values and snapshot bindings before user code. Raw
non-string values and typed event payloads remain deferred.

## RadioButton

Include `cppui/qt/radiobutton.hpp` for `RadioButton(text, value, model)`, accepting
`Binding<T>` or `State<T>&`. Values are copied and must support equality.
Construction reads the model for dependency tracking; native clicks commit the
current value before the current `.click()` callback. Programmatic updates do
not write the model. Native ownership and callback replacement follow Checkbox.

PUI uses Qt's automatic exclusivity among radio buttons sharing a physical
parent. Separate layouts form separate groups. This also preserves a reference
quirk: setting the model to an unmatched value after selection can leave the
last radio checked, because Qt refuses to uncheck its only selected button.
An initially unmatched model has no selected radio.

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

## Grid and Stack

`cppui/qt/layout.hpp` includes `Grid()` and `Stack()` (also available through
`grid.hpp` and `stack.hpp`). Grid uses the existing node fields `grid_row`,
`grid_column`, `grid_rowspan` and `grid_columnspan`. Both coordinates must be
nonnegative; absent/zero spans use one cell. Grid coordinates/spans participate
in reconciliation identity, so changing them replaces that cell. A layout is
required to group several controls in a cell. The shared fluent placement API
is still undecided; the cookbook currently assigns these fields directly.

Stack uses Qt's StackAll mode and reverses initial declaration order, placing
the last declared layer on top. All layers share the available geometry.
Unchanged redraws retain the raised layer. Insertions/removals and keyed moves
let QStackedLayout choose its current layer, as in PUI; moving the current layer
can raise its neighbor. Grid and Stack share HBox/VBox's nested
padding rules. PUI ignores Spacer inside these two containers; CPPUI also skips
its native insertion and explicitly reclaims the unmounted item.

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
destruction invalidates the declaration's observer. Fixed-size spacing and
custom weights remain pending with the layout API.

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
construction and change flags immediately on a currently mounted declaration.
Initial flags are not reapplied on redraw. Keep state, not borrowed node
references, between redraws. Window-manager behavior still needs real-display
validation beyond the automated offscreen tests.

## Tabs

Include `cppui/qt/tab.hpp` or `cppui/lvgl/tab.hpp`. `Tabs()` owns a native
QTabWidget or LVGL tabview; `Tab(label)` is virtual and copies its label.
Use normal construction scopes:

```cpp
for(auto tabs: CPPUI::Tabs()) {
    for(auto first: CPPUI::Tab("Tab 1")) {
        CPPUI::Label("Content 1");
    }
    for(auto second: CPPUI::Tab("Tab 2")) {
        CPPUI::Label("Content 2");
    }
}
```

Following [PUI's implementation](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/PUI/PySide6/tab.py),
each physical child reached through a virtual Tab becomes a native page. Page
insertion finds the nearest ancestor Tab for its label. No implicit layout is
inserted and no extra children are discarded. To put multiple controls on one
page, place an explicit VBox or HBox inside the Tab. An empty Tab creates no page.
Direct declarations inside Tabs must be `Tab` nodes; additional virtual views
can appear inside each Tab.

Native widgets are reused on redraw and keyed moves. Unchanged redraws and label
updates preserve user selection; removal/reinsertion uses Qt's native selection
behavior. Removed pages and declarations are reclaimed through normal teardown.

LVGL provides one native page container per physical child, as required by its
tabview. These containers do not group sibling declarations. Keyed moves preserve
the child widget and selected page; removing the selected page clamps the active
index to the remaining pages. Empty tabviews retain their native container.
Independent subview redraws preserve the same page order and selection rules.

Two reference quirks are preserved: `Tabs(tabposition = "n")` stores the string
but leaves the native position unchanged, and `postSync()` updates page labels
by direct Tab declaration index. With two physical children in Tab("A") followed
by one in Tab("B"), page labels become A, B, B; an empty first Tab can similarly
overwrite the next page's label. Use one physical child per Tab for matching
page/title pairs. The cookbook follows that convention.

## Canvas

Include `cppui/qt/canvas.hpp` or `cppui/lvgl/canvas.hpp` for the selected backend.
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

Both backends include `drawText`, `drawLine`, `drawPolyline`, `drawCubicBezier`,
`drawPolygon`, `drawRect` and `drawEllipse`; Qt also has `drawImage`. Coordinates are logical
pixels; each operation restores painter state. Colors accept `0xRRGGBB`, color
names or strings such as `"#ff0000"`. Use `{}` for an absent color: shape fill and
stroke are disabled when absent; text keeps the current pen. Rectangles take
two corners, ellipses take center/radii, and coordinate sequences use
`std::vector<CanvasPoint>` or initializer lists.

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
Input/drag/drop events, shared layout/style modifiers and Shapely integration
remain pending. The cookbook uses a private size/background adapter meanwhile.

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
interactive selection is not implemented; Html/MarkDown are unavailable. The
full shared Text recipe remains disabled until those capabilities are decided.

## Splitter

Include `cppui/qt/splitter.hpp` and use `Splitter(vertical = false)` as a child
construction scope. It defaults to horizontal; `Splitter(true)` is vertical.
Redraw updates orientation while retaining the native splitter and its panes.
Unchanged redraw preserves user-adjusted pane sizes. Keyed children can move,
be inserted or removed, including flattened virtual subviews. Nested splitters
are supported; removed panes follow the normal deferred-deletion lifecycle.

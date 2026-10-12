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

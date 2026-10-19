# Portable examples

All examples include `cppui/backend.hpp`, declare CPPUI components and start via
`Application::run()`. C++ sources contain no native toolkit calls or backend
conditionals. CMake selects the toolkit; each backend owns native initialization,
event processing and cleanup.

- [Counter](minimal.hpp): buttons, State, repeated/conditional children, checkbox,
  layout, styling, a divider and wrapping text. Build with [Qt](minimal_qt/README.md),
  [wx](minimal_wx/README.md) or [LVGL SDL2/X11](minimal_lvgl/README.md).
- [Virtual nodes](virtual_nodes.hpp): sibling/nested views and retained external
  State. Its [CMake project](test_virtual_nodes/README.md) supports all four builds.

Interactive executables contain no test driver. With `BUILD_TESTING=ON` (the
default), separate `_smoke` executables compile the same declarations and link
native helpers from `tests/examples/`. Use CTest to run them with the appropriate
environment, or execute a `_smoke` binary directly to watch its native controls.
`BUILD_TESTING=OFF` builds only the interactive examples.

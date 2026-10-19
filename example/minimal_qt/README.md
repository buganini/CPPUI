# Qt counter example

Builds the shared [counter source](../minimal.hpp) with the Qt backend. Requires
Qt 6 development files and CMake 3.24+:

```sh
cmake -S example/minimal_qt -B example/minimal_qt/build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build example/minimal_qt/build -j "$(nproc)"
./example/minimal_qt/build/minimal
ctest --test-dir example/minimal_qt/build --output-on-failure
```

The example uses only portable CPPUI declarations for the counter, conditional
labels, checkbox, layout and styling. `Application::run()` owns native startup.
The separate `minimal_smoke` executable checks native button/checkbox events,
label updates and removal/reinsertion, with Qt's offscreen platform in CTest.
Add `-DCPPUI_VALGRIND_TESTS=ON` to a non-sanitized build for memory checks.
The root Qt build exposes `cppui_minimal_qt` and `cppui_minimal_qt_smoke`.

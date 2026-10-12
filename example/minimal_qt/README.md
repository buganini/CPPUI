# Qt counter example

Requires Qt 6 development files and CMake 3.24+. From this directory:

```sh
cmake -S . -B build
cmake --build build
./build/minimal
ctest --test-dir build --output-on-failure
```

The executable accepts `--smoke` to click the counter controls, check native
label updates and exercise an empty range without starting the interactive loop.
CTest selects Qt's offscreen platform. Add `-DCPPUI_VALGRIND_TESTS=ON` in a
non-sanitized build to check the same run with Memcheck. The root Qt build also
registers this example's smoke and optional memory tests.

`UI` inherits the Application constructors. To set an initial default window
icon, construct it with `UI app(argc, argv, "icon.png")`. The optional path is
copied; redraws preserve later native icon changes.

It also shows PUI's `.layout(...)`, `.style(...)` and typed `.qt(...)` modifiers for
padding, a preferred label width, colors/fonts and a fixed horizontal size policy.

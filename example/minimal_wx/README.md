# wxWidgets counter example

Builds the same [counter source](../minimal.hpp) as Qt and LVGL. Install
wxWidgets 3.2 development packages (`libwxgtk3.2-dev` on Linux or
`brew install wxwidgets` on macOS), then run:

```sh
cmake -S example/minimal_wx -B example/minimal_wx/build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build example/minimal_wx/build -j "$(nproc)"
./example/minimal_wx/build/minimal
ctest --test-dir example/minimal_wx/build --output-on-failure
```

Native event injection lives in the separate `minimal_smoke` test executable.
It checks counter buttons, the visibility checkbox, native labels and an empty
label range. Both executables require a display; Linux CI can run CTest under
`xvfb-run -a`. The root `CPPUI_BUILD_WX=ON` build exposes `cppui_minimal_wx` and
`cppui_minimal_wx_smoke`, including optional Valgrind checks.

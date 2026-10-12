# wxWidgets counter

Install wxWidgets 3.2 development packages (`libwxgtk3.2-dev` on Linux or
`brew install wxwidgets` on macOS), then build:

```sh
cmake -S example/minimal_wx -B build-minimal-wx
cmake --build build-minimal-wx
./build-minimal-wx/minimal
```

`--smoke` mounts, redraws and tears down the example. It still requires a display.
The root `CPPUI_BUILD_WX=ON` build registers it with CTest and optional Valgrind.

# LVGL counter example

Builds the same [counter source](../minimal.hpp) as Qt and wx, using LVGL 9.6.0
and SDL2 or X11 development packages. From the repository root:

```sh
make lvgl-checkout
cmake -S example/minimal_lvgl -B example/minimal_lvgl/build-sdl2 \
  -DCPPUI_LVGL_SOURCE_DIR="$PWD/lvgl-9.6.0" \
  -DCPPUI_LVGL_DRIVER=lvgl_sdl2 -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build example/minimal_lvgl/build-sdl2 -j "$(nproc)"
./example/minimal_lvgl/build-sdl2/minimal
ctest --test-dir example/minimal_lvgl/build-sdl2 --output-on-failure
```

For X11, use `-DCPPUI_LVGL_DRIVER=lvgl_x11` and a different build directory,
for example `example/minimal_lvgl/build-x11`. X11 runs require `DISPLAY`;
Linux CI can supply it with Xvfb. SDL2 CTest runs use dummy video/software
rendering. To watch the same test, run `minimal_smoke` without those overrides.

`cmake/CPPUILVGLDesktop.cmake` supplies the same LVGL configuration, input bridge
and driver fixes used by the cookbook. `Application::run()` creates the native
host and inputs; closing the window exits the loop and application teardown
releases owned resources. No native initialization or test helpers are linked
into the shared example source. Add `-DCPPUI_VALGRIND_TESTS=ON` for memory checks.

# Shared virtual-node example

The portable [source](../virtual_nodes.hpp) demonstrates sibling views, nested
views, independent redraws and removing/reinserting a virtual subtree. It uses
`CPPUI::Backend::View` and `Application::run()` on Qt, wx, LVGL SDL2 and LVGL X11.
Its external State model follows [PUI's virtual-node example](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/virtual_nodes.py).

From the repository root, select a backend at configure time (Qt is the default):

```sh
cmake -S example/test_virtual_nodes -B example/test_virtual_nodes/build-qt \
  -DCPPUI_EXAMPLE_BACKEND=qt -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build example/test_virtual_nodes/build-qt -j "$(nproc)"
./example/test_virtual_nodes/build-qt/test_virtual_nodes
ctest --test-dir example/test_virtual_nodes/build-qt --output-on-failure
```

Use `CPPUI_EXAMPLE_BACKEND=wx`, `lvgl_sdl2` or `lvgl_x11` in separate build
directories. LVGL also requires `-DCPPUI_LVGL_SOURCE_DIR="$PWD/lvgl-9.6.0"`;
`make lvgl-checkout` obtains the supported SDK. See the [LVGL counter build](../minimal_lvgl/README.md)
for dependency and display requirements.

Native clicks and label assertions live in the separate `test_virtual_nodes_smoke`
executable under `tests/examples/`. It covers independent State updates, repeated
Hide/Show, state changes while unmounted, and growing an empty nested view without
disturbing its sibling. CTest uses Qt offscreen and SDL dummy video; wx and X11
need a display. `-DCPPUI_VALGRIND_TESTS=ON` registers additional memory checks.

The root Qt build exposes `cppui_virtual_nodes` and `cppui_virtual_nodes_smoke`.
The wx build exposes `cppui_virtual_nodes_wx` and `cppui_virtual_nodes_wx_smoke`.

# Prerequisties
## macOS
```
brew install sdl2
```

# Build
```
git clone https://github.com/lvgl/lvgl
wget https://raw.githubusercontent.com/lvgl/lv_port_pc_eclipse/master/lv_conf.h
cmake .
make
```

For the shared SDL2/X11 cookbook and Valgrind tests against LVGL 9, see
[the cookbook build](../../cookbook/README.md). That build uses an explicit SDK
path for LVGL v9.6.0; it does not require an in-tree clone.

The example's `Spacer()` expands vertically in its `VBox`. The same public
component expands horizontally in an `HBox`; it is available from
`cppui/lvgl/layout.hpp` or `cppui/lvgl/spacer.hpp`.

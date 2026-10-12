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

For the shared C++ cookbook and headless/Valgrind tests against LVGL 9, see
[the cookbook build](../../cookbook/README.md). That build uses an explicit SDK
path and was validated with LVGL 9.3.0; it does not require an in-tree clone.

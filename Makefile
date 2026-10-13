.DEFAULT_GOAL := help

CMAKE ?= cmake
BUILD_TYPE ?= RelWithDebInfo
JOBS ?= $(shell nproc)
CPPUI_LVGL_SOURCE_DIR ?= $(CURDIR)/lvgl-9.6.0
CMAKE_ARGS ?=

.PHONY: help qt wx lvgl-checkout lvgl_sdl2 lvgl_x11

help:
	@printf '%s\n' 'Build and run cookbooks: make qt | make wx | make lvgl_sdl2 | make lvgl_x11' \
		'LVGL source checkout: make lvgl-checkout (also run by LVGL builds)' \
		'Options: JOBS, BUILD_TYPE, CPPUI_LVGL_SOURCE_DIR, CMAKE_ARGS'

qt:
	$(CMAKE) -S cookbook/qt -B build-cookbook-qt \
		-DCMAKE_BUILD_TYPE="$(BUILD_TYPE)" $(CMAKE_ARGS)
	$(CMAKE) --build build-cookbook-qt --target cppui_cookbook_qt -j "$(JOBS)"
	./build-cookbook-qt/cppui_cookbook_qt

wx:
	$(CMAKE) -S cookbook/wx -B build-cookbook-wx \
		-DCMAKE_BUILD_TYPE="$(BUILD_TYPE)" $(CMAKE_ARGS)
	$(CMAKE) --build build-cookbook-wx --target cppui_cookbook_wx -j "$(JOBS)"
	./build-cookbook-wx/cppui_cookbook_wx

lvgl-checkout:
	@if [ ! -f "$(CPPUI_LVGL_SOURCE_DIR)/lvgl.h" ]; then \
		git clone --depth 1 --branch v9.6.0 https://github.com/lvgl/lvgl.git "$(CPPUI_LVGL_SOURCE_DIR)"; \
	fi

lvgl_sdl2: lvgl-checkout
	$(CMAKE) -S cookbook/lvgl_sdl2 -B build-cookbook-lvgl_sdl2 \
		-DCMAKE_BUILD_TYPE="$(BUILD_TYPE)" \
		-DCPPUI_LVGL_SOURCE_DIR="$(CPPUI_LVGL_SOURCE_DIR)" $(CMAKE_ARGS)
	$(CMAKE) --build build-cookbook-lvgl_sdl2 --target cppui_cookbook_lvgl_sdl2 -j "$(JOBS)"
	./build-cookbook-lvgl_sdl2/cppui_cookbook_lvgl_sdl2

lvgl_x11: lvgl-checkout
	$(CMAKE) -S cookbook/lvgl_x11 -B build-cookbook-lvgl_x11 \
		-DCMAKE_BUILD_TYPE="$(BUILD_TYPE)" \
		-DCPPUI_LVGL_SOURCE_DIR="$(CPPUI_LVGL_SOURCE_DIR)" $(CMAKE_ARGS)
	$(CMAKE) --build build-cookbook-lvgl_x11 --target cppui_cookbook_lvgl_x11 -j "$(JOBS)"
	./build-cookbook-lvgl_x11/cppui_cookbook_lvgl_x11

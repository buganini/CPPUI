# Shared recipes selected by this backend's implemented capabilities.
target_sources(${target} PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/backend.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../canvas.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../checkbox.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../combobox.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../dialog.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../modal.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../mdi.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../image.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../grid.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../stack.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../progressbar.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../radiobutton.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../scroll.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../splitter.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../tab.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../text.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../textfield.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../timeline.cpp")

target_compile_definitions(${target} PRIVATE CPPUI_COOKBOOK_ASSET_DIR="${CMAKE_CURRENT_LIST_DIR}/../assets")

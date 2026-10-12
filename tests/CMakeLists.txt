find_package(Threads REQUIRED)

add_executable(cppui_core_headers core_headers.cpp core_headers_support.cpp)
target_link_libraries(cppui_core_headers PRIVATE CPPUI::cppui Threads::Threads)
set_target_properties(cppui_core_headers PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)
cppui_add_test(core.headers cppui_core_headers)

add_executable(cppui_core_lifetime core_lifetime.cpp)
target_link_libraries(cppui_core_lifetime PRIVATE CPPUI::cppui)
set_target_properties(cppui_core_lifetime PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)
cppui_add_test(core.lifetime cppui_core_lifetime)

add_executable(cppui_core_scopes core_scopes.cpp)
target_link_libraries(cppui_core_scopes PRIVATE CPPUI::cppui)
set_target_properties(cppui_core_scopes PROPERTIES
    CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
cppui_add_test(core.scopes cppui_core_scopes)

add_executable(cppui_core_state core_state.cpp)
target_link_libraries(cppui_core_state PRIVATE CPPUI::cppui)
set_target_properties(cppui_core_state PROPERTIES
    CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
cppui_add_test(core.state cppui_core_state)

add_executable(cppui_core_reconciliation core_reconciliation.cpp)
target_link_libraries(cppui_core_reconciliation PRIVATE CPPUI::cppui)
set_target_properties(cppui_core_reconciliation PROPERTIES
    CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
cppui_add_test(core.reconciliation cppui_core_reconciliation)

add_executable(cppui_core_optional core_optional.cpp)
target_link_libraries(cppui_core_optional PRIVATE CPPUI::cppui)
set_target_properties(cppui_core_optional PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
cppui_add_test(core.optional cppui_core_optional)

add_executable(cppui_core_binding core_binding.cpp)
target_link_libraries(cppui_core_binding PRIVATE CPPUI::cppui)
set_target_properties(cppui_core_binding PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
cppui_add_test(core.binding cppui_core_binding)

# Each public core header must compile as the first include, independently of
# whatever headers the application happens to include before it.
foreach(header context_header context node_header node view_header view state dom optional binding scroll modifiers)
    set(source "${CMAKE_CURRENT_BINARY_DIR}/header_${header}.cpp")
    file(WRITE "${source}" "#include \"cppui/${header}.hpp\"\n")
    add_library(cppui_header_${header} OBJECT "${source}")
    target_link_libraries(cppui_header_${header} PRIVATE CPPUI::cppui)
    set_target_properties(cppui_header_${header} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
    )
endforeach()

if(CPPUI_BUILD_QT)
    add_executable(cppui_qt_virtual_nodes qt_virtual_nodes.cpp qt_support.cpp)
    target_link_libraries(cppui_qt_virtual_nodes PRIVATE CPPUI::qt)
    cppui_add_test(qt.virtual_nodes cppui_qt_virtual_nodes)

    add_executable(cppui_qt_widgets qt_widgets.cpp)
    target_link_libraries(cppui_qt_widgets PRIVATE CPPUI::qt)
    cppui_add_test(qt.widgets cppui_qt_widgets)

    add_executable(cppui_qt_lifetime qt_lifetime.cpp)
    target_link_libraries(cppui_qt_lifetime PRIVATE CPPUI::qt)
    cppui_add_test(qt.lifetime cppui_qt_lifetime)
    cppui_add_test(qt.application_owned cppui_qt_lifetime --owned)

    add_executable(cppui_qt_application qt_application.cpp)
    target_link_libraries(cppui_qt_application PRIVATE CPPUI::qt)
    cppui_add_test(qt.application_quit cppui_qt_application)
    cppui_add_test(qt.application_quit_owned cppui_qt_application --owned)

    add_executable(cppui_qt_modifiers qt_modifiers.cpp)
    target_link_libraries(cppui_qt_modifiers PRIVATE CPPUI::qt)
    cppui_add_test(qt.modifiers cppui_qt_modifiers)

    add_executable(cppui_qt_layout qt_layout.cpp)
    target_link_libraries(cppui_qt_layout PRIVATE CPPUI::qt)
    cppui_add_test(qt.layout cppui_qt_layout)

    add_executable(cppui_qt_splitter qt_splitter.cpp)
    target_link_libraries(cppui_qt_splitter PRIVATE CPPUI::qt)
    cppui_add_test(qt.splitter cppui_qt_splitter)

    add_executable(cppui_qt_canvas qt_canvas.cpp)
    target_link_libraries(cppui_qt_canvas PRIVATE CPPUI::qt)
    cppui_add_test(qt.canvas cppui_qt_canvas)
    cppui_add_test(qt.canvas_hidpi cppui_qt_canvas)
    set_property(TEST qt.canvas_hidpi APPEND PROPERTY ENVIRONMENT "QT_SCALE_FACTOR=2")
    if(CPPUI_VALGRIND_TESTS)
        set_property(TEST valgrind.qt.canvas_hidpi APPEND PROPERTY ENVIRONMENT "QT_SCALE_FACTOR=2")
    endif()

    add_executable(cppui_qt_tab qt_tab.cpp)
    target_link_libraries(cppui_qt_tab PRIVATE CPPUI::qt)
    cppui_add_test(qt.tab cppui_qt_tab)

    add_executable(cppui_qt_dialog qt_dialog.cpp)
    target_link_libraries(cppui_qt_dialog PRIVATE CPPUI::qt)
    cppui_add_test(qt.dialog cppui_qt_dialog)

    add_executable(cppui_qt_modal qt_modal.cpp)
    target_link_libraries(cppui_qt_modal PRIVATE CPPUI::qt)
    cppui_add_test(qt.modal cppui_qt_modal)

    add_executable(cppui_qt_table qt_table.cpp)
    target_link_libraries(cppui_qt_table PRIVATE CPPUI::qt)
    cppui_add_test(qt.table cppui_qt_table)

    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_executable(cppui_qt_tree qt_tree.cpp)
    target_link_libraries(cppui_qt_tree PRIVATE CPPUI::qt Qt6::Test)
    cppui_add_test(qt.tree cppui_qt_tree)

    add_executable(cppui_qt_scroll qt_scroll.cpp)
    target_link_libraries(cppui_qt_scroll PRIVATE CPPUI::qt)
    cppui_add_test(qt.scroll cppui_qt_scroll)

    add_executable(cppui_qt_combobox qt_combobox.cpp)
    target_link_libraries(cppui_qt_combobox PRIVATE CPPUI::qt)
    cppui_add_test(qt.combobox cppui_qt_combobox)

    add_executable(cppui_qt_radiobutton qt_radiobutton.cpp)
    target_link_libraries(cppui_qt_radiobutton PRIVATE CPPUI::qt)
    cppui_add_test(qt.radiobutton cppui_qt_radiobutton)

    add_executable(cppui_qt_textfield qt_textfield.cpp)
    target_link_libraries(cppui_qt_textfield PRIVATE CPPUI::qt)
    cppui_add_test(qt.textfield cppui_qt_textfield)

    add_executable(cppui_qt_actions qt_actions.cpp)
    target_link_libraries(cppui_qt_actions PRIVATE CPPUI::qt)
    cppui_add_test(qt.actions cppui_qt_actions)

    add_executable(cppui_qt_containers qt_containers.cpp)
    target_link_libraries(cppui_qt_containers PRIVATE CPPUI::qt)
    cppui_add_test(qt.containers cppui_qt_containers)

    add_executable(cppui_qt_image qt_image.cpp)
    target_link_libraries(cppui_qt_image PRIVATE CPPUI::qt)
    cppui_add_test(qt.image cppui_qt_image)

    add_executable(cppui_qt_mdi qt_mdi.cpp)
    target_link_libraries(cppui_qt_mdi PRIVATE CPPUI::qt)
    cppui_add_test(qt.mdi cppui_qt_mdi)

    add_executable(cppui_qt_timeline qt_timeline.cpp)
    target_link_libraries(cppui_qt_timeline PRIVATE CPPUI::qt)
    cppui_add_test(qt.timeline cppui_qt_timeline)

    add_executable(cppui_qt_window qt_window.cpp)
    target_link_libraries(cppui_qt_window PRIVATE CPPUI::qt)
    cppui_add_test(qt.window cppui_qt_window)

    add_executable(cppui_qt_scheduling qt_scheduling.cpp)
    target_link_libraries(cppui_qt_scheduling PRIVATE CPPUI::qt)
    cppui_add_test(qt.scheduling cppui_qt_scheduling)

    add_executable(cppui_qt_spacer qt_spacer.cpp)
    target_link_libraries(cppui_qt_spacer PRIVATE CPPUI::qt)
    cppui_add_test(qt.spacer cppui_qt_spacer)

    add_executable(cppui_qt_label qt_label.cpp)
    target_link_libraries(cppui_qt_label PRIVATE CPPUI::qt)
    cppui_add_test(qt.label cppui_qt_label)

    add_executable(cppui_qt_checkbox qt_checkbox.cpp)
    target_link_libraries(cppui_qt_checkbox PRIVATE CPPUI::qt)
    cppui_add_test(qt.checkbox cppui_qt_checkbox)

    foreach(header options application base button canvas checkbox combobox dialog divider grid image image_resource label layout mdi menu modal progressbar radiobutton scroll spacer splitter stack tab table text textfield timeline toolbar tree window)
        set(source "${CMAKE_CURRENT_BINARY_DIR}/header_qt_${header}.cpp")
        file(WRITE "${source}"
            "#include \"cppui/qt/${header}.hpp\"\n#include \"cppui/qt/${header}.hpp\"\n")
        add_library(cppui_header_qt_${header} OBJECT "${source}")
        target_link_libraries(cppui_header_qt_${header} PRIVATE CPPUI::qt)
    endforeach()
endif()

add_executable(cppui_core_grid core_grid.cpp)
target_link_libraries(cppui_core_grid PRIVATE CPPUI::cppui)
set_target_properties(cppui_core_grid PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
cppui_add_test(core.grid cppui_core_grid)

# Link this header in isolation as well as compiling it: size bindings use the
# view dependency protocol without requiring a caller to include view.hpp.
add_executable(cppui_core_node_size core_node_size.cpp)
target_link_libraries(cppui_core_node_size PRIVATE CPPUI::cppui)
set_target_properties(cppui_core_node_size PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
cppui_add_test(core.node_size cppui_core_node_size)

add_executable(cppui_core_adaptive core_adaptive.cpp)
target_link_libraries(cppui_core_adaptive PRIVATE CPPUI::cppui)
set_target_properties(cppui_core_adaptive PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
cppui_add_test(core.adaptive cppui_core_adaptive)

find_package(Threads REQUIRED)

if(CPPUI_BUILD_WX)
    add_executable(cppui_wx_grid native_grid.cpp)
    target_compile_definitions(cppui_wx_grid PRIVATE CPPUI_TEST_WX)
    target_link_libraries(cppui_wx_grid PRIVATE CPPUI::wx Threads::Threads)
    cppui_add_test(wx.grid cppui_wx_grid)
    add_executable(cppui_wx_adaptive native_adaptive.cpp)
    target_compile_definitions(cppui_wx_adaptive PRIVATE CPPUI_TEST_WX)
    target_link_libraries(cppui_wx_adaptive PRIVATE CPPUI::wx Threads::Threads)
    cppui_add_test(wx.adaptive cppui_wx_adaptive)
    add_executable(cppui_wx_visibility native_visibility.cpp)
    target_compile_definitions(cppui_wx_visibility PRIVATE CPPUI_TEST_WX)
    target_link_libraries(cppui_wx_visibility PRIVATE CPPUI::wx Threads::Threads)
    cppui_add_test(wx.visibility cppui_wx_visibility)
    add_executable(cppui_wx_stack wx_stack.cpp)
    target_link_libraries(cppui_wx_stack PRIVATE CPPUI::wx)
    cppui_add_test(wx.stack cppui_wx_stack)
    add_executable(cppui_wx_splitter wx_splitter.cpp)
    target_link_libraries(cppui_wx_splitter PRIVATE CPPUI::wx)
    set_target_properties(cppui_wx_splitter PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
    cppui_add_test(wx.splitter cppui_wx_splitter)
    add_executable(cppui_wx_drop native_drop.cpp)
    target_compile_definitions(cppui_wx_drop PRIVATE CPPUI_TEST_WX)
    target_link_libraries(cppui_wx_drop PRIVATE CPPUI::wx)
    cppui_add_test(wx.drop cppui_wx_drop)
    add_executable(cppui_wx_refs native_refs.cpp)
    target_compile_definitions(cppui_wx_refs PRIVATE CPPUI_TEST_WX)
    target_link_libraries(cppui_wx_refs PRIVATE CPPUI::wx Threads::Threads)
    cppui_add_test(wx.refs cppui_wx_refs)
    add_executable(cppui_wx_native_baseline wx_native_baseline.cpp)
    target_link_libraries(cppui_wx_native_baseline PRIVATE CPPUI::wx)
    cppui_add_test(wx.native_baseline cppui_wx_native_baseline)
    add_executable(cppui_wx_runtime wx_runtime.cpp wx_support.cpp)
    target_link_libraries(cppui_wx_runtime PRIVATE CPPUI::wx)
    cppui_add_test(wx.runtime cppui_wx_runtime)
    cppui_add_test(wx.application_owned cppui_wx_runtime --owned)
    add_executable(cppui_wx_controls wx_controls.cpp)
    target_link_libraries(cppui_wx_controls PRIVATE CPPUI::wx)
    cppui_add_test(wx.controls cppui_wx_controls)
    add_executable(cppui_wx_combobox wx_combobox.cpp)
    target_link_libraries(cppui_wx_combobox PRIVATE CPPUI::wx)
    cppui_add_test(wx.combobox cppui_wx_combobox)
    add_executable(cppui_wx_containers wx_containers.cpp)
    target_link_libraries(cppui_wx_containers PRIVATE CPPUI::wx)
    cppui_add_test(wx.containers cppui_wx_containers)
    add_executable(cppui_wx_views wx_views.cpp)
    target_link_libraries(cppui_wx_views PRIVATE CPPUI::wx)
    cppui_add_test(wx.views cppui_wx_views)
    add_executable(cppui_wx_graphics wx_graphics.cpp)
    target_link_libraries(cppui_wx_graphics PRIVATE CPPUI::wx)
    cppui_add_test(wx.graphics cppui_wx_graphics)
    add_executable(cppui_wx_data_views wx_data_views.cpp)
    target_link_libraries(cppui_wx_data_views PRIVATE CPPUI::wx)
    cppui_add_test(wx.data_views cppui_wx_data_views)
    foreach(header adaptive drop application base button label layout window grid divider progressbar checkbox radiobutton textfield combobox completion text scroll tab views dialog timeline menu image_resource image canvas table tree stack splitter)
        set(source "${CMAKE_CURRENT_BINARY_DIR}/header_wx_${header}.cpp")
        file(WRITE "${source}" "#include \"cppui/wx/${header}.hpp\"\n#include \"cppui/wx/${header}.hpp\"\n")
        add_library(cppui_header_wx_${header} OBJECT "${source}")
        target_link_libraries(cppui_header_wx_${header} PRIVATE CPPUI::wx)
        set_target_properties(cppui_header_wx_${header} PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
    endforeach()
endif()

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

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(cppui_core_landlock core_landlock.cpp)
    target_link_libraries(cppui_core_landlock PRIVATE CPPUI::cppui Threads::Threads)
    cppui_add_test(core.landlock cppui_core_landlock)
    set_tests_properties(core.landlock PROPERTIES SKIP_RETURN_CODE 77)
endif()

add_executable(cppui_core_drop core_drop.cpp)
target_link_libraries(cppui_core_drop PRIVATE CPPUI::cppui)
cppui_add_test(core.drop cppui_core_drop)

add_executable(cppui_core_refs core_refs.cpp)
target_link_libraries(cppui_core_refs PRIVATE CPPUI::cppui Threads::Threads)
cppui_add_test(core.refs cppui_core_refs)

add_executable(cppui_core_options core_options.cpp)
target_link_libraries(cppui_core_options PRIVATE CPPUI::cppui)
cppui_add_test(core.options cppui_core_options)

add_executable(cppui_core_frame_rate core_frame_rate.cpp)
target_link_libraries(cppui_core_frame_rate PRIVATE CPPUI::cppui)
cppui_add_test(core.frame_rate cppui_core_frame_rate)

add_executable(cppui_core_optional core_optional.cpp)
target_link_libraries(cppui_core_optional PRIVATE CPPUI::cppui)
set_target_properties(cppui_core_optional PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
cppui_add_test(core.optional cppui_core_optional)

add_executable(cppui_core_modifiers core_modifiers.cpp)
target_link_libraries(cppui_core_modifiers PRIVATE CPPUI::cppui)
set_target_properties(cppui_core_modifiers PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
cppui_add_test(core.modifiers cppui_core_modifiers)

add_executable(cppui_core_binding core_binding.cpp)
target_link_libraries(cppui_core_binding PRIVATE CPPUI::cppui)
set_target_properties(cppui_core_binding PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
cppui_add_test(core.binding cppui_core_binding)

# Each public core header must compile as the first include, independently of
# whatever headers the application happens to include before it.
foreach(header grid adaptive context_header context node_header node view_header view state dom optional binding scroll modifiers table_adapter tree_adapter diagnostics application_title canvas_input frame_rate options refs drop landlock side poster)
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
    add_executable(cppui_qt_grid native_grid.cpp)
    target_compile_definitions(cppui_qt_grid PRIVATE CPPUI_TEST_QT)
    target_link_libraries(cppui_qt_grid PRIVATE CPPUI::qt Threads::Threads)
    cppui_add_test(qt.grid cppui_qt_grid)
    add_executable(cppui_qt_adaptive native_adaptive.cpp)
    target_compile_definitions(cppui_qt_adaptive PRIVATE CPPUI_TEST_QT)
    target_link_libraries(cppui_qt_adaptive PRIVATE CPPUI::qt Threads::Threads)
    cppui_add_test(qt.adaptive cppui_qt_adaptive)
    add_executable(cppui_qt_visibility native_visibility.cpp)
    target_compile_definitions(cppui_qt_visibility PRIVATE CPPUI_TEST_QT)
    target_link_libraries(cppui_qt_visibility PRIVATE CPPUI::qt Threads::Threads)
    cppui_add_test(qt.visibility cppui_qt_visibility)
    add_executable(cppui_qt_drop native_drop.cpp)
    target_compile_definitions(cppui_qt_drop PRIVATE CPPUI_TEST_QT)
    target_link_libraries(cppui_qt_drop PRIVATE CPPUI::qt)
    cppui_add_test(qt.drop cppui_qt_drop)
    add_executable(cppui_qt_refs native_refs.cpp)
    target_compile_definitions(cppui_qt_refs PRIVATE CPPUI_TEST_QT)
    target_link_libraries(cppui_qt_refs PRIVATE CPPUI::qt Threads::Threads)
    cppui_add_test(qt.refs cppui_qt_refs)
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

    add_executable(cppui_qt_views qt_views.cpp)
    target_link_libraries(cppui_qt_views PRIVATE CPPUI::qt)
    cppui_add_test(qt.views cppui_qt_views)

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
    add_executable(cppui_qt_table_adapter qt_table_adapter.cpp)
    target_link_libraries(cppui_qt_table_adapter PRIVATE CPPUI::qt Qt6::Test)
    cppui_add_test(qt.table_adapter cppui_qt_table_adapter)

    add_executable(cppui_qt_tree qt_tree.cpp)
    target_link_libraries(cppui_qt_tree PRIVATE CPPUI::qt Qt6::Test)
    cppui_add_test(qt.tree cppui_qt_tree)

    add_executable(cppui_qt_tree_adapter qt_tree_adapter.cpp)
    target_link_libraries(cppui_qt_tree_adapter PRIVATE CPPUI::qt Qt6::Test)
    cppui_add_test(qt.tree_adapter cppui_qt_tree_adapter)

    add_executable(cppui_qt_scroll qt_scroll.cpp)
    target_link_libraries(cppui_qt_scroll PRIVATE CPPUI::qt)
    cppui_add_test(qt.scroll cppui_qt_scroll)

    add_executable(cppui_qt_combobox qt_combobox.cpp)
    target_link_libraries(cppui_qt_combobox PRIVATE CPPUI::qt Qt6::Test)
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

    foreach(header adaptive drop options application base button canvas checkbox combobox dialog divider grid image image_resource label layout mdi menu modal progressbar radiobutton scroll spacer splitter stack tab table text textfield timeline toolbar tree views window)
        set(source "${CMAKE_CURRENT_BINARY_DIR}/header_qt_${header}.cpp")
        file(WRITE "${source}"
            "#include \"cppui/qt/${header}.hpp\"\n#include \"cppui/qt/${header}.hpp\"\n")
        add_library(cppui_header_qt_${header} OBJECT "${source}")
        target_link_libraries(cppui_header_qt_${header} PRIVATE CPPUI::qt)
    endforeach()
endif()

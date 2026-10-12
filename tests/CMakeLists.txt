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

# Each public core header must compile as the first include, independently of
# whatever headers the application happens to include before it.
foreach(header context_header context node_header node view_header view state dom)
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

    foreach(header application base button label layout progressbar text window)
        set(source "${CMAKE_CURRENT_BINARY_DIR}/header_qt_${header}.cpp")
        file(WRITE "${source}"
            "#include \"cppui/qt/${header}.hpp\"\n#include \"cppui/qt/${header}.hpp\"\n")
        add_library(cppui_header_qt_${header} OBJECT "${source}")
        target_link_libraries(cppui_header_qt_${header} PRIVATE CPPUI::qt)
    endforeach()
endif()

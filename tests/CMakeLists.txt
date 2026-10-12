find_package(Threads REQUIRED)

add_executable(cppui_core_headers core_headers.cpp core_headers_support.cpp)
target_link_libraries(cppui_core_headers PRIVATE CPPUI::cppui Threads::Threads)
set_target_properties(cppui_core_headers PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)
add_test(NAME core.headers COMMAND cppui_core_headers)

add_executable(cppui_core_lifetime core_lifetime.cpp)
target_link_libraries(cppui_core_lifetime PRIVATE CPPUI::cppui)
set_target_properties(cppui_core_lifetime PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)
add_test(NAME core.lifetime COMMAND cppui_core_lifetime)

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
    add_test(NAME qt.virtual_nodes COMMAND cppui_qt_virtual_nodes)
    set_tests_properties(qt.virtual_nodes PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        TIMEOUT 20
    )

    add_executable(cppui_qt_widgets qt_widgets.cpp)
    target_link_libraries(cppui_qt_widgets PRIVATE CPPUI::qt)
    add_test(NAME qt.widgets COMMAND cppui_qt_widgets)
    set_tests_properties(qt.widgets PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 20)

    foreach(header application base button label layout window)
        set(source "${CMAKE_CURRENT_BINARY_DIR}/header_qt_${header}.cpp")
        file(WRITE "${source}"
            "#include \"cppui/qt/${header}.hpp\"\n#include \"cppui/qt/${header}.hpp\"\n")
        add_library(cppui_header_qt_${header} OBJECT "${source}")
        target_link_libraries(cppui_header_qt_${header} PRIVATE CPPUI::qt)
    endforeach()
endif()

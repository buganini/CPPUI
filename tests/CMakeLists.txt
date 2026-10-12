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

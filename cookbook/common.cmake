include(CTest)
include("${CMAKE_CURRENT_LIST_DIR}/../cmake/CPPUItesting.cmake")

function(cppui_add_cookbook target backend)
    set(cookbook_dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}")
    if(NOT backend MATCHES "^(qt|wx|lvgl)$")
        message(FATAL_ERROR "Unknown cookbook backend: ${backend}")
    endif()
    string(TOUPPER "${backend}" backend_macro)
    add_library(${target}_sources OBJECT
        "${cookbook_dir}/adaptive.cpp"
        "${cookbook_dir}/adaptive_grid.cpp"
        "${cookbook_dir}/binding.cpp"
        "${cookbook_dir}/button.cpp"
        "${cookbook_dir}/canvas.cpp"
        "${cookbook_dir}/checkbox.cpp"
        "${cookbook_dir}/combobox.cpp"
        "${cookbook_dir}/cookbook.cpp"
        "${cookbook_dir}/dialog.cpp"
        "${cookbook_dir}/drop.cpp"
        "${cookbook_dir}/grid.cpp"
        "${cookbook_dir}/hbox.cpp"
        "${cookbook_dir}/heavycanvas.cpp"
        "${cookbook_dir}/image.cpp"
        "${cookbook_dir}/label.cpp"
        "${cookbook_dir}/maskedfield.cpp"
        "${cookbook_dir}/mdi.cpp"
        "${cookbook_dir}/modal.cpp"
        "${cookbook_dir}/progressbar.cpp"
        "${cookbook_dir}/radiobutton.cpp"
        "${cookbook_dir}/recipes.cpp"
        "${cookbook_dir}/refs.cpp"
        "${cookbook_dir}/scroll.cpp"
        "${cookbook_dir}/splitter.cpp"
        "${cookbook_dir}/stack.cpp"
        "${cookbook_dir}/tab.cpp"
        "${cookbook_dir}/table.cpp"
        "${cookbook_dir}/table_adapter.cpp"
        "${cookbook_dir}/text.cpp"
        "${cookbook_dir}/textfield.cpp"
        "${cookbook_dir}/textedit.cpp"
        "${cookbook_dir}/timeline.cpp"
        "${cookbook_dir}/tree.cpp"
        "${cookbook_dir}/tree_adapter.cpp"
        "${cookbook_dir}/vbox.cpp"
        "${cookbook_dir}/views.cpp"
    )
    target_include_directories(${target}_sources PUBLIC "${cookbook_dir}/.." "${cookbook_dir}")
    target_compile_features(${target}_sources PUBLIC cxx_std_11)
    target_compile_definitions(${target}_sources PUBLIC
        CPPUI_BACKEND_${backend_macro}=1
        CPPUI_COOKBOOK_SOURCE_DIR="${cookbook_dir}"
        CPPUI_COOKBOOK_ASSET_DIR="${cookbook_dir}/assets")
    set_target_properties(${target}_sources PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
    add_executable(${target} "${cookbook_dir}/main.cpp")
    target_link_libraries(${target} PRIVATE ${target}_sources)
    set_target_properties(${target} PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
    if(BUILD_TESTING)
        add_executable(${target}_smoke "${cookbook_dir}/../tests/cookbook_smoke.cpp"
            "${cookbook_dir}/../tests/cookbook/${backend}.cpp")
        target_link_libraries(${target}_smoke PRIVATE ${target}_sources)
        set_target_properties(${target}_smoke PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
        if(NOT TARGET cppui_heavycanvas_data)
            add_executable(cppui_heavycanvas_data "${cookbook_dir}/../tests/heavycanvas_data.cpp")
            target_compile_features(cppui_heavycanvas_data PRIVATE cxx_std_11)
            cppui_add_test(cookbook.heavycanvas_data cppui_heavycanvas_data)
        endif()
        set(test_backend "${backend}")
        if(ARGC GREATER 2)
            set(test_backend "${ARGV2}")
        endif()
        cppui_add_test("cookbook.${test_backend}" ${target}_smoke)
    endif()
endfunction()

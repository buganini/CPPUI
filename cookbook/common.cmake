include(CTest)
include("${CMAKE_CURRENT_LIST_DIR}/../cmake/CPPUItesting.cmake")

function(cppui_add_cookbook target backend)
    set(cookbook_dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}")
    if(NOT backend MATCHES "^(qt|wx|lvgl)$")
        message(FATAL_ERROR "Unknown cookbook backend: ${backend}")
    endif()
    string(TOUPPER "${backend}" backend_macro)
    add_executable(${target}
        "${cookbook_dir}/backend.cpp"
        "${cookbook_dir}/binding.cpp"
        "${cookbook_dir}/canvas.cpp"
        "${cookbook_dir}/checkbox.cpp"
        "${cookbook_dir}/combobox.cpp"
        "${cookbook_dir}/cookbook.cpp"
        "${cookbook_dir}/dialog.cpp"
        "${cookbook_dir}/drop.cpp"
        "${cookbook_dir}/grid.cpp"
        "${cookbook_dir}/heavycanvas.cpp"
        "${cookbook_dir}/image.cpp"
        "${cookbook_dir}/main.cpp"
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
        "${cookbook_dir}/timeline.cpp"
        "${cookbook_dir}/tree.cpp"
        "${cookbook_dir}/tree_adapter.cpp"
        "${cookbook_dir}/views.cpp"
    )
    target_include_directories(${target} PRIVATE "${cookbook_dir}/.." "${cookbook_dir}")
    target_compile_features(${target} PRIVATE cxx_std_11)
    target_compile_definitions(${target} PRIVATE
        CPPUI_COOKBOOK_${backend_macro}=1
        CPPUI_COOKBOOK_ASSET_DIR="${cookbook_dir}/assets")
    if(BUILD_TESTING)
        if(NOT TARGET cppui_heavycanvas_data)
            add_executable(cppui_heavycanvas_data "${cookbook_dir}/../tests/heavycanvas_data.cpp")
            target_compile_features(cppui_heavycanvas_data PRIVATE cxx_std_11)
            cppui_add_test(cookbook.heavycanvas_data cppui_heavycanvas_data)
        endif()
        set(test_backend "${backend}")
        if(ARGC GREATER 2)
            set(test_backend "${ARGV2}")
        endif()
        cppui_add_test("cookbook.${test_backend}" ${target} --smoke)
    endif()
endfunction()

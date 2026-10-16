include(CTest)
include("${CMAKE_CURRENT_LIST_DIR}/../cmake/CPPUItesting.cmake")

function(cppui_add_cookbook target backend)
    set(cookbook_dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}")
    add_executable(${target}
        "${cookbook_dir}/cookbook.cpp"
        "${cookbook_dir}/recipes.cpp"
        "${cookbook_dir}/${backend}/main.cpp")
    target_include_directories(${target} PRIVATE
        "${cookbook_dir}/.." "${cookbook_dir}/${backend}")
    target_compile_features(${target} PRIVATE cxx_std_11)
    include("${cookbook_dir}/${backend}/sources.cmake" OPTIONAL)
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

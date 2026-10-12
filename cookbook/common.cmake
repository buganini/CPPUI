include(CTest)
include("${CMAKE_CURRENT_LIST_DIR}/../cmake/CPPUItesting.cmake")

function(cppui_add_cookbook target backend)
    set(cookbook_dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}")
    add_executable(${target}
        "${cookbook_dir}/gallery.cpp"
        "${cookbook_dir}/recipes.cpp"
        "${cookbook_dir}/${backend}/main.cpp")
    target_include_directories(${target} PRIVATE
        "${cookbook_dir}/.." "${cookbook_dir}/${backend}")
    target_compile_features(${target} PRIVATE cxx_std_11)
    include("${cookbook_dir}/${backend}/sources.cmake" OPTIONAL)
    if(BUILD_TESTING)
        cppui_add_test("cookbook.${backend}" ${target} --smoke)
    endif()
endfunction()

include_guard(GLOBAL)

option(CPPUI_VALGRIND_TESTS "Register additional Valgrind memory tests" OFF)
set(CPPUI_VALGRIND_OPTIONS "" CACHE STRING "Additional Valgrind arguments (semicolon-separated)")
if(CPPUI_VALGRIND_TESTS)
    find_program(CPPUI_VALGRIND_EXECUTABLE NAMES valgrind REQUIRED)
endif()

function(cppui_add_test name target)
    add_test(NAME "${name}" COMMAND ${target} ${ARGN})
    set_tests_properties("${name}" PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        TIMEOUT 20)
    if(CPPUI_VALGRIND_TESTS)
        add_test(NAME "valgrind.${name}"
            COMMAND "${CPPUI_VALGRIND_EXECUTABLE}"
                --tool=memcheck
                --leak-check=full
                --show-leak-kinds=all
                --errors-for-leak-kinds=definite,indirect,possible
                --track-origins=yes
                --error-exitcode=99
                ${CPPUI_VALGRIND_OPTIONS}
                $<TARGET_FILE:${target}> ${ARGN})
        set_tests_properties("valgrind.${name}" PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            LABELS valgrind
            TIMEOUT 120)
    endif()
endfunction()

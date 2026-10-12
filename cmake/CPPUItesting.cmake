include_guard(GLOBAL)

option(CPPUI_VALGRIND_TESTS "Register additional Valgrind memory tests" OFF)
set(CPPUI_VALGRIND_OPTIONS "" CACHE STRING "Additional Valgrind arguments (semicolon-separated)")
if(CPPUI_VALGRIND_TESTS)
    find_program(CPPUI_VALGRIND_EXECUTABLE NAMES valgrind REQUIRED)
endif()

function(cppui_add_test name target)
    add_test(NAME "${name}" COMMAND ${target} ${ARGN})
    set(native_memory_options)
    if(name MATCHES "(^|\\.)wx($|[._])" AND UNIX AND NOT APPLE)
        set(native_memory_options "--num-callers=24" "--suppressions=${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../tests/wx/valgrind.supp")
        set_property(TEST "${name}" APPEND PROPERTY ENVIRONMENT_MODIFICATION
            "LSAN_OPTIONS=string_append::suppressions=${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../tests/wx/lsan.supp")
    endif()
    set_tests_properties("${name}" PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        TIMEOUT 20)
    if(CPPUI_VALGRIND_TESTS)
        set(memory_timeout 120)
        if(name STREQUAL "cookbook.wx")
            # Three complete native gallery cycles are slow under Memcheck.
            set(memory_timeout 300)
        endif()
        add_test(NAME "valgrind.${name}"
            COMMAND "${CPPUI_VALGRIND_EXECUTABLE}"
                --tool=memcheck
                --leak-check=full
                --show-leak-kinds=all
                --errors-for-leak-kinds=definite,indirect,possible
                --track-origins=yes
                --error-exitcode=99
                ${CPPUI_VALGRIND_OPTIONS}
                ${native_memory_options}
                $<TARGET_FILE:${target}> ${ARGN})
        set_tests_properties("valgrind.${name}" PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            # Qt's regexp JIT emits unsymbolized Memcheck conditional-jump
            # reports; use its interpreted path in memory checks (Valgrind FAQ 5.4).
            ENVIRONMENT_MODIFICATION "QT_ENABLE_REGEXP_JIT=set:0"
            LABELS valgrind
            TIMEOUT ${memory_timeout})
    endif()
endfunction()

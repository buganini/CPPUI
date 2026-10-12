# LVGL v9.6.0 joins its X11 tick thread only on a window-manager close, not
# when lv_display_delete/lv_deinit deletes it. Build a corrected copy so the
# shared startup can release the display without a thread accessing freed data.
# The supplied SDK checkout is never modified.
set(x11_source "${CPPUI_LVGL_SOURCE_DIR}/src/drivers/x11/lv_x11_display.c")
file(READ "${x11_source}" x11_code)
set(delete_site "    x11_disp_data_t * xd = lv_display_get_driver_data(disp);\n\n    lv_timer_delete(xd->timer);")
set(close_site "                    xd->terminated = true;\n                    void * ret = NULL;\n                    pthread_join(xd->thr_tick, &ret);\n")
foreach(fragment IN ITEMS "${delete_site}" "${close_site}" "bool            terminated;")
    string(FIND "${x11_code}" "${fragment}" position)
    if(position EQUAL -1)
        message(FATAL_ERROR "X11 lifetime fix expects LVGL v9.6.0; review the driver before updating the SDK")
    endif()
endforeach()
string(REPLACE "${delete_site}"
    "    x11_disp_data_t * xd = lv_display_get_driver_data(disp);\n\n    xd->terminated = true;\n    pthread_join(xd->thr_tick, NULL);\n    lv_timer_delete(xd->timer);"
    x11_code "${x11_code}")
string(REPLACE "${close_site}" "" x11_code "${x11_code}")
# The stop flag is shared by the UI and tick threads.
string(REPLACE "bool            terminated;" "_Atomic bool    terminated;" x11_code "${x11_code}")
set(patched_x11 "${CMAKE_CURRENT_BINARY_DIR}/lv_x11_display.c")
file(WRITE "${patched_x11}" "#include <stdatomic.h>\n${x11_code}")
get_target_property(lvgl_sources lvgl SOURCES)
list(REMOVE_ITEM lvgl_sources "${x11_source}")
list(APPEND lvgl_sources "${patched_x11}")
set_property(TARGET lvgl PROPERTY SOURCES "${lvgl_sources}")
target_compile_features(lvgl PRIVATE c_std_11)
target_include_directories(lvgl PRIVATE "${CPPUI_LVGL_SOURCE_DIR}/src/drivers/x11")

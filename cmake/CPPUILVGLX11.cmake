# LVGL v9.6.0 joins its X11 tick thread only on a window-manager close, not
# when lv_display_delete/lv_deinit deletes it. Build a corrected copy so the
# shared startup can release the display without a thread accessing freed data.
# Use a native arrow cursor so mouse movement does not wait for LVGL rendering.
# The supplied SDK checkout is never modified.
set(x11_source "${CPPUI_LVGL_SOURCE_DIR}/src/drivers/x11/lv_x11_display.c")
file(READ "${x11_source}" x11_code)
set(delete_site "    x11_disp_data_t * xd = lv_display_get_driver_data(disp);\n\n    lv_timer_delete(xd->timer);")
set(close_site "                    xd->terminated = true;\n                    void * ret = NULL;\n                    pthread_join(xd->thr_tick, &ret);\n")
set(cursor_site [=[    XColor black = { .red = 0, .green = 0, .blue = 0 };
    char empty_data[] = { 0 };

    Pixmap empty_bitmap = XCreateBitmapFromData(xd->hdr.display, xd->window, empty_data, 1, 1);
    Cursor inv_cursor = XCreatePixmapCursor(xd->hdr.display, empty_bitmap, empty_bitmap, &black, &black, 0, 0);
    XDefineCursor(xd->hdr.display, xd->window, inv_cursor);
    XFreeCursor(xd->hdr.display, inv_cursor);
    XFreePixmap(xd->hdr.display, empty_bitmap);]=])
foreach(fragment IN ITEMS "${delete_site}" "${close_site}" "${cursor_site}" "x11_hide_cursor" "bool            terminated;")
    string(FIND "${x11_code}" "${fragment}" position)
    if(position EQUAL -1)
        message(FATAL_ERROR "X11 driver fixes expect LVGL v9.6.0; review the driver before updating the SDK")
    endif()
endforeach()
string(REPLACE "${delete_site}"
    "    x11_disp_data_t * xd = lv_display_get_driver_data(disp);\n\n    xd->terminated = true;\n    pthread_join(xd->thr_tick, NULL);\n    lv_timer_delete(xd->timer);"
    x11_code "${x11_code}")
string(REPLACE "${close_site}" "" x11_code "${x11_code}")
# The stop flag is shared by the UI and tick threads.
string(REPLACE "bool            terminated;" "_Atomic bool    terminated;" x11_code "${x11_code}")
string(REPLACE "${cursor_site}"
    "    Cursor cursor = XCreateFontCursor(xd->hdr.display, XC_left_ptr);\n    XDefineCursor(xd->hdr.display, xd->window, cursor);\n    XFreeCursor(xd->hdr.display, cursor);"
    x11_code "${x11_code}")
string(REPLACE "x11_hide_cursor" "x11_set_cursor" x11_code "${x11_code}")
# Expose the native window without duplicating LVGL's private struct layout.
string(APPEND x11_code "\n#if LV_USE_X11\nunsigned long cppui_lvgl_x11_window_id(lv_display_t * disp) {\n    x11_disp_data_t * xd = lv_display_get_driver_data(disp);\n    return xd->window;\n}\n#endif\n")
set(patched_x11 "${CMAKE_CURRENT_BINARY_DIR}/lv_x11_display.c")
file(WRITE "${patched_x11}" "#include <stdatomic.h>\n#include <X11/cursorfont.h>\n${x11_code}")
get_target_property(lvgl_sources lvgl SOURCES)
list(REMOVE_ITEM lvgl_sources "${x11_source}")
list(APPEND lvgl_sources "${patched_x11}")
set_property(TARGET lvgl PROPERTY SOURCES "${lvgl_sources}")
target_compile_features(lvgl PRIVATE c_std_11)
target_include_directories(lvgl PRIVATE "${CPPUI_LVGL_SOURCE_DIR}/src/drivers/x11")

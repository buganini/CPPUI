#ifndef CPPUI_BACKEND
#define CPPUI_BACKEND

#if (defined(CPPUI_BACKEND_QT) + defined(CPPUI_BACKEND_WX) + defined(CPPUI_BACKEND_LVGL)) != 1
#error "Select exactly one CPPUI backend: CPPUI_BACKEND_QT, CPPUI_BACKEND_WX or CPPUI_BACKEND_LVGL"
#endif
#if defined(CPPUI_BACKEND_QT)
#include "qt/backend.hpp"
#elif defined(CPPUI_BACKEND_WX)
#include "wx/backend.hpp"
#else
#include "lvgl/backend.hpp"
#endif
#endif

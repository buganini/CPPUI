#ifndef CPPUI_QT_SPACER
#define CPPUI_QT_SPACER

#include "base.hpp"

namespace CPPUI {
    inline Qt::Spacer & Spacer() { return *new Qt::Spacer(); }
}
#endif

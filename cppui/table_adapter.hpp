#ifndef CPPUI_TABLE_ADAPTER
#define CPPUI_TABLE_ADAPTER

#include "optional.hpp"

namespace CPPUI {
    // Typed C++ counterpart of PUI's BaseTableAdapter. The adapter owns or
    // explicitly borrows its backing storage; Table retains the adapter itself.
    template<class T = std::string> class BaseTableAdapter {
        public:
        using Value = T;
        virtual ~BaseTableAdapter() = default;
        virtual T data(int row, int column) const = 0;
        virtual T editData(int row, int column) const { return data(row, column); }
        virtual void setData(int, int, const T &) {}
        virtual bool editable(int, int) const { return false; }
        virtual OptionalText columnHeader(int column) const { return std::string("Col ") + std::to_string(column); }
        virtual OptionalText rowHeader(int row) const { return std::string("Row ") + std::to_string(row); }
        virtual int rowCount() const = 0;
        virtual int columnCount() const = 0;
        virtual void clicked(int, int) {}
        virtual void dblclicked(int, int) {}
    };
}
#endif

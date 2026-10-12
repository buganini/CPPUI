#ifndef CPPUI_QT_OPTIONS
#define CPPUI_QT_OPTIONS

#include "../modifiers.hpp"
#include <QLayout>
#include <QStyle>
#include <QWidget>
#include <map>

namespace CPPUI {
    using QtStyleSheet = std::map<std::string, std::string>;
    // Typed counterparts of PUI's qt(**kwargs); names follow its keyword keys.
    struct QtOptions {
        Optional<QSizePolicy::Policy> HorizontalPolicy, VerticalPolicy;
        Optional<QLayout::SizeConstraint> SizeConstraint;
        // Widgets borrow Style; QApplication adopts it, following native Qt.
        Optional<QStyle *> Style;
        Optional<QtStyleSheet> StyleSheet;
        void merge(const QtOptions & other) {
            if(other.HorizontalPolicy) HorizontalPolicy = other.HorizontalPolicy;
            if(other.VerticalPolicy) VerticalPolicy = other.VerticalPolicy;
            if(other.SizeConstraint) SizeConstraint = other.SizeConstraint;
            if(other.Style) Style = other.Style;
            if(other.StyleSheet) StyleSheet = other.StyleSheet;
        }
    };
    namespace Qt {
        struct SizeHints {
            Optional<int> widthHint, heightHint;
            virtual ~SizeHints() = default;
        };
        template<class Native> class Widget: public Native, public SizeHints {
            public:
            using Native::Native;
            QSize sizeHint() const override {
                auto size = Native::sizeHint();
                if(widthHint) size.setWidth(*widthHint);
                if(heightHint) size.setHeight(*heightHint);
                return size;
            }
        };
    }
}
#endif

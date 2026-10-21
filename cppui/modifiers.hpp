#ifndef CPPUI_MODIFIERS
#define CPPUI_MODIFIERS

#include "optional.hpp"
#include <type_traits>

namespace CPPUI {
    // PUI's padding/margin order: top, right, bottom, left.
    struct Insets {
        int top, right, bottom, left;
        Insets(int all): Insets(all, all, all, all) {}
        Insets(int vertical, int horizontal): Insets(vertical, horizontal, vertical, horizontal) {}
        Insets(int top, int horizontal, int bottom): Insets(top, horizontal, bottom, horizontal) {}
        Insets(int top, int right, int bottom, int left): top(top), right(right), bottom(bottom), left(left) {}
    };

    namespace detail {
        enum class LayoutKey { Width, Height, Weight, Padding, Margin };
        enum class StyleKey { Color, BgColor, FontSize, FontWeight, FontFamily };
        enum class GridKey { Row, Column, RowSpan, ColumnSpan };

        // The key is part of the type, so applying an argument needs no lookup.
        template<class Key, Key key, class T>
        struct ModifierArgument { T value; };

        template<class Key, class Argument>
        struct IsModifierArgument: std::false_type {};
        template<class Key, Key key, class T>
        struct IsModifierArgument<Key, ModifierArgument<Key, key, T>>: std::true_type {};

        template<class Key, class... Arguments>
        struct AreModifierArguments;
        template<class Key>
        struct AreModifierArguments<Key>: std::true_type {};
        template<class Key, class First, class... Rest>
        struct AreModifierArguments<Key, First, Rest...>: std::integral_constant<bool,
            IsModifierArgument<Key, typename std::decay<First>::type>::value &&
            AreModifierArguments<Key, Rest...>::value> {};
    }

    namespace layout {
        using Width = detail::ModifierArgument<detail::LayoutKey, detail::LayoutKey::Width, int>;
        using Height = detail::ModifierArgument<detail::LayoutKey, detail::LayoutKey::Height, int>;
        using Weight = detail::ModifierArgument<detail::LayoutKey, detail::LayoutKey::Weight, int>;
        using Padding = detail::ModifierArgument<detail::LayoutKey, detail::LayoutKey::Padding, Insets>;
        using Margin = detail::ModifierArgument<detail::LayoutKey, detail::LayoutKey::Margin, Insets>;

        inline Width width(int value) { return {value}; }
        inline Height height(int value) { return {value}; }
        inline Weight weight(int value) { return {value}; }
        inline Padding padding(Insets value) { return {value}; }
        inline Padding padding(int vertical, int horizontal) { return {Insets(vertical, horizontal)}; }
        inline Padding padding(int top, int horizontal, int bottom) { return {Insets(top, horizontal, bottom)}; }
        inline Padding padding(int top, int right, int bottom, int left) { return {Insets(top, right, bottom, left)}; }
        inline Margin margin(Insets value) { return {value}; }
        inline Margin margin(int vertical, int horizontal) { return {Insets(vertical, horizontal)}; }
        inline Margin margin(int top, int horizontal, int bottom) { return {Insets(top, horizontal, bottom)}; }
        inline Margin margin(int top, int right, int bottom, int left) { return {Insets(top, right, bottom, left)}; }
    }

    namespace style {
        using Color = detail::ModifierArgument<detail::StyleKey, detail::StyleKey::Color, int>;
        using BgColor = detail::ModifierArgument<detail::StyleKey, detail::StyleKey::BgColor, int>;
        using FontSize = detail::ModifierArgument<detail::StyleKey, detail::StyleKey::FontSize, int>;
        using FontWeight = detail::ModifierArgument<detail::StyleKey, detail::StyleKey::FontWeight, int>;
        using FontFamily = detail::ModifierArgument<detail::StyleKey, detail::StyleKey::FontFamily, const char *>;

        inline Color color(int value) { return {value}; }
        inline BgColor bgColor(int value) { return {value}; }
        inline FontSize fontSize(int value) { return {value}; }
        inline FontWeight fontWeight(int value) { return {value}; }
        // Borrow a null-terminated string for the declaration's lifetime.
        inline FontFamily fontFamily(const char * value) { return {value}; }
    }

    namespace grid {
        using Row = detail::ModifierArgument<detail::GridKey, detail::GridKey::Row, int>;
        using Column = detail::ModifierArgument<detail::GridKey, detail::GridKey::Column, int>;
        using RowSpan = detail::ModifierArgument<detail::GridKey, detail::GridKey::RowSpan, int>;
        using ColumnSpan = detail::ModifierArgument<detail::GridKey, detail::GridKey::ColumnSpan, int>;
        inline Row row(int value) { return {value}; }
        inline Column column(int value) { return {value}; }
        inline RowSpan rowspan(int value) { return {value}; }
        inline ColumnSpan columnspan(int value) { return {value}; }
        struct Dense { Optional<bool> value; };
        inline Dense dense(Optional<bool> value) { return {value}; }
    }

    struct GridCell {
        int row, column, rowspan, columnspan;
        GridCell(int row=0,int column=0,int rowspan=1,int columnspan=1)
            :row(row),column(column),rowspan(rowspan),columnspan(columnspan) {}
        bool operator==(const GridCell & other) const {
            return row==other.row && column==other.column && rowspan==other.rowspan && columnspan==other.columnspan;
        }
    };
    struct QtOptions;
}
#endif

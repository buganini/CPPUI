#include "cppui/node.hpp"
#include <cstring>
#include <iostream>
#include <type_traits>
#include <utility>

namespace {
    struct Node: CPPUI::Node { void destroy(bool) override {} };
    void require(bool value, const char * message) {
        if(!value) throw std::runtime_error(message);
    }
    bool equal(const CPPUI::Insets & a, const CPPUI::Insets & b) {
        return a.top == b.top && a.right == b.right && a.bottom == b.bottom && a.left == b.left;
    }

    template<class... Arguments>
    struct AcceptsLayout {
        template<class T>
        static auto check(int) -> decltype(std::declval<T &>().layout(std::declval<Arguments>()...), std::true_type());
        template<class> static std::false_type check(...);
        static const bool value = decltype(check<CPPUI::Node>(0))::value;
    };
    template<class... Arguments>
    struct AcceptsStyle {
        template<class T>
        static auto check(int) -> decltype(std::declval<T &>().style(std::declval<Arguments>()...), std::true_type());
        template<class> static std::false_type check(...);
        static const bool value = decltype(check<CPPUI::Node>(0))::value;
    };
    template<class Argument>
    struct AcceptsWideLayout {
        template<class T>
        static auto check(int) -> decltype(std::declval<T &>().wideLayout(600,std::declval<Argument>()),std::true_type());
        template<class> static std::false_type check(...);
        static const bool value=decltype(check<CPPUI::Node>(0))::value;
    };
    template<class Argument>
    struct AcceptsWideStyle {
        template<class T>
        static auto check(int) -> decltype(std::declval<T &>().wideStyle(600,std::declval<Argument>()),std::true_type());
        template<class> static std::false_type check(...);
        static const bool value=decltype(check<CPPUI::Node>(0))::value;
    };

    static_assert(AcceptsWideLayout<CPPUI::layout::Width>::value && AcceptsWideStyle<CPPUI::style::Color>::value,
                  "wide modifiers rejected named arguments");
    static_assert(!AcceptsWideLayout<CPPUI::style::Color>::value && !AcceptsWideStyle<CPPUI::layout::Width>::value,
                  "wide modifiers accepted the wrong argument domain");
    static_assert(AcceptsLayout<CPPUI::layout::Padding, const CPPUI::layout::Width &>::value, "named layout arguments rejected");
    static_assert(AcceptsStyle<const CPPUI::style::FontFamily &, CPPUI::style::Color>::value, "named style arguments rejected");
    static_assert(AcceptsLayout<int, CPPUI::Optional<int>>::value, "positional layout arguments rejected");
    static_assert(AcceptsStyle<int, CPPUI::Optional<int>>::value, "positional style arguments rejected");
    static_assert(!AcceptsLayout<CPPUI::style::Color>::value, "style accepted by layout");
    static_assert(!AcceptsLayout<CPPUI::layout::Width, CPPUI::style::Color>::value, "mixed domains accepted");
    static_assert(!AcceptsStyle<CPPUI::layout::Width>::value, "layout accepted by style");
    static_assert(!AcceptsStyle<CPPUI::style::Color, CPPUI::layout::Width>::value, "mixed domains accepted");
    static_assert(!AcceptsLayout<CPPUI::layout::Width, int>::value, "mixed named and positional arguments accepted");
    static_assert(!AcceptsStyle<int, CPPUI::style::Color>::value, "mixed positional and named arguments accepted");
    static_assert(!AcceptsLayout<std::string>::value && !AcceptsStyle<std::string>::value, "untyped strings accepted");
    static_assert(std::is_trivially_copyable<CPPUI::layout::Width>::value, "numeric arguments should be plain values");
    static_assert(std::is_trivially_copyable<CPPUI::layout::Padding>::value, "inset arguments should be plain values");
    static_assert(std::is_trivially_copyable<CPPUI::style::FontFamily>::value, "font family arguments should only borrow text");
    static_assert(!AcceptsStyle<int, int, int, int, std::string>::value, "owning strings should require an explicit borrowed pointer");

    template<class Action>
    void rejects(Action action) {
        bool rejected = false;
        try { action(); } catch(const std::invalid_argument &) { rejected = true; }
        require(rejected, "negative dimension/weight/font size accepted");
    }
}

int main() {
    namespace layout = CPPUI::layout;
    namespace style = CPPUI::style;
    try {
        Node named, positional;
        auto & result = named
            .layout(layout::margin(1, 2, 3, 4), layout::weight(2), layout::height(40),
                    layout::padding(8, 12), layout::width(200))
            .style(style::fontFamily("Sans"), style::fontWeight(700), style::bgColor(0x222222),
                   style::fontSize(16), style::color(0xffffff));
        positional.layout(200, 40, 2, CPPUI::Insets(8, 12), CPPUI::Insets(1, 2, 3, 4))
                  .style(0xffffff, 0x222222, 16, 700, "Sans");
        require(&result == &named, "named modifiers did not return the borrowed node");
        require(*named.layout_width == *positional.layout_width && *named.layout_height == *positional.layout_height &&
                *named.layout_weight == *positional.layout_weight &&
                equal(*named.layout_padding, *positional.layout_padding) &&
                equal(*named.layout_margin, *positional.layout_margin), "named layout differs from positional layout");
        require(*named.style_color == *positional.style_color && *named.style_bgcolor == *positional.style_bgcolor &&
                *named.style_fontsize == *positional.style_fontsize && *named.style_fontweight == *positional.style_fontweight &&
                std::strcmp(named.style_fontfamily, positional.style_fontfamily) == 0, "named style differs from positional style");

        named.layout(layout::width(0)).layout({}, {}, 0).style(style::color(0)).style({}, {}, 20);
        named.layout().style().layout({}).style({});
        require(*named.layout_width == 0 && *named.layout_weight == 0 && *named.layout_height == 40 &&
                *named.style_color == 0 && *named.style_fontsize == 20 && *named.style_bgcolor == 0x222222,
                "zero, omitted arguments or interleaved positional/named calls lost merge semantics");
        named.layout(layout::width(10), layout::width(30)).style(style::fontSize(12), style::fontSize(18));
        require(*named.layout_width == 30 && *named.style_fontsize == 18, "duplicate arguments did not apply left-to-right");

        const auto width = layout::width(250);
        std::string familyOwner(80, 'A');
        auto family = style::fontFamily(familyOwner.c_str());
        named.layout(width).style(family);
        require(*named.layout_width == 250 && named.style_fontfamily == familyOwner.c_str(),
                "font family was copied instead of borrowed");
        familyOwner[0] = 'B';
        require(named.style_fontfamily[0] == 'B', "font family no longer refers to the caller's storage");
        named.style(style::fontFamily(nullptr)).style({}, {}, {}, {}, nullptr);
        require(named.style_fontfamily == familyOwner.c_str(), "null font family cleared a previous override");
        named.style(style::fontFamily(""));
        require(named.style_fontfamily && !*named.style_fontfamily, "empty font family was treated as absent");
        positional.style({}, {}, {}, {}, familyOwner.c_str());
        require(positional.style_fontfamily == familyOwner.c_str(), "positional font family was copied");
        positional.style({}, {}, {}, {}, "");

        require(equal(layout::padding(4).value, CPPUI::Insets(4)) &&
                equal(layout::padding(1, 2, 3).value, CPPUI::Insets(1, 2, 3)) &&
                equal(layout::padding(1, 2, 3, 4).value, CPPUI::Insets(1, 2, 3, 4)) &&
                equal(layout::padding(CPPUI::Insets(5)).value, CPPUI::Insets(5)) &&
                equal(layout::margin(4).value, CPPUI::Insets(4)) &&
                equal(layout::margin(1, 2).value, CPPUI::Insets(1, 2)) &&
                equal(layout::margin(1, 2, 3).value, CPPUI::Insets(1, 2, 3)),
                "named inset shorthand changed the side order");

        rejects([&]() { named.layout(layout::width(-1)); });
        rejects([&]() { named.layout(layout::width(99), layout::height(-1)); });
        rejects([&]() { named.layout(layout::height(99), layout::weight(-1)); });
        rejects([&]() { named.style(style::color(0xffffff), style::fontSize(-1)); });
        require(*named.layout_width == 250 && *named.layout_height == 40 && *named.style_color == 0,
                "invalid named arguments partially changed the node");

        Node fresh;
        fresh.layout(layout::weight(1)).style(style::fontSize(16));
        require(!fresh.layout_width && !fresh.layout_height && !fresh.layout_padding && !fresh.layout_margin &&
                !fresh.style_color && !fresh.style_bgcolor && !fresh.style_fontweight && !fresh.style_fontfamily,
                "unspecified named arguments created overrides");
    } catch(const std::exception & error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

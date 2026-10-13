#include "backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    void text(Model &) {
        for(auto column: CPPUI::VBox().layout({}, {}, 1)) {
            CPPUI::Text("Multiple\nLines\nText");
            CPPUI::Html("<i>HTML</i> <span style=\"color:red\">support</span>");
            CPPUI::MarkDown("# H1\n## H2\n* item 1\n* item 2");
            CPPUI::Spacer();
        }
    }
}

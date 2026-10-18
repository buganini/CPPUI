#include "backend.hpp"
#include "recipes.hpp"

using namespace CPPUI;

namespace Cookbook {
    namespace {
        void button(Model & model) {
            for(auto column: CPPUI::VBox().layout({}, {}, 1)) {
                for(auto row: CPPUI::HBox()) {
                    CPPUI::Label(static_cast<std::string>(model.buttonValue));
                    Button("A").click([&model]() { model.buttonValue = std::string("A"); });
                    Button("B").click([&model]() { model.buttonValue = std::string("B"); });
                    Button("Disabled").disabled().click([&model]() { model.buttonValue = std::string("Disabled"); });
                    CPPUI::Button("Styled").style(0xff0000, {}, 20);
                    CPPUI::Spacer();
                }
                CPPUI::Spacer();
            }
        }
        void label(Model &) {
            for(auto column: CPPUI::VBox().layout({}, {}, 1)) {
                CPPUI::Label("Hello World");
                CPPUI::Label("Big Yellow").style(0xffff00, 0xff0000, 20);
                CPPUI::Label("No <b>HTML</b>");
                CPPUI::Spacer();
            }
        }
        void hbox(Model &) {
            for(auto column: CPPUI::VBox()) {
                for(auto row: CPPUI::HBox().id("spacer-row")) {
                    CPPUI::Label("Column 1");
                    CPPUI::Spacer().id("row-gap");
                    CPPUI::Label("Column 2").id("row-end");
                }
                for(auto row: CPPUI::HBox()) {
                    CPPUI::Label("Column 1");
                    CPPUI::Label("Column 2").layout({}, {}, 1);
                    CPPUI::Label("Column 3");
                }
            }
            CPPUI::Spacer();
        }
        void vbox(Model &) {
            for(auto column: CPPUI::VBox().layout({}, {}, 1).id("spacer-column")) {
                CPPUI::Label("Row 1");
                CPPUI::Divider();
                CPPUI::Label("Row 2");
                CPPUI::Spacer().id("column-gap");
            }
        }
    }

    const std::vector<Recipe> & recipes() {
        static const std::vector<Recipe> result = []() {
            std::vector<Recipe> pages = {
            {"vbox", "Vbox", vbox, ""},
            {"hbox", "HBox", hbox, ""},
            {"stack", "Stack", nullptr, "Stack and layout options"},
            {"grid", "Grid", nullptr, "Grid placement API"},
            {"label", "Label", label, ""},
            {"text", "Text", nullptr, "Text, Html and MarkDown"},
            {"button", "Button", button, ""},
            {"checkbox", "Checkbox", nullptr, "Boolean/collection binding"},
            {"radiobutton", "RadioButton", nullptr, "Value binding"},
            {"combobox", "Combobox", nullptr, "Selection binding"},
            {"progressbar", "ProgressBar", nullptr, "ProgressBar"},
            {"timeline", "TimelineView", nullptr, "Cancellable timers"},
            {"maskedfield", "MaskedField", nullptr, "Password editor"},
            {"textfield", "TextField", nullptr, "Binding and text conversion"},
            {"scroll", "Scroll", nullptr, "Scroll and offset API"},
            {"heavycanvas", "HeavyCanvas", nullptr, "HeavyCanvas recipe is not yet enabled for this backend"},
            {"canvas", "Canvas", nullptr, "Painter and input APIs"},
            {"image", "Image", nullptr, "Image and size options"},
            {"tab", "Tab", nullptr, "Tabs and Tab"},
            {"views", "Views", nullptr, "Retained pages and optional selection"},
            {"refs", "Refs", refs, ""},
            {"table", "Table", nullptr, "Table data/identity API"},
            {"table_adapter", "Table(Adapter)", nullptr, "Table adapter API"},
            {"tree", "Tree", nullptr, "Tree data/identity API"},
            {"tree_adapter", "Tree(Adapter)", nullptr, "Tree adapter API"},
            {"binding", "State Binding", nullptr, "External getter/setter and subscriptions"},
            {"widget", "WidgetWrapper", nullptr, "Borrowed native widget ownership"},
            {"drop", "Drop", nullptr, "Drag/drop events"},
            {"mdi", "MDI", nullptr, "MdiArea and MdiSubWindow"},
            {"splitter", "Splitter", nullptr, "Splitter"},
            {"modal", "Modal", nullptr, "Bound visibility"},
            {"dialog", "Dialog", nullptr, "Typed results and cancellation"},
            };
            Backend::configureRecipes(pages);
            return pages;
        }();
        return result;
    }
}

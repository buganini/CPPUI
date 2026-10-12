#include "backend.hpp"
#include "recipes.hpp"

using namespace CPPUI;

namespace Cookbook {
    namespace {
        void button(Model & model) {
            for(auto row: HBox()) {
                Label(static_cast<std::string>(model.buttonValue));
                Button("A").click([&model]() { model.buttonValue = std::string("A"); });
                Button("B").click([&model]() { model.buttonValue = std::string("B"); });
            }
        }
        void label(Model &) {
            Label("Hello World");
            Label("No <b>HTML</b>");
        }
        void hbox(Model &) {
            for(auto row: HBox()) {
                Label("Column 1");
                Label("Column 2");
            }
            for(auto row: HBox()) {
                Label("Column 1");
                Label("Column 2");
                Label("Column 3");
            }
        }
        void vbox(Model &) {
            for(auto column: VBox()) {
                Label("Row 1");
                Label("Row 2");
            }
        }
    }

    const std::vector<Recipe> & recipes() {
        static const std::vector<Recipe> result = []() {
            std::vector<Recipe> pages = {
            {"vbox", "VBox", vbox, "Spacer and Divider"},
            {"hbox", "HBox", hbox, "Spacer and layout weights"},
            {"stack", "Stack", nullptr, "Stack and layout options"},
            {"grid", "Grid", nullptr, "Grid placement API"},
            {"label", "Label", label, "Text styles and Spacer"},
            {"text", "Text", nullptr, "Text, Html and MarkDown"},
            {"button", "Button", button, "Styled button and Spacer"},
            {"checkbox", "Checkbox", nullptr, "Boolean/collection binding"},
            {"radiobutton", "RadioButton", nullptr, "Value binding"},
            {"combobox", "ComboBox", nullptr, "Selection binding"},
            {"progressbar", "ProgressBar", nullptr, "ProgressBar"},
            {"timeline", "TimelineView", nullptr, "Cancellable timers"},
            {"textfield", "TextField", nullptr, "Binding and text conversion"},
            {"scroll", "Scroll", nullptr, "Scroll and offset API"},
            {"canvas", "Canvas", nullptr, "Painter and input APIs"},
            {"drawshapely", "DrawShapely", nullptr, "Deferred geometry integration"},
            {"matplotlib", "Matplotlib", nullptr, "Deferred Python integration"},
            {"image", "Image", nullptr, "Image and size options"},
            {"tab", "Tab", nullptr, "Tabs and Tab"},
            {"table", "Table", nullptr, "Table data/identity API"},
            {"table_adapter", "Table (adapter)", nullptr, "Table adapter API"},
            {"tree", "Tree", nullptr, "Tree data/identity API"},
            {"tree_adapter", "Tree (adapter)", nullptr, "Tree adapter API"},
            {"binding", "State Binding", nullptr, "External getter/setter and subscriptions"},
            {"widget", "Widget Wrapper", nullptr, "Borrowed native widget ownership"},
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

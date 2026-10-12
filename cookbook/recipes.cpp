#include "backend.hpp"
#include "recipes.hpp"

using namespace CPPUI;

namespace Cookbook {
    namespace {
        void button(Model & model) {
            for(auto column: Presentation::VBox(1)) {
                for(auto row: Presentation::HBox()) {
                    Presentation::Label(static_cast<std::string>(model.buttonValue));
                    Button("A").click([&model]() { model.buttonValue = std::string("A"); });
                    Button("B").click([&model]() { model.buttonValue = std::string("B"); });
                    Presentation::StyledButton();
                    Presentation::Spacer();
                }
                Presentation::Spacer();
            }
        }
        void label(Model &) {
            for(auto column: Presentation::VBox(1)) {
                Presentation::Label("Hello World");
                Presentation::StyledLabel();
                Presentation::Label("No <b>HTML</b>");
                Presentation::Spacer();
            }
        }
        void hbox(Model &) {
            for(auto column: Presentation::VBox()) {
                for(auto row: Presentation::HBox().id("spacer-row")) {
                    Presentation::Label("Column 1");
                    Presentation::Spacer().id("row-gap");
                    Presentation::Label("Column 2").id("row-end");
                }
                for(auto row: Presentation::HBox()) {
                    Presentation::Label("Column 1");
                    Presentation::Label("Column 2", 1);
                    Presentation::Label("Column 3");
                }
            }
            Presentation::Spacer();
        }
        void vbox(Model &) {
            for(auto column: Presentation::VBox(1).id("spacer-column")) {
                Presentation::Label("Row 1");
                Presentation::Divider();
                Presentation::Label("Row 2");
                Presentation::Spacer().id("column-gap");
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
            {"textfield", "TextField", nullptr, "Binding and text conversion"},
            {"scroll", "Scroll", nullptr, "Scroll and offset API"},
            {"canvas", "Canvas", nullptr, "Painter and input APIs"},
            {"drawshapely", "DrawShapely", nullptr, "Deferred geometry integration"},
            {"matplotlib", "Matplotlib", nullptr, "Deferred Python integration"},
            {"image", "Image", nullptr, "Image and size options"},
            {"tab", "Tab", nullptr, "Tabs and Tab"},
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

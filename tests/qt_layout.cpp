#include "cppui/qt/divider.hpp"
#include "cppui/qt/label.hpp"
#include "cppui/qt/window.hpp"
#include <QApplication>
#include <QEvent>
#include <stdexcept>

namespace {
    void require(bool value, const char * message) {
        if(!value) throw std::runtime_error(message);
    }
    class VirtualDivider: public CPPUI::Qt::QtView {
        public:
        void content() override { CPPUI::Divider(); }
    };
    class Root: public CPPUI::Qt::QtView {
        public:
        bool show = true;
        bool swap = false;
        ~Root() {
            clearChildren();
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        }
        void content() override {
            for(auto window: CPPUI::Window()) {
                for(auto column: CPPUI::VBox()) {
                    if(swap) CPPUI::Label("heading");
                    if(show) CPPUI::Divider().id("horizontal");
                    if(!swap) CPPUI::Label("heading");
                    for(auto row: CPPUI::HBox()) CPPUI::SubView<VirtualDivider>();
                }
            }
        }
    };
}
int main(int argc, char ** argv) {
    QApplication app(argc, argv);
    try {
        Root root;
        root.redraw();
        auto column = root.children->front()->children->front();
        auto outer = static_cast<QWidget *>(column->outer());
        require(outer->contentsMargins() == QMargins(11, 11, 11, 11), "outer layout padding differs from PUI");
        require(outer->layout()->contentsMargins() == QMargins(), "native box adds duplicate layout margins");
        auto row = column->children->back();
        auto nested = static_cast<QWidget *>(row->outer());
        require(nested->contentsMargins() == QMargins() && nested->layout()->contentsMargins() == QMargins(),
                "nested boxes add unwanted padding");
        auto horizontal = static_cast<QFrame *>(column->children->front()->outer());
        auto vertical = static_cast<QFrame *>(row->children->front()->children->front()->outer());
        require(horizontal->frameShape() == QFrame::HLine && vertical->frameShape() == QFrame::VLine,
                "divider did not use its physical parent's orientation");
        require(horizontal->frameShadow() == QFrame::Sunken && !column->children->front()->is_movable,
                "divider appearance/movement differs from PUI");
        root.swap = true;
        root.redraw();
        column = root.children->front()->children->front();
        require(column->children->at(1)->outer() == horizontal, "neighboring move replaced the fixed divider");
        require(column->children->back()->children->front()->children->front()->outer() == vertical,
                "retained virtual divider lost its native object");
        int removed = 0;
        QObject::connect(horizontal, &QObject::destroyed, &app, [&]() { ++removed; });
        root.show = false;
        root.redraw();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        require(removed == 1, "divider removal did not delete its native object once");
        {
            VirtualDivider unboxed;
            unboxed.redraw();
            require(static_cast<QFrame *>(unboxed.children->front()->outer())->frameShape() == QFrame::NoFrame,
                    "divider outside a box should have no frame");
        }
    } catch(const std::exception & error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

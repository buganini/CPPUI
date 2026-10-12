#include "cppui/qt/application.hpp"
#include "cppui/qt/button.hpp"
#include "cppui/qt/label.hpp"
#include "cppui/qt/layout.hpp"
#include "cppui/qt/window.hpp"
#include "cppui/state.hpp"

#include <QEvent>
#include <QStringList>
#include <stdexcept>

void buildExternalQtLabel();

namespace {
    void require(bool condition, const char * message) {
        if(!condition) {
            throw std::runtime_error(message);
        }
    }

    struct Model {
        CPPUI::State<int> left{1};
        CPPUI::State<int> right{1};
        CPPUI::State<int> nested{1};
        CPPUI::State<bool> showMiddle{true};
        bool reversed = false;
    };

    class Labels: public CPPUI::Qt::QtView {
        CPPUI::State<int> & count;
        std::string prefix;

        public:
        Labels(CPPUI::State<int> & value, const char * name): count(value), prefix(name) {}

        void content() override {
            for(int i = 0; i < count; ++i) {
                CPPUI::Label(prefix + "-" + std::to_string(i));
            }
        }
    };

    class Middle: public CPPUI::Qt::QtView {
        Model & model;

        public:
        explicit Middle(Model & value): model(value) {}

        void content() override {
            CPPUI::Label("middle");
            (new Labels(model.nested, "nested"))->id("nested");
        }
    };

    class Root: public CPPUI::Qt::QtView {
        Model & model;

        void left() { (new Labels(model.left, "left"))->id("left"); }
        void right() { (new Labels(model.right, "right"))->id("right"); }

        public:
        explicit Root(Model & value): model(value) {}
        ~Root() { clear(); }

        void clear() {
            destroyed = true;
            for(CPPUI::Node * child: *children) {
                CPPUI::recur_delete(child, true);
            }
            children->clear();
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        }

        void content() override {
            for(auto window: CPPUI::Window().id("window")) {
                for(auto column: CPPUI::VBox()) {
                    buildExternalQtLabel();
                    if(model.reversed) { right(); } else { left(); }
                    if(model.showMiddle) {
                        (new Middle(model))->id("middle");
                    }
                    if(model.reversed) { left(); } else { right(); }
                    CPPUI::Label("tail");
                }
            }
        }

        void addChild(int, CPPUI::Node * child) override {
            static_cast<QWidget *>(child->outer())->show();
        }

        void removeChild(int, CPPUI::Node * child) override {
            static_cast<QWidget *>(child->outer())->close();
        }

        QBoxLayout * layout() {
            auto window = static_cast<QMainWindow *>(children->front()->outer());
            return static_cast<QBoxLayout *>(window->centralWidget()->layout());
        }
    };

    QStringList labels(Root & root) {
        QStringList result;
        auto layout = root.layout();
        for(int i = 0; i < layout->count(); ++i) {
            auto label = qobject_cast<QLabel *>(layout->itemAt(i)->widget());
            require(label != nullptr, "unexpected native child");
            result.append(label->text());
        }
        return result;
    }

    void checkLabels(Root & root, const QStringList & expected) {
        QCoreApplication::processEvents();
        auto actual = labels(root);
        if(actual != expected) {
            throw std::runtime_error("unexpected native order: " + actual.join(", ").toStdString());
        }
    }
}

int main(int argc, char ** argv) {
    QApplication app(argc, argv);
    try {
        Model model;
        int removed = 0;
        Root root(model);
        root.sync();
        checkLabels(root, {"external", "left-0", "middle", "nested-0", "right-0", "tail"});
        auto column = root.layout();
        auto originalLeft = column->itemAt(1)->widget();
        QObject::connect(column->itemAt(2)->widget(), &QObject::destroyed,
                         &app, [&removed]() { ++removed; });
        QObject::connect(column->itemAt(3)->widget(), &QObject::destroyed,
                         &app, [&removed]() { ++removed; });

        model.left = 3;
        checkLabels(root, {"external", "left-0", "left-1", "left-2", "middle", "nested-0", "right-0", "tail"});
        require(root.layout()->itemAt(1)->widget() == originalLeft, "independent redraw remounted a label");

        root.sync();
        require(root.layout() == column, "unchanged redraw remounted the layout");
        model.nested = 2;
        checkLabels(root, {"external", "left-0", "left-1", "left-2", "middle", "nested-0", "nested-1", "right-0", "tail"});

        model.reversed = true;
        root.sync();
        checkLabels(root, {"external", "right-0", "middle", "nested-0", "nested-1", "left-0", "left-1", "left-2", "tail"});
        require(root.layout()->itemAt(5)->widget() == originalLeft, "keyed move remounted a view");

        model.showMiddle = false;
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        checkLabels(root, {"external", "right-0", "left-0", "left-1", "left-2", "tail"});
        require(removed == 2, "removed virtual descendants were not destroyed exactly once");
        model.nested = 3;
        checkLabels(root, {"external", "right-0", "left-0", "left-1", "left-2", "tail"});

        model.showMiddle = true;
        checkLabels(root, {"external", "right-0", "middle", "nested-0", "nested-1", "nested-2", "left-0", "left-1", "left-2", "tail"});
        model.left = 0;
        model.right = 0;
        checkLabels(root, {"external", "middle", "nested-0", "nested-1", "nested-2", "tail"});

        root.clear();
        require(app.topLevelWidgets().empty(), "native widgets leaked after teardown");
    } catch(const std::exception & error) {
        std::cerr << error.what() << std::endl;
        return 1;
    }
}

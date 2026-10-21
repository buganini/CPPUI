#include "cppui/state.hpp"
#include "cppui/qt/window.hpp"
#include "cppui/qt/layout.hpp"
#include "cppui/qt/label.hpp"
#include "cppui/qt/button.hpp"
#include "cppui/qt/image.hpp"
#include <QApplication>
#include <QStyleFactory>
#include <QTemporaryDir>
#include <stdexcept>

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    CPPUI::Node * find(CPPUI::Node & node, const char * id) {
        if(node.has_id && node._id == id) return &node;
        for(auto child: *node.children) if(auto match = find(*child, id)) return match;
        return nullptr;
    }
    class Child: public CPPUI::Qt::QtView {
        public:
        CPPUI::State<int> weight{2};
        void content() override { CPPUI::Label("nested").layout({}, {}, static_cast<int>(weight)).id("nested"); }
    };
    class Root: public CPPUI::Qt::QtView {
        public:
        int phase = 0;
        CPPUI::Optional<int> weight{700};
        bool overrideWeight=false;
        QStyle * nativeStyle = nullptr;
        std::string image;
        void content() override {
            for(auto window: CPPUI::Window().size(600, 400)) {
                auto & column = CPPUI::VBox(); column.id("box");
                if(phase == 0) {
                    CPPUI::QtOptions options;
                    options.SizeConstraint = QLayout::SetMinAndMaxSize;
                    column.layout(CPPUI::layout::padding(1, 2, 3, 4)).qt(options);
                } else if(phase == 1) column.layout({}, {}, {}, CPPUI::Insets(0));
                for(auto scope: column) {
                    auto & label = CPPUI::Label("text"); label.id("label");
                    if(phase == 0) {
                        CPPUI::QtOptions options;
                        options.HorizontalPolicy = QSizePolicy::Fixed;
                        options.VerticalPolicy = QSizePolicy::Maximum;
                        options.Style = nativeStyle;
                        options.StyleSheet = CPPUI::QtStyleSheet{{"border-width", "2px"}};
                        label.layout(CPPUI::layout::weight(3), CPPUI::layout::width(160)).layout({}, 45)
                            .style(CPPUI::style::fontFamily("Sans"), CPPUI::style::fontSize(20),
                                   CPPUI::style::color(0xff0000), CPPUI::style::bgColor(0x123456), CPPUI::style::fontWeight(CPPUI::FontWeight::DemiBold));
                        label.qt(options);
                        CPPUI::QtOptions replacement;
                        replacement.StyleSheet = CPPUI::QtStyleSheet{{"color", "#000000"}, {"font", "16pt"}};
                        label.qt(replacement);
                    } else if(phase == 1) {
                        label.layout(0, 0, 0).style(0, 0);
                        CPPUI::QtOptions options;
                        options.StyleSheet = CPPUI::QtStyleSheet{{"color", "#ff0000"}};
                        label.qt(options);
                        options.StyleSheet = CPPUI::QtStyleSheet{};
                        label.qt(options);
                    }
                    (new Child())->id("subview");
                    CPPUI::Button("button").id("button");
                    auto & weighted=CPPUI::Label("weight only");weighted.id("weighted");
                    if(weight) weighted.style(CPPUI::style::fontWeight(*weight));
                    if(overrideWeight) {
                        CPPUI::QtOptions options;
                        options.StyleSheet=CPPUI::QtStyleSheet{{"font-weight","300"}};
                        weighted.qt(options);
                    }
                    for(auto inherited:CPPUI::VBox().style(CPPUI::style::fontSize(18),CPPUI::style::fontWeight(CPPUI::FontWeight::Bold))) {
                        CPPUI::Label("inherited bold").id("inherited");
                        CPPUI::Label("normal child").style(CPPUI::style::fontWeight(CPPUI::FontWeight::Normal)).id("normal-child");
                    }
                    auto & picture = CPPUI::Image(image); picture.id("image");
                    if(phase == 0) picture.layout({}, 40);
                    if(phase == 1) picture.layout({}, 20);
                }
            }
        }
        QWidget * native(const char * id) { return static_cast<QWidget *>(find(*this, id)->outer()); }
    };
}
int main(int argc, char ** argv) {
    QApplication app(argc, argv);
    try {
        QTemporaryDir dir;
        QImage pixels(100, 50, QImage::Format_RGB32); pixels.fill(::Qt::red);
        const auto path = dir.filePath("image.png"); require(pixels.save(path), "image fixture failed");
        std::unique_ptr<QStyle> style(QStyleFactory::create("Fusion"));
        {
            Root root; root.nativeStyle = style.get(); root.image = path.toStdString(); root.sync();
            auto box = static_cast<QBoxLayout *>(root.native("box")->layout());
            auto label = root.native("label"); label->ensurePolished();
            require(root.native("box")->contentsMargins() == QMargins(4, 1, 2, 3), "padding order differs from PUI");
            require(box->sizeConstraint() == QLayout::SetMinAndMaxSize, "Qt constraint was not applied to the layout");
            require(label->sizeHint() == QSize(160, 45) && box->stretch(0) == 3, "layout dimensions/weight or repeated-call merge failed");
            require(label->sizePolicy().horizontalPolicy() == QSizePolicy::Fixed
                && label->sizePolicy().verticalPolicy() == QSizePolicy::Maximum, "Qt policies were not applied");
            require(label->styleSheet().contains("color:#000000;") && label->styleSheet().contains("background-color:#123456;")
                && label->font().pointSize() == 16 && label->font().weight()==QFont::DemiBold && !label->styleSheet().contains("border-width"),
                "style or Qt stylesheet replacement/precedence failed");
            auto weighted=root.native("weighted");weighted->ensurePolished();
            require(weighted->font().weight()==QFont::Bold && weighted->font().pointSize()==app.font().pointSize(),"weight-only style changed size or failed to apply");
            root.native("inherited")->ensurePolished();root.native("normal-child")->ensurePolished();
            require(root.native("inherited")->font().weight()==QFont::Bold
                && root.native("normal-child")->font().weight()==QFont::Normal
                && root.native("normal-child")->font().pointSize()==18,"font weight inheritance/override failed");
            require(static_cast<QLabel *>(root.native("image"))->pixmap().size() == QSize(80, 40), "image did not scale to requested height");
            auto subview = static_cast<Child *>(find(root, "subview"));
            subview->weight = 5; subview->sync();
            require(box->stretch(1) == 5, "independent virtual child redraw lost its stretch");
            root.phase = 1; root.sync();
            require(root.native("label") == label && label->sizeHint() == QSize(0, 0) && box->stretch(0) == 0,
                "zero dimensions/weight were treated as omitted");
            require(root.native("box")->contentsMargins() == QMargins() && box->sizeConstraint() == QLayout::SetDefaultConstraint,
                "zero padding or omitted Qt constraint did not reset");
            require(label->sizePolicy().horizontalPolicy() == QSizePolicy::Preferred && label->font().pointSize() == app.font().pointSize(),
                "omitted Qt policy/font did not reset");
            require(label->font().weight()==app.font().weight(),"omitted font weight did not reset");
            require(label->styleSheet().contains("color:#000000;"), "explicit empty Qt stylesheet did not clear previous overrides");
            require(static_cast<QLabel *>(root.native("image"))->pixmap().size() == QSize(40, 20), "image sizing change reused a stale scaled pixmap");
            root.phase = 2; root.sync();
            require(label->sizeHint() != QSize(0, 0) && label->styleSheet().isEmpty()
                && root.native("box")->contentsMargins() == QMargins(11, 11, 11, 11), "omitted layout/style did not restore defaults");
            require(static_cast<QLabel *>(root.native("image"))->pixmap().size() == QSize(100, 50), "omitted image size did not restore source pixels");
            for(int weight:{100,200,300,400,500,600,700,800,900,400}) {
                root.weight=weight;root.sync();
                require(root.native("weighted")==weighted && int(weighted->font().weight())==weight,"numeric weight change replaced widget or failed to apply");
            }
            root.weight=700;root.overrideWeight=true;root.sync();
            require(weighted->font().weight()==QFont::Light,"Qt font-weight override lost precedence");
            root.overrideWeight=false;root.weight.reset();root.sync();
            require(weighted->font().weight()==app.font().weight(),"removed weight retained a stale native font");
            for(int i = 0; i < 20; ++i) { root.phase = i % 3; root.sync(); }
            root.clearChildren();
        }
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        require(app.topLevelWidgets().isEmpty(), "modifier lifecycle leaked native roots");
    } catch(const std::exception & error) { std::cerr << error.what() << '\n'; return 1; }
}

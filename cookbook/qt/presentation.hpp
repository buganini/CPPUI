#ifndef CPPUI_COOKBOOK_QT_PRESENTATION
#define CPPUI_COOKBOOK_QT_PRESENTATION

#include <QFrame>
#include <QMenuBar>
#include <QMouseEvent>
#include <QScrollArea>
#include <QScrollBar>

// Cookbook-local presentation, pending the shared public layout/options API.
namespace Cookbook { namespace Presentation {
    struct Item {
        int weight;
        explicit Item(int weight = 0): weight(weight) {}
        virtual ~Item() = default;
    };

    template<class NativeBox>
    class Box: public NativeBox, public Item {
        public:
        explicit Box(int weight): Item(weight) {}
        void postSync() override {
            for(auto child: *this->children) {
                if(child->is_outoforder) continue;
                auto item = dynamic_cast<Item *>(child);
                if(auto spacer = dynamic_cast<CPPUI::Qt::Spacer *>(child)) {
                    for(int i = 0; i < this->layout->count(); ++i)
                        if(this->layout->itemAt(i) == spacer->ui) this->layout->setStretch(i, item ? item->weight : 0);
                } else this->layout->setStretchFactor(static_cast<QWidget *>(child->outer()), item ? item->weight : 0);
            }
        }
    };
    inline Box<CPPUI::Qt::HBox> & HBox(int weight = 0) { return *new Box<CPPUI::Qt::HBox>(weight); }
    inline Box<CPPUI::Qt::VBox> & VBox(int weight = 0) { return *new Box<CPPUI::Qt::VBox>(weight); }
    inline Box<CPPUI::Qt::VBox> & Sidebar() { return VBox(); }

    class WindowNode: public CPPUI::Qt::Window {
        public:
        WindowNode() { title("CPPUI Cookbook"); maximize(); }
        void update(CPPUI::Qt::BaseWidget * prev) override {
            CPPUI::Qt::Window::update(prev);
            auto window = static_cast<QMainWindow *>(ui);
            if(!prev || !prev->ui) {
                auto menu = window->menuBar()->addMenu("Menu");
                auto add = [window](QMenu * menu, const char * title, const char * output) {
                    auto action = menu->addAction(title);
                    QObject::connect(action, &QAction::triggered, window, [output]() { std::cout << output << '\n'; });
                };
                add(menu, "Action", "action");
                add(menu, "Action2", "action2");
                auto submenu = menu->addMenu("Menu2");
                add(submenu, "Action3", "action3");
                add(submenu, "Action4", "action4");
                auto menu3 = window->menuBar()->addMenu("Menu3");
                menu3->addAction("Action5");
                menu3->addAction("Action6");
            }
        }
    };
    inline WindowNode & Window() { return *new WindowNode(); }

    class ScrollNode: public CPPUI::Qt::BaseWidget, public Item {
        public:
        ScrollNode(): Item(1) {}
        void update(CPPUI::Qt::BaseWidget * prev) override {
            if(prev && prev->ui) ui = prev->ui;
            else {
                auto scroll = new QScrollArea();
                scroll->setWidgetResizable(true);
                scroll->setHorizontalScrollBarPolicy(::Qt::ScrollBarAlwaysOff);
                ui = scroll;
            }
        }
        void addChild(int, CPPUI::Node * child) override {
            static_cast<QScrollArea *>(ui)->setWidget(static_cast<QWidget *>(child->outer()));
        }
        void removeChild(int, CPPUI::Node *) override { static_cast<QScrollArea *>(ui)->takeWidget(); }
        void postSync() override {
            auto scroll = static_cast<QScrollArea *>(ui);
            if(scroll->widget()) {
                scroll->widget()->layout()->setSizeConstraint(QLayout::SetMinAndMaxSize);
                scroll->widget()->layout()->activate();
                scroll->setMinimumWidth(scroll->widget()->minimumSizeHint().width()
                    + scroll->verticalScrollBar()->sizeHint().width() + 2 * scroll->frameWidth());
            }
        }
    };
    inline ScrollNode & Scroll() { return *new ScrollNode(); }

    class PageLinkNode: public CPPUI::Qt::Label {
        public:
        explicit PageLinkNode(const std::string & text): Label(text) {}
    };
    inline PageLinkNode & PageLink(const std::string & text) { return *new PageLinkNode(text); }

    class SpacerNode: public CPPUI::Qt::Spacer, public Item {
        public:
        SpacerNode(): Item(1) {}
    };
    inline SpacerNode & Spacer() { return *new SpacerNode(); }
    inline CPPUI::Qt::Divider & Divider() { return CPPUI::Divider(); }

    class LabelNode: public CPPUI::Qt::Label, public Item {
        bool styled;
        public:
        LabelNode(const std::string & text, int weight, bool styled): Label(text), Item(weight), styled(styled) {}
        void update(CPPUI::Qt::BaseWidget * prev) override {
            CPPUI::Qt::Label::update(prev);
            ui->setStyleSheet(styled ? "font:20pt; color:#ffff00; background-color:#ff0000;" : "");
        }
    };
    inline LabelNode & Label(const std::string & text, int weight = 0) { return *new LabelNode(text, weight, false); }
    inline LabelNode & StyledLabel() { return *new LabelNode("Big Yellow", 0, true); }

    template<class NativeText>
    class GrayPane: public NativeText {
        public:
        explicit GrayPane(const std::string & text): NativeText(text) {}
        void update(CPPUI::Qt::BaseWidget * prev) override {
            NativeText::update(prev);
            this->ui->setStyleSheet("background-color:gray;");
        }
    };
    inline GrayPane<CPPUI::Qt::Label> & GrayLabel(const std::string & text) {
        return *new GrayPane<CPPUI::Qt::Label>(text);
    }
    inline GrayPane<CPPUI::Qt::Text> & GrayText(const std::string & text) {
        return *new GrayPane<CPPUI::Qt::Text>(text);
    }

    class StyledButtonNode: public CPPUI::Qt::Button {
        public:
        StyledButtonNode(): Button("Styled") {}
        void update(CPPUI::Qt::BaseWidget * prev) override {
            CPPUI::Qt::Button::update(prev);
            ui->setStyleSheet("font:20pt; color:#ff0000;");
        }
    };
    inline StyledButtonNode & StyledButton() { return *new StyledButtonNode(); }

    class ImageNode: public CPPUI::Qt::Image {
        public:
        explicit ImageNode(const std::string & path): Image(path) {}
        void update(CPPUI::Qt::BaseWidget * prev) override {
            CPPUI::Qt::Image::update(prev);
            static_cast<QLabel *>(ui)->setPixmap(pixmap.scaledToHeight(360, ::Qt::SmoothTransformation));
        }
    };
    inline ImageNode & Image(const std::string & path) { return *new ImageNode(path); }

    class CanvasNode: public CPPUI::Qt::Canvas {
        public:
        explicit CanvasNode(Painter painter): CPPUI::Qt::Canvas(std::move(painter)) {}
        void update(CPPUI::Qt::BaseWidget * prev) override {
            CPPUI::Qt::Canvas::update(prev);
            ui->setFixedSize(100, 100);
            auto palette = ui->palette();
            palette.setColor(QPalette::Window, QColor(0x55, 0x55, 0x55));
            ui->setPalette(palette);
            ui->setAutoFillBackground(true);
        }
    };
    inline CanvasNode & Canvas(CPPUI::Qt::Canvas::Painter painter) {
        return *new CanvasNode(std::move(painter));
    }
}}
#endif

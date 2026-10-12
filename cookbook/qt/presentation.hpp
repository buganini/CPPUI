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
                auto item = dynamic_cast<Item *>(child);
                this->layout->setStretchFactor(static_cast<QWidget *>(child->outer()), item ? item->weight : 0);
            }
        }
    };
    inline Box<CPPUI::Qt::HBox> & HBox(int weight = 0) { return *new Box<CPPUI::Qt::HBox>(weight); }
    inline Box<CPPUI::Qt::VBox> & VBox(int weight = 0) { return *new Box<CPPUI::Qt::VBox>(weight); }
    inline Box<CPPUI::Qt::VBox> & Sidebar() { return VBox(); }

    class WindowNode: public CPPUI::Qt::Window {
        public:
        void update(CPPUI::Qt::BaseWidget * prev) override {
            CPPUI::Qt::Window::update(prev);
            auto window = static_cast<QMainWindow *>(ui);
            window->setWindowTitle("CPPUI Cookbook");
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
                window->setWindowState(window->windowState() | ::Qt::WindowMaximized);
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

    class LinkLabel: public QLabel {
        public:
        std::function<void()> callback;
        void mouseReleaseEvent(QMouseEvent * event) override {
            if(event->button() == ::Qt::LeftButton) {
                auto invoke = callback;
                if(invoke) invoke();
                return;
            }
            QLabel::mouseReleaseEvent(event);
        }
    };
    class PageLinkNode: public CPPUI::Qt::Label {
        public:
        explicit PageLinkNode(const std::string & text): Label(text) {}
        void update(CPPUI::Qt::BaseWidget * prev) override {
            ui = prev && prev->ui ? prev->ui : new LinkLabel();
            auto label = static_cast<LinkLabel *>(ui);
            label->setText(QString::fromStdString(text));
            label->setTextFormat(::Qt::PlainText);
            label->callback = onClicked;
        }
        void destroy(bool direct) override {
            if(ui) static_cast<LinkLabel *>(ui)->callback = {};
            CPPUI::Qt::Label::destroy(direct);
        }
    };
    inline PageLinkNode & PageLink(const std::string & text) { return *new PageLinkNode(text); }

    class SpacerNode: public CPPUI::Qt::BaseWidget, public Item {
        public:
        SpacerNode(): Item(1) {}
        void update(CPPUI::Qt::BaseWidget * prev) override {
            ui = prev && prev->ui ? prev->ui : new QWidget();
            ui->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        }
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
    class StyledButtonNode: public CPPUI::Qt::Button {
        public:
        StyledButtonNode(): Button("Styled") {}
        void update(CPPUI::Qt::BaseWidget * prev) override {
            CPPUI::Qt::Button::update(prev);
            ui->setStyleSheet("font:20pt; color:#ff0000;");
        }
    };
    inline StyledButtonNode & StyledButton() { return *new StyledButtonNode(); }
}}
#endif

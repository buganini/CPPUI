#if defined(CPPUI_TEST_QT)
#include "cppui/qt/backend.hpp"
#elif defined(CPPUI_TEST_WX)
#include "cppui/wx/backend.hpp"
#else
#include "cppui/lvgl/backend.hpp"
#endif
#include <thread>
static void require(bool value,const char * message) {if(!value) throw std::runtime_error(message);}
static CPPUI::Node * find(CPPUI::Node & root,const std::string & id) {
    if(root._id==id) return &root;
    for(auto child:*root.children) if(auto result=find(*child,id)) return result;
    return nullptr;
}
static void pump() {
    for(int i=0;i<15;++i) {
#if defined(CPPUI_TEST_QT)
        QCoreApplication::processEvents();QCoreApplication::sendPostedEvents(nullptr,QEvent::LayoutRequest);
#elif defined(CPPUI_TEST_WX)
        wxTheApp->Yield(true);wxTheApp->ProcessPendingEvents();wxTheApp->ProcessIdle();
#else
        lv_tick_inc(LV_DEF_REFR_PERIOD+1);lv_timer_handler();lv_obj_update_layout(lv_screen_active());
#endif
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
struct Fragment:CPPUI::Backend::View {
    bool extra=false;
    void content() override {
        CPPUI::Label("fragment one").id("fragment-one");
        if(extra) CPPUI::Label("fragment two").id("fragment-two");
    }
};
struct Root:CPPUI::Application {
    using Application::Application;
    int columns=3,builds=0;
    bool hidden=false,reverse=false,automatic=true,responsive=false,present=true,expand=false;
    CPPUI::Optional<bool> dense{true};
    CPPUI::State<std::string> text{std::string("retained")};
    void content() override {
        ++builds;
        for(auto window:CPPUI::Window().id("window")) for(auto box:CPPUI::VBox().id("box")) {
            if(!present) continue;
            auto & grid=CPPUI::Grid();
            if(automatic) grid.columns(columns,CPPUI::grid::dense(dense),CPPUI::grid::expandChildren(expand));
            if(responsive) grid.columns(1,CPPUI::grid::expandChildren(false))
                .wideColumns(600,3,CPPUI::grid::expandChildren(true))
                .wideColumns(800,4);
            for(auto scope:grid.id("grid").layout(CPPUI::layout::weight(1))) {
                const std::vector<std::string> keys=reverse ? std::vector<std::string>{"c","b","a"} : std::vector<std::string>{"a","b","c"};
                for(const auto & key:keys) CPPUI::Label(key).id(key)
                    .grid(CPPUI::grid::row(key=="b" ? 1 : 0),CPPUI::grid::column(key=="c" ? 2 : 0),CPPUI::grid::columnspan(key=="c" ? 1 : 2))
                    .visible(!(hidden && key=="a"));
                CPPUI::TextField(text).id("editor").grid(CPPUI::grid::row(2),CPPUI::grid::column(0));
                if(automatic) CPPUI::SubView<Fragment>().id("fragment");
            }
        }
    }
    void mount() {
#if defined(CPPUI_TEST_QT) || defined(CPPUI_TEST_WX)
        sync();
#else
        redraw();
#endif
        pump();
    }
};
struct WideRoot:Root {
    using Root::Root;
    bool empty=false;
    void content() override {
        for(auto window:CPPUI::Window().id("window")) {
            auto & grid=CPPUI::Grid();
            if(automatic) grid.columns(3,CPPUI::grid::dense(true));
            for(auto scope:grid.id("grid").layout(CPPUI::layout::weight(1))) {
                if(empty) continue;
                CPPUI::TextField(text).grid(CPPUI::grid::row(0),CPPUI::grid::column(0))
                    .wideGrid(600,CPPUI::grid::row(1),CPPUI::grid::column(1),CPPUI::grid::columnspan(2))
                    .wideGrid(800,CPPUI::grid::rowspan(2)).wideGrid(600,CPPUI::grid::column(0))
                    .id("editor").ref("wide-grid-editor");
                CPPUI::Label("other").grid(CPPUI::grid::row(0),CPPUI::grid::column(2)).id("other");
            }
        }
    }
};
struct ExpansionRoot:Root {
    using Root::Root;
    int tileHeight=120;
    bool horizontal=false;
    void content() override {
        for(auto window:CPPUI::Window().id("window")) for(auto box:CPPUI::VBox())
            for(auto scroll:CPPUI::Scroll({},horizontal ? CPPUI::Optional<bool>() : CPPUI::Optional<bool>(false)).layout(CPPUI::layout::height(300)))
                for(auto grid:CPPUI::Grid().columns(2,CPPUI::grid::expandChildren(expand)).id("grid")) {
                    CPPUI::Label("A").id("a").grid(CPPUI::grid::rowspan(2)).layout(80,tileHeight);
                    CPPUI::Label("B").id("b").layout(40,60);
                    CPPUI::Label("C").id("c").grid(CPPUI::grid::rowspan(3),CPPUI::grid::columnspan(2)).layout(120,180);
                    CPPUI::Label("D").id("d").layout(40,60);
                }
    }
};
static CPPUI::Size nativeSize(CPPUI::Node * node) {
#if defined(CPPUI_TEST_QT)
    auto object=static_cast<QWidget *>(node->outer());return {object->width(),object->height()};
#elif defined(CPPUI_TEST_WX)
    auto size=static_cast<wxWindow *>(node->outer())->GetSize();return {size.x,size.y};
#else
    auto object=static_cast<lv_obj_t *>(node->outer());return {lv_obj_get_width(object),lv_obj_get_height(object)};
#endif
}
static void draft(CPPUI::Node * node) {
#if defined(CPPUI_TEST_QT)
    auto editor=static_cast<QLineEdit *>(node->outer());editor->setText("draft");editor->setSelection(1,2);
#elif defined(CPPUI_TEST_WX)
    auto editor=static_cast<wxTextCtrl *>(node->outer());editor->ChangeValue("draft");editor->SetSelection(1,3);
#else
    auto editor=static_cast<lv_obj_t *>(node->outer());lv_textarea_set_text(editor,"draft");lv_textarea_set_cursor_pos(editor,2);
#endif
}
static void checkDraft(CPPUI::Node * node) {
#if defined(CPPUI_TEST_QT)
    auto editor=static_cast<QLineEdit *>(node->outer());require(editor->text()=="draft" && editor->selectionStart()==1 && editor->selectedText()=="ra","Qt editor draft/selection lost");
#elif defined(CPPUI_TEST_WX)
    auto editor=static_cast<wxTextCtrl *>(node->outer());long start,end;editor->GetSelection(&start,&end);require(editor->GetValue()=="draft" && start==1 && end==3,"wx editor draft/selection lost");
#else
    auto editor=static_cast<lv_obj_t *>(node->outer());require(std::string(lv_textarea_get_text(editor))=="draft" && lv_textarea_get_cursor_pos(editor)==2,"LVGL editor draft/caret lost");
#endif
}
static void checkNative(CPPUI::Node * node) {
    require(bool(node->grid_cell),"missing effective grid cell");const auto cell=*node->grid_cell;
#if defined(CPPUI_TEST_QT)
    auto owner=static_cast<CPPUI::Qt::Grid *>(node->dom_parent);int row,col,rows,cols;
    owner->qtlayout->getItemPosition(owner->qtlayout->indexOf(static_cast<QWidget *>(node->outer())),&row,&col,&rows,&cols);
    require(CPPUI::GridCell(row,col,rows,cols)==cell,"Qt native cell differs from placement");
#elif defined(CPPUI_TEST_WX)
    auto owner=static_cast<CPPUI::Wx::Grid *>(node->dom_parent);auto sizer=static_cast<wxGridBagSizer *>(owner->sizer());
    auto item=sizer->FindItemAtPosition(wxGBPosition(cell.row,cell.column));require(item && item->GetSpan()==wxGBSpan(cell.rowspan,cell.columnspan),"wx native cell differs from placement");
#else
    auto object=static_cast<lv_obj_t *>(node->outer());
    require(lv_obj_get_style_grid_cell_row_pos(object,LV_PART_MAIN)==cell.row && lv_obj_get_style_grid_cell_column_pos(object,LV_PART_MAIN)==cell.column
        && lv_obj_get_style_grid_cell_row_span(object,LV_PART_MAIN)==cell.rowspan && lv_obj_get_style_grid_cell_column_span(object,LV_PART_MAIN)==cell.columnspan,"LVGL native cell differs from placement");
#endif
}
static void resize(Root & root,int width) {
#if defined(CPPUI_TEST_QT)
    auto window=static_cast<QWidget *>(find(root,"window")->outer());window->resize(width,500);window->show();
#elif defined(CPPUI_TEST_WX)
    auto window=static_cast<wxWindow *>(find(root,"window")->outer());window->SetClientSize(width,500);window->Show();
#else
    (void)root;lv_display_set_resolution(lv_display_get_default(),width,500);
#endif
    pump();
}
int main(int argc,char ** argv) {
#if !defined(CPPUI_TEST_QT) && !defined(CPPUI_TEST_WX)
    lv_init();lv_display_create(900,500);
#endif
    int result=0;
    try {
        Root root(argc,argv);root.mount();resize(root,900);
        require(*find(root,"c")->grid_cell==CPPUI::GridCell(0,2),"dense placement wrong");checkNative(find(root,"c"));
        auto editor=find(root,"editor")->outer();auto a=find(root,"a")->outer();draft(find(root,"editor"));
        for(int count:{1,3,2,3}) {
            root.columns=count;root.mount();
            require(find(root,"editor")->outer()==editor && find(root,"a")->outer()==a,"column reflow recreated native controls");
            require(find(root,"a")->grid_cell->columnspan==std::min(2,count) && find(root,"a")->grid_columnspan==2,"span clamp changed declared span");
            checkDraft(find(root,"editor"));checkNative(find(root,"b"));
        }
        root.columns=4;
        for(bool expand:{true,false,true}) {
            root.expand=expand;root.mount();
            require(find(root,"editor")->outer()==editor && find(root,"a")->outer()==a,"expansion replaced native controls");
            require(find(root,"fragment-one")->grid_cell->columnspan==(expand ? 2 : 1),"native expansion toggle did not update spans");
            require(find(root,"a")->grid_columnspan==2,"expansion changed declared native span");
            for(const char * key:{"a","b","c","editor","fragment-one"}) checkNative(find(root,key));
            checkDraft(find(root,"editor"));
        }
        root.expand=false;root.columns=3;root.mount();
        root.hidden=true;root.mount();require(!find(root,"a")->grid_cell && find(root,"a")->outer()==a,"hidden item was removed or occupied a cell");
        require(find(root,"b")->grid_cell->row==0 && find(root,"b")->grid_cell->column==0,"hidden cell did not compact");
        root.reverse=true;root.mount();checkDraft(find(root,"editor"));checkNative(find(root,"c"));
        auto fragment=static_cast<Fragment *>(find(root,"fragment"));fragment->extra=true;fragment->redraw();pump();
        auto added=find(root,"fragment-two");require(added && added->grid_cell,"independent virtual child did not reflow");checkNative(added);
        fragment->extra=false;fragment->redraw();pump();require(!find(root,"fragment-two"),"virtual child removal failed");
        root.hidden=false;root.reverse=false;root.dense=false;root.mount();require(*find(root,"c")->grid_cell==CPPUI::GridCell(1,2),"forward mode did not move candidate");
        root.dense.reset();root.mount();checkNative(find(root,"c"));
        root.automatic=false;root.mount();require(find(root,"editor")->outer()==editor,"auto-to-manual replaced editor");checkDraft(find(root,"editor"));checkNative(find(root,"a"));
        root.automatic=true;root.responsive=true;root.mount();
        for(int width:{450,750,950,450,950}) {
            resize(root,width);require(find(root,"grid")->grid_columns==(width==450 ? 1 : width==750 ? 3 : 4),"parent-width breakpoint did not reflow");
            require(find(root,"editor")->outer()==editor,"responsive reflow replaced editor");checkDraft(find(root,"editor"));checkNative(find(root,"c"));
        }
        int before=root.builds;pump();require(root.builds==before,"stable grid kept redrawing");
        root.present=false;root.mount();root.present=true;root.mount();
        root.present=false;root.mount();
        for(bool horizontal:{false,true}) {
            ExpansionRoot expanded(argc,argv);expanded.expand=true;expanded.horizontal=horizontal;expanded.mount();
            auto original=find(expanded,"b")->outer();
            const auto initialHeight=nativeSize(find(expanded,"a")).height;
            for(int height:{120,180,120}) for(bool enabled:{true,false,true}) {
                expanded.tileHeight=height;expanded.expand=enabled;expanded.mount();
                auto a=find(expanded,"a"),b=find(expanded,"b"),c=find(expanded,"c"),d=find(expanded,"d");
                require(b->outer()==original,"expansion recreated the tile");
                require(b->grid_cell->rowspan==(enabled ? 2 : 1),"content grid expansion span differs");
                require(enabled ? nativeSize(b).height==nativeSize(a).height : nativeSize(b).height<nativeSize(a).height,
                    "content grid did not fill or release the expanded row span");
                require(enabled ? nativeSize(d).width==nativeSize(c).width : nativeSize(d).width<nativeSize(c).width,
                    "content grid did not fill or release the expanded column span");
                if(enabled && height==120) require(nativeSize(a).height==initialHeight,
                    "content grid retained a previously stretched height");
            }
        }
        for(bool automatic:{true,false}) {
            WideRoot wide(argc,argv);wide.automatic=automatic;wide.mount();resize(wide,450);
            auto original=find(wide,"editor")->outer();draft(find(wide,"editor"));
            for(int width:{450,750,950,450,750}) {
                resize(wide,width);
                auto editorNode=find(wide,"editor");
                const auto size=find(wide,"grid")->size().get();
                require(bool(size),"wideGrid did not observe its physical parent");
                const bool matched=size->width>=600,tall=size->width>=800;
                require(editorNode->outer()==original,"wideGrid changed declaration identity");
                require(editorNode->grid_column==0 && editorNode->grid_row==int(matched)
                    && editorNode->grid_columnspan==(matched ? 2 : -1) && editorNode->grid_rowspan==(tall ? 2 : -1),
                    "wideGrid thresholds, order or fallback differ");
                require(editorNode->grid_cell->row==(automatic ? 0 : int(matched)),"wideGrid manual/automatic row differs");
                auto other=find(wide,"other");
                require(other->grid_cell->rowspan==(automatic && tall ? 2 : 1),"vertical expansion did not follow responsive spans");
                checkDraft(editorNode);checkNative(editorNode);checkNative(other);
            }
            wide.empty=true;wide.mount();wide.empty=false;wide.mount();
            checkNative(find(wide,"editor"));
        }
        std::cout<<"native grid passed\n";
    } catch(const std::exception & error) {std::cerr<<error.what()<<'\n';result=1;}
#if !defined(CPPUI_TEST_QT) && !defined(CPPUI_TEST_WX)
    lv_deinit();
#endif
    return result;
}

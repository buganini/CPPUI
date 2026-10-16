#include "cppui/wx/application.hpp"
#include "cppui/wx/window.hpp"
#include "cppui/wx/layout.hpp"
#include "cppui/wx/table.hpp"
#include "cppui/wx/tree.hpp"
#include <wx/stopwatch.h>
static void require(bool c,const char * m){if(!c)throw std::runtime_error(m);}
static void pump(){wxStopWatch clock;do{wxTheApp->Yield(true);wxTheApp->ProcessPendingEvents();wxTheApp->ProcessIdle();wxMilliSleep(1);}while(clock.Time()<25);}
static CPPUI::Node * find(CPPUI::Node & root,const std::string & id){if(root._id==id)return &root;for(auto c:*root.children)if(auto n=find(*c,id))return n;return nullptr;}
struct TableAdapter:CPPUI::BaseTableAdapter<int> {
    int value=3;bool fail=false;
    int data(int,int) const override{if(fail)throw std::runtime_error("expected adapter read failure");return value*10;}
    int editData(int,int) const override{return value;}
    void setData(int,int,const int & v) override{value=v;}
    bool editable(int,int) const override{return true;}
    int rowCount() const override{return 1;}int columnCount() const override{return 1;}
};
struct Entry {std::string text;std::weak_ptr<Entry> parent;std::vector<std::shared_ptr<Entry>> children;};
struct TreeAdapter:CPPUI::BaseTreeAdapter<Entry> {
    std::vector<Handle> roots;
    Handle parent(const Handle & node) const override{return node->parent.lock();}
    Handle child(const Handle & p,int index) const override{return (p?p->children:roots).at(index);}
    std::string data(const Handle & node) const override{return node->text;}
    int rowCount(const Handle & p) const override{return (p?p->children:roots).size();}
};
class Root:public CPPUI::Application {
public:
    using Application::Application;
    CPPUI::State<int> number{4};CPPUI::State<bool> flag{false};CPPUI::State<std::string> text{std::string("raw")};
    bool decorated=false;
    bool reverse=false,show=true;int calls=0,version=1;bool remove=false;
    std::shared_ptr<TableAdapter> tableAdapter=std::make_shared<TableAdapter>();
    std::shared_ptr<TreeAdapter> treeAdapter=std::make_shared<TreeAdapter>();
    void content() override {
        for(auto w:CPPUI::Window().size(600,500))for(auto v:CPPUI::VBox())if(show){
            for(auto t:CPPUI::Table().columnHeader({"Number","Text","Bool"}).id("table"))for(auto row:CPPUI::TableNode()){
                auto & choice=CPPUI::TableNode(number).choices(std::vector<std::pair<int,std::string>>{{4,"Four"},{5,"Five"}});
                if(decorated)choice.prefix("[").suffix("]");
                auto & cell=CPPUI::TableNode(text).display("Shown");if(decorated)cell.prefix("Price: $").suffix(" USD").placeholder("Enter price");int n=version;cell.click([this,n](){calls+=n;if(remove){show=false;sync();}});
                CPPUI::TableNode(flag);
            }
            CPPUI::Table(tableAdapter).id("adapter-table");
            for(auto tree:CPPUI::Tree().id("tree")){
                if(reverse)CPPUI::TreeNode("B").id("b");
                for(auto a:CPPUI::TreeNode("A").expand([this](){++calls;}).id("a"))CPPUI::TreeNode("Child").id("c");
                if(!reverse)CPPUI::TreeNode("B").id("b");
            }
            CPPUI::Tree(treeAdapter).id("adapter-tree");
        }
    }
    CPPUI::Wx::Table * table(const std::string & id="table"){return static_cast<CPPUI::Wx::Table *>(find(*this,id));}
    CPPUI::Wx::Tree * tree(const std::string & id="tree"){return static_cast<CPPUI::Wx::Tree *>(find(*this,id));}
};
int main(int argc,char ** argv){
    wxSetAssertHandler([](const wxString &,int,const wxString &,const wxString &c,const wxString &m){std::cerr<<c<<": "<<m<<'\n';std::abort();});
    try{
        Root root(argc,argv);auto a=std::make_shared<Entry>();a->text="Adapter A";auto b=std::make_shared<Entry>();b->text="Adapter B";auto c=std::make_shared<Entry>();c->text="Child";c->parent=a;a->children.push_back(c);root.treeAdapter->roots={a,b};root.sync();pump();
        auto table=root.table();require(table->model->GetValue(0,0)=="Four"&&table->model->GetValue(0,1)=="Shown","display values");
        require(table->model->edit(0,0,"5")&&int(root.number)==5,"typed edit");require(!table->model->edit(0,0,"5x")&&!table->model->edit(0,0,"5.5"),"invalid integer accepted");pump();
        table=root.table();require(table->model->edit(0,2,"True")&&bool(root.flag),"boolean edit");pump();
        auto grid=static_cast<wxGrid *>(root.table()->ui);grid->SetGridCursor(0,0);grid->EnableCellEditControl();auto editor=grid->GetCellEditor(0,0);auto choice=dynamic_cast<wxChoice *>(editor->GetControl());require(choice&&choice->GetStringSelection()=="Five","choice label/raw selection");choice->SetSelection(0);wxString next;require(editor->EndEdit(0,0,grid,"Five",&next)&&next=="4","choice raw conversion");editor->ApplyEdit(0,0,grid);editor->DecRef();pump();require(int(root.number)==4,"choice commit");
        grid=static_cast<wxGrid *>(root.table()->ui);grid->SetGridCursor(0,1);grid->EnableCellEditControl();editor=grid->GetCellEditor(0,1);auto input=dynamic_cast<wxTextCtrl *>(editor->GetControl());require(input&&input->GetValue()=="raw","editor shows display instead of raw");input->ChangeValue("draft");root.sync();require(!editor->EndEdit(0,1,grid,"Shown",&next),"stale editor committed");editor->DecRef();pump();require(std::string(root.text)=="raw","draft leaked across redraw");
        root.decorated=true;root.sync();pump();grid=static_cast<wxGrid *>(root.table()->ui);
        require(root.table()->model->GetValue(0,1)=="Price: $Shown USD", "decorations did not wrap display");
        grid->SetGridCursor(0,1);grid->EnableCellEditControl();editor=grid->GetCellEditor(0,1);
        auto decorated=dynamic_cast<CPPUI::Wx::TableEditor *>(editor);
        input=decorated?dynamic_cast<wxTextCtrl *>(decorated->field):nullptr;
        require(input && dynamic_cast<wxPanel *>(editor->GetWindow()) && input->GetValue()=="raw"
            && input->GetHint()=="Enter price", "decorated editor lost raw value or hint");
        input->ChangeValue("12");wxKeyEvent enter(wxEVT_KEY_DOWN);enter.m_keyCode=WXK_RETURN;enter.SetEventObject(input);
        input->GetEventHandler()->ProcessEvent(enter);pump();editor->DecRef();
        require(std::string(root.text)=="12", "decorated editor Enter did not commit raw value");
        grid=static_cast<wxGrid *>(root.table()->ui);grid->SetGridCursor(0,0);grid->EnableCellEditControl();editor=grid->GetCellEditor(0,0);
        decorated=dynamic_cast<CPPUI::Wx::TableEditor *>(editor);choice=decorated?dynamic_cast<wxChoice *>(decorated->field):nullptr;
        require(choice && choice->GetStringSelection()=="Four", "decorated choice lost raw selection");
        choice->SetSelection(1);wxCommandEvent selected(wxEVT_CHOICE,choice->GetId());selected.SetEventObject(choice);choice->ProcessWindowEvent(selected);
        editor->DecRef();pump();require(int(root.number)==5, "decorated choice did not commit raw typed value");
        root.decorated=false;root.sync();pump();require(root.table()->model->GetValue(0,1)=="Shown", "omitted decorations did not clear");
        auto tree=root.tree();auto native=static_cast<wxDataViewCtrl *>(tree->ui);auto item=wxDataViewItem(static_cast<CPPUI::Wx::TreeNode *>(find(root,"a"))->item.get());native->Expand(item);native->Select(item);pump();int callbacks=root.calls;
        root.reverse=true;root.sync();pump();tree=root.tree();require(tree->model->roots[1]==item.GetID()&&native->IsExpanded(item)&&native->GetSelection()==item,"keyed tree state lost");require(root.calls==callbacks,"restore emitted expand callback");
        auto adapterTree=root.tree("adapter-tree");auto adapterNative=static_cast<wxDataViewCtrl *>(adapterTree->ui);auto adapterItem=wxDataViewItem(adapterTree->model->roots[0]);adapterNative->Expand(adapterItem);adapterNative->Select(adapterItem);std::reverse(root.treeAdapter->roots.begin(),root.treeAdapter->roots.end());root.sync();pump();require(adapterNative->GetSelection()==adapterItem&&adapterNative->IsExpanded(adapterItem),"adapter identity lost");
        auto adapterTable=root.table("adapter-table");require(adapterTable->model->GetValue(0,0)=="30"&&adapterTable->model->edit(0,0,"8"),"adapter edit");root.sync();pump();require(root.table("adapter-table")->model->GetValue(0,0)=="80","adapter refresh");
        auto old=root.table()->ui;root.tableAdapter->fail=true;try{root.sync();require(false,"adapter failure swallowed");}catch(const std::runtime_error &){}root.tableAdapter->fail=false;require(root.table()->ui==old,"failed build replaced old native tree");root.sync();
        root.version=7;root.remove=true;root.sync();auto dispatch=root.table()->model;callbacks=root.calls;dispatch->dispatch(0,1,false);pump();require(!root.show&&root.calls==callbacks+7,"callback replacement/self removal");
        std::weak_ptr<TableAdapter> tableOwner=root.tableAdapter;std::weak_ptr<TreeAdapter> treeOwner=root.treeAdapter;root.tableAdapter.reset();root.treeAdapter.reset();require(tableOwner.expired()&&treeOwner.expired(),"retired adapter ownership retained");root.clearChildren();pump();
    }catch(const std::exception & e){std::cerr<<e.what()<<'\n';return 1;}return 0;
}

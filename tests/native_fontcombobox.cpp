#include "cppui/backend.hpp"
#include <stdexcept>
static void require(bool ok,const char * message){if(!ok)throw std::runtime_error(message);}
static CPPUI::Node * find(CPPUI::Node & n,const char * id){if(n._id==id)return &n;for(auto c:*n.children)if(auto p=find(*c,id))return p;return nullptr;}
static void pump(){for(int i=0;i<8;++i){
#if defined(CPPUI_BACKEND_QT)
 QCoreApplication::processEvents();QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
#elif defined(CPPUI_BACKEND_WX)
 wxTheApp->Yield(true);wxTheApp->ProcessPendingEvents();wxTheApp->ProcessIdle();
#else
 lv_tick_inc(20);lv_timer_handler();lv_obj_update_layout(lv_screen_active());
#endif
}}
struct Root:CPPUI::Application{
 using Application::Application;
 CPPUI::State<std::string> font{std::string("missing")};
 std::vector<std::string> fonts{"serif","monospace","serif"};
 bool disabled=false,defaults=false,show=true;
 int changes=0;
 void content()override{
  for(auto window:CPPUI::Window().size(600,400))if(show){
   auto & combo=defaults?CPPUI::FontComboBox(font):CPPUI::FontComboBox(font,fonts);
   combo.disabled(disabled).change([this](){++changes;}).id("font");
  }
 }
 void mount(){
#if defined(CPPUI_BACKEND_LVGL)
 redraw();
#else
 sync();
#endif
 pump();
 }
 void choose(int index){auto native=find(*this,"font")->outer();
#if defined(CPPUI_BACKEND_QT)
 auto combo=static_cast<QComboBox *>(native);combo->setCurrentIndex(index);QMetaObject::invokeMethod(combo,"activated",Qt::DirectConnection,Q_ARG(int,index));
#elif defined(CPPUI_BACKEND_WX)
 auto combo=static_cast<wxComboBox *>(native);combo->SetSelection(index);wxCommandEvent event(wxEVT_COMBOBOX);event.SetInt(index);combo->ProcessWindowEvent(event);
#else
 auto combo=static_cast<lv_obj_t *>(native);lv_dropdown_set_selected(combo,index);lv_obj_send_event(combo,LV_EVENT_VALUE_CHANGED,nullptr);
#endif
 pump();}
 std::string selected(){auto native=find(*this,"font")->outer();
#if defined(CPPUI_BACKEND_QT)
 return static_cast<QComboBox *>(native)->currentText().toStdString();
#elif defined(CPPUI_BACKEND_WX)
 return static_cast<wxComboBox *>(native)->GetValue().ToStdString(wxConvUTF8);
#else
 auto combo=static_cast<lv_obj_t *>(native);if(lv_dropdown_get_text(combo))return lv_dropdown_get_text(combo);
 char text[256];lv_dropdown_get_selected_str(combo,text,sizeof(text));return text;
#endif
 }
 int count(){auto native=find(*this,"font")->outer();
#if defined(CPPUI_BACKEND_QT)
 return static_cast<QComboBox *>(native)->count();
#elif defined(CPPUI_BACKEND_WX)
 return static_cast<wxComboBox *>(native)->GetCount();
#else
 auto combo=static_cast<lv_obj_t *>(native);
 return *lv_dropdown_get_options(combo) ? lv_dropdown_get_option_count(combo) : 0;
#endif
 }
};
int main(int argc,char ** argv){
#if defined(CPPUI_BACKEND_LVGL)
 lv_init();lv_display_create(600,400);
#endif
 int result=0;try{
  require(CPPUI::detail::fontChoices({"serif","serif","monospace"}).size()==2,"duplicate family names");
  bool invalid=false;try{CPPUI::detail::fontChoices({"  "});}catch(const std::invalid_argument &){invalid=true;}require(invalid,"blank family accepted");
  Root root(argc,argv);root.mount();require(root.count()==2 && std::string(root.font)=="missing" && root.changes==0 && root.selected().empty(),"build changed unavailable family");
  auto retained=find(root,"font")->outer();root.choose(1);require(std::string(root.font)=="monospace" && root.changes==1,"selection did not write family");
  root.disabled=true;root.font=std::string("serif");root.mount();require(root.selected()=="serif","disabled control did not reflect programmatic selection");root.choose(1);require(std::string(root.font)=="serif" && root.changes==1,"disabled selector changed binding");
  root.disabled=false;root.fonts={"monospace","serif"};root.mount();require(find(root,"font")->outer()==retained,"font list update replaced control");root.choose(0);require(std::string(root.font)=="monospace" && root.changes==2,"reordered choices used stale callback/value");
  root.fonts.clear();root.mount();require(root.count()==0 && std::string(root.font)=="monospace","empty list changed binding");
  root.defaults=true;root.mount();require(root.count()>0 && std::string(root.font)=="monospace","default families changed model");
#if defined(CPPUI_BACKEND_QT)
  require(qobject_cast<QFontComboBox *>(static_cast<QWidget *>(find(root,"font")->outer())),"default Qt selector is not QFontComboBox");
#endif
  root.show=false;root.mount();require(!find(root,"font"),"removed font picker retained declaration");
 }catch(const std::exception & e){std::cerr<<e.what()<<'\n';result=1;}
#if defined(CPPUI_BACKEND_LVGL)
 lv_deinit();
#endif
 return result;
}

#include "cppui/backend.hpp"
#include "cppui/gap.hpp"
#include <limits>
#include <stdexcept>
static void require(bool ok,const char * message){if(!ok)throw std::runtime_error(message);}
static CPPUI::Node * find(CPPUI::Node & n,const char * id){if(n._id==id)return &n;for(auto c:*n.children)if(auto p=find(*c,id))return p;return nullptr;}
static void pump(){for(int i=0;i<12;++i){
#if defined(CPPUI_BACKEND_QT)
 QCoreApplication::processEvents();QCoreApplication::sendPostedEvents(nullptr,QEvent::LayoutRequest);
#elif defined(CPPUI_BACKEND_WX)
 wxTheApp->Yield(true);wxTheApp->ProcessPendingEvents();wxTheApp->ProcessIdle();
#else
 lv_tick_inc(20);lv_timer_handler();lv_obj_update_layout(lv_screen_active());
#endif
}}
struct Rect{int x,y,w,h;};
static Rect rect(CPPUI::Node * node){
#if defined(CPPUI_BACKEND_QT)
 auto r=static_cast<QWidget *>(node->outer())->geometry();return {r.x(),r.y(),r.width(),r.height()};
#elif defined(CPPUI_BACKEND_WX)
 auto w=static_cast<wxWindow *>(node->outer());auto p=w->GetScreenPosition();auto s=w->GetSize();return {p.x,p.y,s.x,s.y};
#else
 auto w=static_cast<lv_obj_t *>(node->outer());return {lv_obj_get_x(w),lv_obj_get_y(w),lv_obj_get_width(w),lv_obj_get_height(w)};
#endif
}
struct Root:CPPUI::Application{
 using Application::Application;
 bool horizontal=true,inverse=false,hidden=false,spacer=false,grid=false,adaptive=false,defaults=false;
 double gap=0;
 void content()override{
  for(auto window:CPPUI::Window().size(600,400)){
   CPPUI::Node * box;
   if(grid)box=&CPPUI::Grid(CPPUI::Gap(gap,gap+3));
   else if(adaptive)box=&CPPUI::HVBox(horizontal?1:1000,inverse,inverse,gap);
   else if(horizontal)box=defaults?&CPPUI::HBox(inverse):&CPPUI::HBox(inverse,gap);
   else box=defaults?&CPPUI::VBox(inverse):&CPPUI::VBox(inverse,gap);
   for(auto scope:box->id("box").layout(CPPUI::layout::padding(0))){
    CPPUI::Label("A").id("a").layout(30,30,{},CPPUI::Insets(0)).grid(CPPUI::grid::row(0),CPPUI::grid::column(0));
    if(!grid){
     if(spacer)CPPUI::Spacer().layout(CPPUI::layout::weight(0)).visible(!hidden);
     else CPPUI::Label("B").id("b").layout(30,30,{},CPPUI::Insets(0)).visible(!hidden);
    }
    CPPUI::Label("C").id("c").layout(30,30,{},CPPUI::Insets(0)).grid(CPPUI::grid::row(0),CPPUI::grid::column(1));
    if(grid)CPPUI::Label("D").id("d").layout(30,30,{},CPPUI::Insets(0)).grid(CPPUI::grid::row(1),CPPUI::grid::column(0));
   }
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
};
int main(int argc,char ** argv){
#if defined(CPPUI_BACKEND_LVGL)
 lv_init();lv_display_create(600,400);
#endif
 int result=0;try{
  for(double value:{-1.,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}){
   bool rejected=false;try{CPPUI::Gap gap(value);(void)gap;}catch(const std::invalid_argument &){rejected=true;}require(rejected,"invalid gap accepted");
  }
  require(CPPUI::Gap(2.5,3.5).rows==2 && CPPUI::Gap(2.5,3.5).columns==4,"gap rounding differs");
  Root root(argc,argv);
  for(bool adaptive:{false,true})for(bool horizontal:{true,false})for(bool inverse:{false,true})for(bool spacer:{false,true}){
   root.adaptive=adaptive;root.horizontal=horizontal;root.inverse=inverse;root.spacer=spacer;
   void * retained=nullptr;
   for(double gap:{0.,12.,3.5,0.})for(bool hidden:{false,true}){
    root.gap=gap;root.hidden=hidden;root.mount();auto node=find(root,"a");
    if(retained)require(node->outer()==retained,"gap changed child identity");retained=node->outer();
    auto a=rect(node),c=rect(find(root,"c"));
    const int distance=horizontal?std::abs(c.x-a.x)-30:std::abs(c.y-a.y)-30;
    const int expected=CPPUI::detail::gapPixels(gap)*(hidden?1:2)+(!hidden && !spacer?30:0);
    require(distance==expected,"box gap differs across direction, visibility or Spacer");
   }
  }
  root.adaptive=false;root.defaults=true;root.hidden=true;root.mount();
  auto a=rect(find(root,"a")),c=rect(find(root,"c"));require(std::abs(c.y-a.y)-30==10,"default Box gap is not 10");
  root.grid=true;
  for(double gap:{0.,12.,4.,0.}){
   root.gap=gap;root.mount();auto a=rect(find(root,"a")),c=rect(find(root,"c")),d=rect(find(root,"d"));
   require(c.x-a.x-a.w==gap+3 && d.y-a.y-a.h==gap,"Grid row/column gaps differ");
  }
 }catch(const std::exception & e){std::cerr<<e.what()<<'\n';result=1;}
#if defined(CPPUI_BACKEND_LVGL)
 lv_deinit();
#endif
 return result;
}

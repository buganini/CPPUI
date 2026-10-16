#include "cppui/wx/application.hpp"
#include "cppui/wx/window.hpp"
#include "cppui/wx/layout.hpp"
#include "cppui/wx/image.hpp"
#include "cppui/wx/label.hpp"
#include "cppui/wx/canvas.hpp"
#include <wx/dcmemory.h>
#include <wx/filename.h>
#include <wx/stopwatch.h>
#include <cstdio>
static void require(bool c,const char * m){if(!c)throw std::runtime_error(m);}
static void pump(){wxStopWatch clock;do{wxTheApp->Yield(true);wxTheApp->ProcessPendingEvents();wxTheApp->ProcessIdle();wxMilliSleep(1);}while(clock.Time()<60);}
static CPPUI::Node * find(CPPUI::Node & node,const std::string & id){if(node._id==id)return &node;for(auto child:*node.children)if(auto result=find(*child,id))return result;return nullptr;}
class Root:public CPPUI::Application {
public:
    using Application::Application;
    std::string path;
    bool show=true,remove=false;
    int paints=0,version=0,last=-1,moves=0,wheels=0,leaves=0;
    bool input=true;
    CPPUI::CanvasPointerEvent lastInput;
    std::shared_ptr<int> token=std::make_shared<int>(1);
    void content() override {
        if(path.empty())return;
        for(auto w:CPPUI::Window().size(300,300))for(auto v:CPPUI::VBox()) {
            for(auto layers:CPPUI::Stack().id("layers")){CPPUI::Label("Bottom").layout(100,50).id("bottom");CPPUI::Label("Top").id("top");}
            CPPUI::Image(path).layout({},32).id("image");
            if(show){auto keep=token;int snapshot=version;auto & canvas=CPPUI::Canvas([this,keep,snapshot](CPPUI::CanvasPainter & c){++paints;last=snapshot;c.drawRect(0,0,50,50,0xff0000);if(remove){show=false;sync();}return paints<3;});
                if(input)canvas.mousemove([this,snapshot](const CPPUI::CanvasPointerEvent & e){moves+=snapshot+1;lastInput=e;})
                    .wheel([this,snapshot](const CPPUI::CanvasPointerEvent & e){wheels+=snapshot+1;lastInput=e;})
                    .mouseleave([this,snapshot](){leaves+=snapshot+1;});
                canvas.layout(100,100).id("canvas");}
        }
    }
};
static void frameStyles(){
    auto render=[](bool isolate,const wxColour & foreground){
        wxBitmap bitmap(240,150,32);wxMemoryDC dc(bitmap);dc.SetBackground(*wxWHITE_BRUSH);dc.Clear();
        {std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        CPPUI::CanvasPainter c(*gc,240,150,1,*wxNORMAL_FONT,foreground);
        std::vector<std::function<void()>> operations{
            [&](){c.drawRect(4,4,40,30,0xff0000,0x224466,3);},
            [&](){c.drawLine(45,15,90,20,0x00ff00,4);},
            [&](){c.drawRect(100,4,130,30);},
            [&](){c.drawText(10,40,"Default after NoPen",-1,-1,-1);},
            [&](){c.drawText(10,80,"Rotated",-1,-1,12,0xff0000,15);},
            [&](){c.drawText(10,110,"Default again",-1,-1,-1);}};
        for(const auto & op:operations){if(isolate)gc->PushState();op();if(isolate)gc->PopState();}}
        dc.SelectObject(wxNullBitmap);auto image=bitmap.ConvertToImage();
        return std::vector<unsigned char>(image.GetData(),image.GetData()+240*150*3);
    };
    for(auto foreground:{*wxBLACK,*wxBLUE})require(render(false,foreground)==render(true,foreground),"mixed Canvas styles affected later operations");
    require(render(false,*wxBLACK)!=render(false,*wxBLUE),"Canvas cached a prior frame's default text color");
}
static void scopes(){
    auto render=[](bool raw){
        wxBitmap bitmap(180,110,32);wxMemoryDC dc(bitmap);dc.SetBackground(*wxWHITE_BRUSH);dc.Clear();
        {std::unique_ptr<wxGraphicsContext> native(wxGraphicsContext::Create(dc));
         CPPUI::CanvasPainter c(*native,180,110,1);
         c.drawRect(5,5,30,30,0xff0000,0x0000ff);
         c.push();c.drawRect(40,5,65,30,0x00ff00,0xff0000);
         c.push();
         if(raw){native->SetPen(wxPen(*wxYELLOW));native->SetBrush(wxBrush(*wxCYAN));}
         else c.drawRect(-40,-40,-20,-20,0xffff00,0x00ffff);
         c.pop();native->DrawRectangle(75,5,25,25);
         c.drawRect(75,40,100,65,0x00ff00,0xff0000);
         c.pop();native->DrawRectangle(110,5,25,25);
         c.drawText(5,80,"Restored");c.drawRect(110,40,135,65,0xff0000,0x0000ff);
         bool threw=false;try{c.pop();}catch(const std::logic_error &){threw=true;}
         require(threw,"unmatched Canvas pop did not fail");}
        dc.SelectObject(wxNullBitmap);auto image=bitmap.ConvertToImage();
        return std::vector<unsigned char>(image.GetData(),image.GetData()+180*110*3);
    };
    require(render(false)==render(true),"nested wx scope failed to restore pen/brush and helper styles");
}
int main(int argc,char ** argv){
    wxSetAssertHandler([](const wxString &,int,const wxString &,const wxString &c,const wxString &m){std::cerr<<c<<": "<<m<<'\n';std::abort();});
    try{
        Root root(argc,argv);root.sync();frameStyles();scopes();root.path=wxFileName::CreateTempFileName("cppui-wx-image-").ToStdString(wxConvUTF8);
        wxImage input(8,4);input.SetRGB(wxRect(0,0,8,4),0,255,0);wxInitAllImageHandlers();require(input.SaveFile(wxString::FromUTF8(root.path),wxBITMAP_TYPE_PNG),"image fixture write failed");
        auto source=CPPUI::ImageResource::load(root.path);require(source.width()==8&&source.height()==4,"resource load");require(source.scale(16,16).height()==8,"aspect ratio");require(source.crop(1,1,2,2).width()==2&&source.crop(-1,0,2,2).isNull(),"crop bounds");require(CPPUI::ImageResource::load(root.path+"missing").isNull(),"missing resource");
        wxBitmap bitmap(100,100,32);wxMemoryDC dc(bitmap);dc.SetBackground(*wxWHITE_BRUSH);dc.Clear();
        {std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));CPPUI::CanvasPainter c(*gc,100,100,1);
        c.drawRect(25,25,5,5,0xff0000);c.drawEllipse(50,15,8,8,0x0000ff);c.drawImage(source,70,5,20,20);c.drawLine(0,40,90,40,0);c.drawPolyline({{0,45},{40,48}},0);c.drawCubicBezier({0,50},{20,55},{40,55},{60,50},0);c.drawPolygon({{60,60},{80,60},{70,80}},0x123456);c.drawText(0,80,"wx 中文",-1,-1,10,0);c.drawImage(source,30,60,10,10,0,0,4,4,.5);}
        dc.SelectObject(wxNullBitmap);auto pixels=bitmap.ConvertToImage();require(pixels.GetRed(10,10)>240&&pixels.GetGreen(10,10)<10,"rectangle pixels");require(pixels.GetBlue(50,15)>240,"ellipse pixels");require(pixels.GetGreen(75,10)>240&&pixels.GetRed(75,10)<10,"image pixels");require(pixels.GetGreen(35,65)>240&&pixels.GetRed(35,65)>100,"image opacity");
        root.sync();pump();auto bottom=static_cast<wxWindow *>(find(root,"bottom")->outer());auto top=static_cast<wxWindow *>(find(root,"top")->outer());require(bottom->GetPosition()==top->GetPosition()&&bottom->GetSize()==top->GetSize(),"stack layers must overlap");require(root.paints>0&&root.last==0,"native paint");root.version=1;root.sync();pump();require(root.last==1,"paint replacement");
        auto canvas=static_cast<wxWindow *>(find(root,"canvas")->outer());
        for(int i=1;i<=3;++i){
            root.version=i;root.sync();int before=root.wheels;
            wxMouseEvent wheel(wxEVT_MOUSEWHEEL);wheel.SetPosition(wxPoint(20,30));wheel.m_wheelRotation=120;wheel.SetEventObject(canvas);canvas->ProcessWindowEvent(wheel);
            require(root.wheels==before+i+1 && root.lastInput.x==20 && root.lastInput.y==30 && root.lastInput.angleY==120 && root.lastInput.pixelY==0,"Canvas wheel units/current callback failed");
            before=root.moves;wxMouseEvent move(wxEVT_MOTION);move.SetPosition(wxPoint(12,34));move.SetEventObject(canvas);canvas->ProcessWindowEvent(move);
            require(root.moves==before+i+1 && root.lastInput.x==12 && root.lastInput.y==34,"Canvas move coordinates/current callback failed");
            before=root.leaves;wxMouseEvent leave(wxEVT_LEAVE_WINDOW);leave.SetEventObject(canvas);canvas->ProcessWindowEvent(leave);
            require(root.leaves==before+i+1,"Canvas leave callback failed");
        }
        root.input=false;root.sync();int before=root.wheels;wxMouseEvent wheel(wxEVT_MOUSEWHEEL);wheel.m_wheelRotation=120;wheel.SetEventObject(canvas);canvas->ProcessWindowEvent(wheel);
        require(root.wheels==before,"omitted Canvas callback retained dispatch");
        std::weak_ptr<int> token=root.token;root.token.reset();root.remove=true;root.sync();pump();int paints=root.paints;pump();require(!root.show&&root.paints==paints&&token.expired(),"paint self removal retained callback");
        root.clearChildren();pump();std::remove(root.path.c_str());
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}return 0;
}

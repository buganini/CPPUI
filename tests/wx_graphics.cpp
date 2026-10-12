#include "cppui/wx/application.hpp"
#include "cppui/wx/window.hpp"
#include "cppui/wx/layout.hpp"
#include "cppui/wx/image.hpp"
#include "cppui/wx/canvas.hpp"
#include <wx/dcmemory.h>
#include <wx/filename.h>
#include <wx/stopwatch.h>
#include <cstdio>
static void require(bool c,const char * m){if(!c)throw std::runtime_error(m);}
static void pump(){wxStopWatch clock;do{wxTheApp->Yield(true);wxTheApp->ProcessPendingEvents();wxTheApp->ProcessIdle();wxMilliSleep(1);}while(clock.Time()<60);}
class Root:public CPPUI::Application {
public:
    using Application::Application;
    std::string path;
    bool show=true,remove=false;
    int paints=0,version=0,last=-1;
    std::shared_ptr<int> token=std::make_shared<int>(1);
    void content() override {
        if(path.empty())return;
        for(auto w:CPPUI::Window().size(300,300))for(auto v:CPPUI::VBox()) {
            CPPUI::Image(path).layout({},32).id("image");
            if(show){auto keep=token;int snapshot=version;CPPUI::Canvas([this,keep,snapshot](CPPUI::CanvasPainter & c){++paints;last=snapshot;c.drawRect(0,0,50,50,0xff0000);if(remove){show=false;sync();}return paints<3;}).layout(100,100).id("canvas");}
        }
    }
};
int main(int argc,char ** argv){
    wxSetAssertHandler([](const wxString &,int,const wxString &,const wxString &c,const wxString &m){std::cerr<<c<<": "<<m<<'\n';std::abort();});
    try{
        Root root(argc,argv);root.sync();root.path=wxFileName::CreateTempFileName("cppui-wx-image-").ToStdString(wxConvUTF8);
        wxImage input(8,4);input.SetRGB(wxRect(0,0,8,4),0,255,0);wxInitAllImageHandlers();require(input.SaveFile(wxString::FromUTF8(root.path),wxBITMAP_TYPE_PNG),"image fixture write failed");
        auto source=CPPUI::ImageResource::load(root.path);require(source.width()==8&&source.height()==4,"resource load");require(source.scale(16,16).height()==8,"aspect ratio");require(source.crop(1,1,2,2).width()==2&&source.crop(-1,0,2,2).isNull(),"crop bounds");require(CPPUI::ImageResource::load(root.path+"missing").isNull(),"missing resource");
        wxBitmap bitmap(100,100,32);wxMemoryDC dc(bitmap);dc.SetBackground(*wxWHITE_BRUSH);dc.Clear();
        {std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));CPPUI::CanvasPainter c(*gc,100,100,1);
        c.drawRect(25,25,5,5,0xff0000);c.drawEllipse(50,15,8,8,0x0000ff);c.drawImage(source,70,5,20,20);c.drawLine(0,40,90,40,0);c.drawPolyline({{0,45},{40,48}},0);c.drawCubicBezier({0,50},{20,55},{40,55},{60,50},0);c.drawPolygon({{60,60},{80,60},{70,80}},0x123456);c.drawText(0,80,"wx 中文",-1,-1,10,0);c.drawImage(source,30,60,10,10,0,0,4,4,.5);}
        dc.SelectObject(wxNullBitmap);auto pixels=bitmap.ConvertToImage();require(pixels.GetRed(10,10)>240&&pixels.GetGreen(10,10)<10,"rectangle pixels");require(pixels.GetBlue(50,15)>240,"ellipse pixels");require(pixels.GetGreen(75,10)>240&&pixels.GetRed(75,10)<10,"image pixels");require(pixels.GetGreen(35,65)>240&&pixels.GetRed(35,65)>100,"image opacity");
        root.sync();pump();require(root.paints>0&&root.last==0,"native paint");root.version=1;root.sync();pump();require(root.last==1,"paint replacement");
        std::weak_ptr<int> token=root.token;root.token.reset();root.remove=true;root.sync();pump();int paints=root.paints;pump();require(!root.show&&root.paints==paints&&token.expired(),"paint self removal retained callback");
        root.clearChildren();pump();std::remove(root.path.c_str());
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}return 0;
}

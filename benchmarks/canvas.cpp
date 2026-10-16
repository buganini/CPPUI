// Compile twice against different CPPUI include roots; see doc/canvas-performance.md.
#if defined(CPPUI_BENCH_WX)
#include "cppui/wx/application.hpp"
#include "cppui/wx/canvas.hpp"
#include <wx/dcmemory.h>
class Runtime:public CPPUI::Application {public:using Application::Application;void content() override {}};
#else
#include "cppui/qt/canvas.hpp"
#include <QApplication>
#endif
#include "../cookbook/heavycanvas_data.hpp"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <stdexcept>

using Clock=std::chrono::steady_clock;
static const int width=1024,height=720;
static void draw(CPPUI::CanvasPainter & p,const std::string & workload,int count,
                 const Cookbook::HeavyCanvasData::ChartData * data){
    if(workload!="heavy"){
        const int colors[]={0xF07883,0x45C6B0,0xF5C45E,0x71B7FF};
        for(int i=0;i<count;++i){
            const int color=colors[workload=="same"?0:i%4];
            const int x=i%1000,y=(i/1000)%700;
            p.drawLine(x,y,x+2,y+2,color);
            p.drawRect(x,y+3,x+2,y+5,color,color);
        }
        return;
    }
    // Same OHLCV and six indicator passes as the shared HeavyCanvas recipe.
    const auto & rows=data->rows;
    double low=rows.front().lower,high=rows.front().upper,volume=0;
    for(const auto & r:rows){low=std::min(low,std::min(r.low,r.lower));high=std::max(high,std::max(r.high,r.upper));volume=std::max(volume,r.volume);}
    const double padding=(high-low)*.06;low-=padding;high+=padding;
    struct Panel {double top,bottom,low,high;};
    const Panel panels[]={{24,288,low,high},{332,420,0,volume*1.05},{464,552,0,100},{596,684,0,100}};
    const double step=(width-76.)/count,body=std::max(1,std::min(5,int(step*.65)));
    auto x=[&](int i){return 62+(i+.5)*step;};
    auto y=[&](double value,int panel){const auto & q=panels[panel];return q.bottom-(value-q.low)/(q.high-q.low)*(q.bottom-q.top);};
    for(int i=0;i<count;++i){
        const auto & r=rows[i];const int color=r.close>=r.open?0xF07883:0x45C6B0;const double cx=x(i),a=y(r.open,0),b=y(r.close,0);
        p.drawLine(cx,y(r.low,0),cx,y(r.high,0),color);
        p.drawRect(cx-body/2,std::min(a,b),cx+body/2,std::max(a,b)+1,color);
        p.drawRect(cx-body/2,y(r.volume,1),cx+body/2,panels[1].bottom,color);
    }
    using Row=Cookbook::HeavyCanvasData::Row;
    const double Row::* indicators[]={&Row::upper,&Row::mid,&Row::lower,&Row::k,&Row::d,&Row::rsi};
    const int positions[]={0,0,0,2,2,3},colors[]={0x71B7FF,0xF5C45E,0xBA95EF,0xF5C45E,0x71B7FF,0xF5C45E};
    for(int n=0;n<6;++n)for(int i=1;i<count;++i)
        p.drawLine(x(i-1),y(rows[i-1].*indicators[n],positions[n]),x(i),y(rows[i].*indicators[n],positions[n]),colors[n],2);
}
static void hashByte(std::uint64_t & hash,unsigned char byte){hash=(hash^byte)*1099511628211ULL;}
int main(int argc,char ** argv){
    try {
        const std::string workload=argc>1?argv[1]:"heavy";
        const int count=argc>2?std::stoi(argv[2]):30000,repeats=argc>3?std::stoi(argv[3]):9;
        if((workload!="same"&&workload!="mixed"&&workload!="heavy")||count<=0||repeats<=0)throw std::runtime_error("usage: benchmark same|mixed|heavy count repeats");
#if defined(CPPUI_BENCH_WX)
        Runtime runtime(argc,argv);runtime.sync();
#else
        QApplication app(argc,argv);
#endif
        auto data=workload=="heavy"?Cookbook::HeavyCanvasData::generate(count,42):nullptr;
        std::vector<double> samples;std::uint64_t hash=14695981039346656037ULL;
        for(int round=-2;round<repeats;++round){
#if defined(CPPUI_BENCH_WX)
            wxBitmap bitmap(width,height,32);wxMemoryDC dc(bitmap);dc.SetBackground(*wxWHITE_BRUSH);dc.Clear();
            auto begin=Clock::now();
            {std::unique_ptr<wxGraphicsContext> native(wxGraphicsContext::Create(dc));
             {CPPUI::CanvasPainter painter(*native,width,height,1);draw(painter,workload,count,data.get());}
             native->Flush();}
            auto end=Clock::now();dc.SelectObject(wxNullBitmap);
            if(round==repeats-1){auto image=bitmap.ConvertToImage();auto bytes=image.GetData();for(int i=0;i<width*height*3;++i)hashByte(hash,bytes[i]);}
#else
            QImage image(width,height,QImage::Format_ARGB32_Premultiplied);image.fill(Qt::white);
            auto begin=Clock::now();
            {QPainter native(&image);
#ifndef CPPUI_BENCH_NO_AA
             native.setRenderHint(QPainter::Antialiasing);
#endif
             CPPUI::CanvasPainter painter(native,width,height,1);draw(painter,workload,count,data.get());}
            auto end=Clock::now();
            if(round==repeats-1)for(int y=0;y<height;++y)for(int x=0;x<width;++x){const auto pixel=image.pixel(x,y);hashByte(hash,qRed(pixel));hashByte(hash,qGreen(pixel));hashByte(hash,qBlue(pixel));}
#endif
            if(round>=0)samples.push_back(std::chrono::duration<double,std::milli>(end-begin).count());
        }
        std::cout<<workload<<','<<count<<','<<std::hex<<hash<<std::dec;
        for(double sample:samples)std::cout<<','<<std::fixed<<std::setprecision(3)<<sample;
        std::cout<<'\n';
    }catch(const std::exception & e){std::cerr<<e.what()<<'\n';return 1;}
}

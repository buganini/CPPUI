#include "cppui/backend.hpp"
#include "recipes.hpp"
#include "heavycanvas_data.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace Cookbook {
    namespace {
        const int background=0x101A2B, gridColor=0x28374D, muted=0x94A6BE, textColor=0xDCE5F2;
        const int up=0xF07883, down=0x45C6B0, gold=0xF5C45E, blue=0x71B7FF, purple=0xBA95EF;
        std::string number(double value, int digits=2) {
            std::ostringstream out; out << std::fixed << std::setprecision(digits) << value; return out.str();
        }
        using HeavyCanvasData::Row;
        using HeavyCanvasData::ChartData;
        using HeavyCanvasData::generate;
        struct Viewport { int generation=0, start=0, stop=0; };
        struct Cursor { bool visible=false; double x=0, y=0; };
    }
    struct HeavyChart {
        CPPUI::State<std::string> samples{std::string("1000")};
        CPPUI::State<int> seed{42};
        CPPUI::State<bool> continuous{false}, fullFrameChanges{false};
        unsigned paintSequence=0;
        CPPUI::State<Viewport> viewport{Viewport()};
        CPPUI::State<Cursor> cursor{Cursor()};
        std::shared_ptr<ChartData> data;
        int count=0,cachedSeed=0,generation=0,width=0,height=0;
        std::pair<int,int> bounds() {
            const auto view=static_cast<Viewport>(viewport);
            return view.generation==generation ? std::make_pair(view.start,view.stop) : std::make_pair(0,count);
        }
        void zoom(const CPPUI::CanvasPointerEvent & event) {
            const double delta=event.angleY ? event.angleY : event.pixelY;
            if(!delta || width<320 || height<360 || event.x<62 || event.x>width-14) return;
            const auto visible=bounds(); const int size=visible.second-visible.first;
            const int next=std::max(std::min(20,count),std::min(count,int(std::round(size*(delta>0 ? .8 : 1.25)))));
            if(next==size) return;
            Viewport view; view.generation=generation;
            view.start=std::max(0,std::min(count-next,int(std::round(visible.first+(event.x-62)/(width-76)*(size-next)))));
            view.stop=view.start+next; viewport=view;
        }
    };
    void heavycanvas(Model & model) {
        if(!model.heavyChart) model.heavyChart=std::make_shared<HeavyChart>();
        auto chart=model.heavyChart;
        const int count=std::stoi(static_cast<std::string>(chart->samples)), seed=chart->seed;
        if(count!=chart->count || seed!=chart->cachedSeed) {
            chart->data=generate(count,seed); chart->count=count; chart->cachedSeed=seed;
            // Returning to an earlier preset must not revive its old zoom.
            ++chart->generation;
        }
        auto data=chart->data; const auto bounds=chart->bounds(); const int start=bounds.first,stop=bounds.second,size=stop-start;
        const auto cursor=static_cast<Cursor>(chart->cursor);
        const bool continuous=chart->continuous, fullFrameChanges=chart->fullFrameChanges;
        for(auto column:CPPUI::VBox().layout({}, {}, 1)) {
            for(auto row:CPPUI::HBox()) {
                CPPUI::Label("Sessions");
                for(auto combo:CPPUI::ComboBox(false,chart->samples).id("heavy-samples"))
                    for(int n:HeavyCanvasData::sampleCounts()) CPPUI::ComboBoxItem(std::to_string(n));
                CPPUI::Button("Regenerate").click([chart](){chart->seed+=1;});
                CPPUI::Button("Reset zoom").click([chart](){chart->viewport=Viewport();});
                CPPUI::Checkbox("Run continuously (FPS)",chart->continuous);
                CPPUI::Checkbox("Full-frame changes",chart->fullFrameChanges);
            }
            CPPUI::Label(std::to_string(count)+" sessions | Seed "+std::to_string(seed)+" | ~"+std::to_string(9*size)+" draw calls");
            CPPUI::Label("Visible: "+std::to_string(size)+" / "+std::to_string(count)+" sessions | Zoom "+number(double(count)/size)+"x");
            CPPUI::Label("Synthetic data / Hover to inspect / Wheel up to zoom in, down to zoom out");
            CPPUI::Label(data->rows[start].label+" - "+data->rows[stop-1].label+" | Last "+number(data->rows[stop-1].close));
            for(auto scroll:CPPUI::Scroll().layout({}, {}, 1)) {
                CPPUI::Canvas([chart,data,start,stop,size,cursor,continuous,fullFrameChanges](CPPUI::CanvasPainter & canvas) {
                    const int tint=fullFrameChanges ? (++chart->paintSequence % 16) : 0;
                    chart->width=canvas.width; chart->height=canvas.height;
                    auto label=[&](double x,double y,const std::string & value,int color=textColor,int font=9){canvas.drawText(x,y,value,-1,-1,font,color ^ tint);};
                    if(canvas.width<320 || canvas.height<360) { label(10,10,"Enlarge the chart to view data."); return false; }
                    const double left=62,right=canvas.width-14,step=(right-left)/size,unit=(canvas.height-192)/6.0;
                    auto x=[&](int i){return left+(i-start+.5)*step;};
                    double low=data->rows[start].lower,high=data->rows[start].upper,volume=0;
                    for(int i=start;i<stop;++i) { const auto & r=data->rows[i]; low=std::min(low,std::min(r.low,r.lower)); high=std::max(high,std::max(r.high,r.upper)); volume=std::max(volume,r.volume); }
                    const double padding=(high-low)*.06; low-=padding; high+=padding;
                    struct Panel { double top,bottom,low,high; }; Panel panels[4];
                    const char * titles[]={"Price / Bollinger 20","Volume","K / D (9, 3) - gold / blue","RSI 14"};
                    double top=24;
                    for(int p=0;p<4;++p) {
                        panels[p]={top,top+unit*(p==0?3:1),p==0?low:0,p==0?high:(p==1?volume*1.05:100)};
                        const auto & panel=panels[p]; label(left,top-21,titles[p],textColor,10);
                        for(double f:{0.,.5,1.}) {
                            double y=panel.bottom-f*(panel.bottom-panel.top),value=panel.low+f*(panel.high-panel.low);
                            canvas.drawLine(left,y,right,y,gridColor ^ tint); label(3,y-8,number(p==1?value/1e6:value,1)+(p==1?"M":""),muted);
                        }
                        std::vector<int> ticks{start,stop-1}; if(canvas.width>=430) ticks.insert(ticks.begin()+1,start+size/2);
                        for(int i:ticks) label(std::max(left,std::min(x(i)-34,right-72)),panel.bottom+7,data->rows[i].label,muted);
                        top=panel.bottom+44;
                    }
                    auto y=[&](double value,int p){auto panel=panels[p];return panel.bottom-(value-panel.low)/(panel.high-panel.low)*(panel.bottom-panel.top);};
                    double body=std::max(1,std::min(5,int(step*.65)));
                    for(int i=start;i<stop;++i) {
                        const auto & r=data->rows[i]; int color=(r.close>=r.open?up:down) ^ tint; double cx=x(i),a=y(r.open,0),b=y(r.close,0);
                        canvas.drawLine(cx,y(r.low,0),cx,y(r.high,0),color);
                        canvas.drawRect(cx-body/2,std::min(a,b),cx+body/2,std::max(a,b)+1,color);
                        canvas.drawRect(cx-body/2,y(r.volume,1),cx+body/2,panels[1].bottom,color);
                    }
                    const double Row::* indicators[]={&Row::upper,&Row::mid,&Row::lower,&Row::k,&Row::d,&Row::rsi};
                    const int positions[]={0,0,0,2,2,3},colors[]={blue,gold,purple,gold,blue,gold};
                    for(int n=0;n<6;++n) for(int i=start+1;i<stop;++i)
                        canvas.drawLine(x(i-1),y(data->rows[i-1].*indicators[n],positions[n]),x(i),y(data->rows[i].*indicators[n],positions[n]),colors[n] ^ tint,2);
                    if(cursor.visible && cursor.x>=left && cursor.x<=right && cursor.y>=panels[0].top && cursor.y<=panels[3].bottom) {
                        int index=std::min(stop-1,start+int((cursor.x-left)/step)); const auto & r=data->rows[index]; double cx=x(index);
                        canvas.drawLine(cx,panels[0].top,cx,panels[3].bottom,muted ^ tint);
                        for(const auto & p:panels) if(cursor.y>=p.top && cursor.y<=p.bottom) canvas.drawLine(left,cursor.y,right,cursor.y,muted ^ tint);
                        const double boxWidth=std::min(330,canvas.width-20),bx=cx+16+boxWidth>right?std::max(10.,cx-boxWidth-16):cx+16,by=panels[0].top+6;
                        canvas.drawRect(bx,by,bx+boxWidth,by+144,0x25374E ^ tint);
                        std::vector<std::string> lines{r.label+" | Synthetic OHLCV","Open "+number(r.open)+"   High "+number(r.high),
                            "Low "+number(r.low)+"   Close "+number(r.close),"Volume "+number(r.volume,0),"Band mid "+number(r.mid),
                            "Upper "+number(r.upper)+"   Lower "+number(r.lower),"K / D "+number(r.k)+" / "+number(r.d)+"   RSI "+number(r.rsi)};
                        for(std::size_t i=0;i<lines.size();++i) label(bx+10,by+8+i*18,lines[i]);
                    }
                    return continuous;
                }).showFps().mousemove([chart](const CPPUI::CanvasPointerEvent & event){Cursor cursor;cursor.visible=true;cursor.x=event.x;cursor.y=event.y;chart->cursor=cursor;})
                  .mouseleave([chart](){chart->cursor=Cursor();}).wheel([chart](const CPPUI::CanvasPointerEvent & event){chart->zoom(event);})
                  .layout({},720,1).style({},background).id("heavy-canvas");
            }
        }
    }
}

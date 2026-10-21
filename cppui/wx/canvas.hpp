#ifndef CPPUI_WX_CANVAS
#define CPPUI_WX_CANVAS
#include "base.hpp"
#include "../anchor.hpp"
#include "../canvas_input.hpp"
#include "../frame_rate.hpp"
#include "image_resource.hpp"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <cmath>
#include <cstdint>
#include <type_traits>
#include <map>
#include <stdexcept>
#include <vector>
namespace CPPUI {
class CanvasColor {
    wxColour color;
    bool present=false;
    friend class CanvasPainter;
public:
    CanvasColor()=default;
    CanvasColor(int rgb):CanvasColor(static_cast<std::uint32_t>(rgb)){}
    CanvasColor(std::uint32_t rgb):color((rgb>>16)&255,(rgb>>8)&255,rgb&255),present(true){}
    CanvasColor(const char * name):CanvasColor(std::string(name)){}
    CanvasColor(const std::string & name):color(wxString::FromUTF8(name)),present(color.IsOk()){}
};
struct CanvasPoint {double x,y;};
class CanvasPainter {
    wxGraphicsContext & painter;
    wxFont font;
    wxColour foreground;
    struct SavedState {
        wxGraphicsContext & painter;
        explicit SavedState(wxGraphicsContext & p):painter(p){p.PushState();}
        ~SavedState(){painter.PopState();}
    };
    SavedState frame;
    // wxGraphicsContext has no pen/brush getters, and its native state stack
    // does not restore these wrapper objects. Track the CPPUI styles explicitly.
    using Styles = std::pair<const wxGraphicsPen *, const wxGraphicsBrush *>;
    Styles styles{&wxNullGraphicsPen,&wxNullGraphicsBrush};
    std::vector<Styles> scopes;
    std::map<std::pair<unsigned long,int>,wxGraphicsPen> pens;
    std::map<unsigned long,wxGraphicsBrush> brushes;
    std::map<std::pair<int,unsigned long>,wxGraphicsFont> fonts;
    void pen(CanvasColor color,int width){
        if(!color.present){painter.SetPen(wxNullGraphicsPen);styles.first=&wxNullGraphicsPen;return;}
        auto key=std::make_pair(color.color.GetRGBA(),std::max(1,width));
        auto found=pens.find(key);
        if(found==pens.end())found=pens.emplace(key,painter.CreatePen(wxPen(color.color,key.second))).first;
        painter.SetPen(found->second);styles.first=&found->second;
    }
    void brush(CanvasColor color){
        if(!color.present){painter.SetBrush(wxNullGraphicsBrush);styles.second=&wxNullGraphicsBrush;return;}
        auto key=color.color.GetRGBA();auto found=brushes.find(key);
        if(found==brushes.end())found=brushes.emplace(key,painter.CreateBrush(wxBrush(color.color))).first;
        painter.SetBrush(found->second);styles.second=&found->second;
    }
    void textFont(int size,const wxColour & color){
        auto key=std::make_pair(size,color.GetRGBA());auto found=fonts.find(key);
        if(found==fonts.end()){
            auto value=font;if(size>0)value.SetPointSize(size);
            found=fonts.emplace(key,painter.CreateFont(value,color)).first;
        }
        painter.SetFont(found->second);
    }
public:
    const int width,height;
    const double pixel_density;
    CanvasPainter(wxGraphicsContext & painter,int width,int height,double density,wxFont font=*wxNORMAL_FONT,wxColour foreground=*wxBLACK)
        :painter(painter),font(font),foreground(foreground),frame(painter),width(width),height(height),pixel_density(density){}
    void push(){scopes.push_back(styles);}
    void pop(){
        if(scopes.empty())throw std::logic_error("Canvas.pop() has no matching push()");
        styles=scopes.back();scopes.pop_back();
        painter.SetPen(*styles.first);painter.SetBrush(*styles.second);
    }
    CanvasPainter(const CanvasPainter &)=delete;
    CanvasPainter & operator=(const CanvasPainter &)=delete;
    void drawText(double x,double y,const std::string & text,int w=-1,int h=-1,int size=12,CanvasColor color={},double rotate=0,Anchor anchor=Anchor::LeftTop){
        (void)w;(void)h;SavedState saved(painter);
        auto lines=wxSplit(wxString::FromUTF8(text),'\n');double tw=0,th=0,lineHeight=0;
        textFont(-1,foreground);
        for(auto & line:lines){double lw,lh;painter.GetTextExtent(line.empty()?wxString(" "):line,&lw,&lh);tw=std::max(tw,lw);lineHeight=std::max(lineHeight,lh);}
        th=lineHeight*lines.size();double dx=tw*(int(anchor)/3)/2,dy=th*(int(anchor)%3)/2;
        painter.Translate(x+dx,y+dy);painter.Rotate(rotate*std::acos(-1.0)/180);painter.Translate(-dx,-dy);
        textFont(size,color.present?color.color:foreground);
        double yy=0;for(auto & line:lines){painter.DrawText(line,0,yy);yy+=lineHeight;}
    }
    void drawLine(double x1,double y1,double x2,double y2,CanvasColor color={},int width=1){pen(color,width);painter.StrokeLine(x1,y1,x2,y2);}
    void drawPolyline(const std::vector<CanvasPoint> & points,CanvasColor color={},int width=1){
        if(points.empty())return;pen(color,width);auto path=painter.CreatePath();path.MoveToPoint(points[0].x,points[0].y);for(std::size_t i=1;i<points.size();++i)path.AddLineToPoint(points[i].x,points[i].y);painter.StrokePath(path);
    }
    void drawCubicBezier(CanvasPoint start,CanvasPoint c1,CanvasPoint c2,CanvasPoint end,CanvasColor color={},int width=1){pen(color,width);auto path=painter.CreatePath();path.MoveToPoint(start.x,start.y);path.AddCurveToPoint(c1.x,c1.y,c2.x,c2.y,end.x,end.y);painter.StrokePath(path);}
    void drawPolygon(const std::vector<CanvasPoint> & points,CanvasColor fill={},CanvasColor stroke={},int width=1){
        if(points.empty())return;pen(stroke,width);brush(fill);auto path=painter.CreatePath();path.MoveToPoint(points[0].x,points[0].y);for(std::size_t i=1;i<points.size();++i)path.AddLineToPoint(points[i].x,points[i].y);path.CloseSubpath();painter.DrawPath(path);
    }
    void drawRect(double x1,double y1,double x2,double y2,CanvasColor fill={},CanvasColor stroke={},int width=1){pen(stroke,width);brush(fill);painter.DrawRectangle(std::min(x1,x2),std::min(y1,y2),std::abs(x2-x1),std::abs(y2-y1));}
    void drawEllipse(double x,double y,double rx,double ry,CanvasColor fill={},CanvasColor stroke={},int width=1){pen(stroke,width);brush(fill);painter.DrawEllipse(x-rx,y-ry,rx*2,ry*2);}
    ImageResource loadImage(const std::string & path) const {return ImageResource::load(path);}
    void drawImage(const ImageResource & image,int x=0,int y=0,int width=-1,int height=-1,int sx=0,int sy=0,int sw=-1,int sh=-1,double opacity=1){
        if(image.isNull())return;
        if(sw<0)sw=image.width()-sx;if(sh<0)sh=image.height()-sy;if(width<0)width=sw;if(height<0)height=sh;
        auto cropped=image.crop(sx,sy,sw,sh);if(cropped.isNull()||width<=0||height<=0)return;
        auto bitmap=cropped.image.Copy();if(opacity<1){if(!bitmap.HasAlpha())bitmap.InitAlpha();auto alpha=bitmap.GetAlpha();for(int i=0;i<sw*sh;++i)alpha[i]=static_cast<unsigned char>(alpha[i]*std::max(0.0,opacity));}
        SavedState saved(painter);painter.DrawBitmap(wxBitmap(bitmap),x,y,width,height);
    }
};
namespace Wx {
class CanvasWidget:public wxPanel {
public:
    using Painter=std::function<bool(CanvasPainter &)>;
    std::function<void(const CanvasPointerEvent &)> moved,wheeled;
    std::function<void()> left;
private:
    Painter painter;
    bool showFPS=false;
    FrameRate fps;
    wxStaticText * fpsLabel=nullptr;
    std::uint64_t generation=0;
    void paint(wxPaintEvent &){
        wxAutoBufferedPaintDC dc(this);dc.SetBackground(wxBrush(GetBackgroundColour()));dc.Clear();
        auto invoke=painter;if(!invoke)return;auto version=generation;bool again=false;
        {std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));if(!gc)return;
        CanvasPainter canvas(*gc,GetClientSize().x,GetClientSize().y,std::max(1.0,GetContentScaleFactor()),GetFont(),GetForegroundColour());
        try{again=invoke(canvas);}catch(...){reportException("paint");return;}}
        if(showFPS&&painter&&version==generation){
            if(!fpsLabel){fpsLabel=new wxStaticText(this,wxID_ANY,"FPS: ...");fpsLabel->SetName("cppui-fps");
                fpsLabel->SetBackgroundColour(wxColour("#101a2b"));fpsLabel->SetForegroundColour(*wxWHITE);}
            if(auto text=fps.tick())fpsLabel->SetLabel(wxString::FromUTF8(*text));
            fpsLabel->SetPosition(wxPoint(std::max(0,GetClientSize().x-fpsLabel->GetBestSize().x-8),8));
        }
        if(again&&painter&&version==generation)Refresh(false);
    }
public:
    explicit CanvasWidget(wxWindow * parent):wxPanel(parent){SetBackgroundStyle(wxBG_STYLE_PAINT);Bind(wxEVT_PAINT,&CanvasWidget::paint,this);
        Bind(wxEVT_MOTION,[this](wxMouseEvent & event){
            auto invoke=moved;if(!invoke){event.Skip();return;}
            CanvasPointerEvent value;value.x=event.GetX();value.y=event.GetY();
            try{invoke(value);}catch(...){reportException("Canvas move");}
        });
        Bind(wxEVT_LEAVE_WINDOW,[this](wxMouseEvent & event){
            auto invoke=left;event.Skip();if(invoke)try{invoke();}catch(...){reportException("Canvas leave");}
        });
        Bind(wxEVT_MOUSEWHEEL,[this](wxMouseEvent & event){
            auto invoke=wheeled;if(!invoke){event.Skip();return;}
            CanvasPointerEvent value;value.x=event.GetX();value.y=event.GetY();
            if(event.GetWheelAxis()==wxMOUSE_WHEEL_HORIZONTAL)value.angleX=event.GetWheelRotation();
            else value.angleY=event.GetWheelRotation();
            try{invoke(value);}catch(...){reportException("Canvas wheel");}
        });
    }
    void setShowFPS(bool value){if(showFPS==value)return;showFPS=value;fps.reset();if(!value&&fpsLabel){fpsLabel->Destroy();fpsLabel=nullptr;}}
    void setPainter(Painter value){painter=std::move(value);++generation;Refresh(false);}
    void clearPainter(){painter={};moved={};wheeled={};left={};++generation;}
};
class Canvas:public Widget {
public:
    using Painter=CanvasWidget::Painter;
    bool showFPS=false;
    Canvas & showFps(bool value=true){showFPS=value;return *this;}
    Painter painter;
    std::function<void(const CanvasPointerEvent &)> moved,wheeled;
    std::function<void()> left;
    Canvas & mousemove(std::function<void(const CanvasPointerEvent &)> callback){moved=std::move(callback);return *this;}
    Canvas & mouseleave(std::function<void()> callback){left=std::move(callback);return *this;}
    Canvas & wheel(std::function<void(const CanvasPointerEvent &)> callback){wheeled=std::move(callback);return *this;}
    explicit Canvas(Painter painter):painter(std::move(painter)){expandX=expandY=3;}
    void updateWidget(Widget * prev) override {ui=prev?prev->ui:new CanvasWidget(parentWindow());auto canvas=static_cast<CanvasWidget *>(ui);canvas->moved=moved;canvas->wheeled=wheeled;canvas->left=left;canvas->setShowFPS(showFPS);canvas->setPainter(painter);}
    void destroy(bool direct) override {if(ui)static_cast<CanvasWidget *>(ui)->clearPainter();Widget::destroy(direct);}
};
namespace detail {
    template<class Callback> Canvas::Painter canvasCallback(Callback callback,std::true_type){return [callback](CanvasPainter & painter)mutable{callback(painter);return false;};}
    template<class Callback> Canvas::Painter canvasCallback(Callback callback,std::false_type){return [callback](CanvasPainter & painter)mutable{return bool(callback(painter));};}
}
}
template<class Callback> Wx::Canvas & Canvas(Callback callback){using Result=typename std::result_of<Callback &(CanvasPainter &)>::type;return *new Wx::Canvas(Wx::detail::canvasCallback(std::move(callback),std::is_void<Result>()));}
}
#endif

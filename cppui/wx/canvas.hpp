#ifndef CPPUI_WX_CANVAS
#define CPPUI_WX_CANVAS
#include "base.hpp"
#include "image_resource.hpp"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <cmath>
#include <cstdint>
#include <type_traits>
namespace CPPUI {
enum class Anchor { LEFT_TOP,LEFT_CENTER,LEFT_BOTTOM,CENTER_TOP,CENTER,CENTER_BOTTOM,RIGHT_TOP,RIGHT_CENTER,RIGHT_BOTTOM };
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
    void pen(CanvasColor color,int width){painter.SetPen(color.present?wxPen(color.color,std::max(1,width)):*wxTRANSPARENT_PEN);}
    void brush(CanvasColor color){painter.SetBrush(color.present?wxBrush(color.color):*wxTRANSPARENT_BRUSH);}
public:
    const int width,height;
    const double pixel_density;
    CanvasPainter(wxGraphicsContext & painter,int width,int height,double density,wxFont font=*wxNORMAL_FONT,wxColour foreground=*wxBLACK)
        :painter(painter),font(font),foreground(foreground),width(width),height(height),pixel_density(density){}
    CanvasPainter(const CanvasPainter &)=delete;
    CanvasPainter & operator=(const CanvasPainter &)=delete;
    void drawText(double x,double y,const std::string & text,int w=-1,int h=-1,int size=12,CanvasColor color={},double rotate=0,Anchor anchor=Anchor::LEFT_TOP){
        (void)w;(void)h;SavedState saved(painter);
        auto lines=wxSplit(wxString::FromUTF8(text),'\n');double tw=0,th=0,lineHeight=0;
        painter.SetFont(font,foreground);
        for(auto & line:lines){double lw,lh;painter.GetTextExtent(line.empty()?wxString(" "):line,&lw,&lh);tw=std::max(tw,lw);lineHeight=std::max(lineHeight,lh);}
        th=lineHeight*lines.size();double dx=tw*(int(anchor)/3)/2,dy=th*(int(anchor)%3)/2;
        painter.Translate(x+dx,y+dy);painter.Rotate(rotate*std::acos(-1.0)/180);painter.Translate(-dx,-dy);
        auto drawFont=font;if(size>0)drawFont.SetPointSize(size);painter.SetFont(drawFont,color.present?color.color:foreground);
        double yy=0;for(auto & line:lines){painter.DrawText(line,0,yy);yy+=lineHeight;}
    }
    void drawLine(double x1,double y1,double x2,double y2,CanvasColor color={},int width=1){SavedState saved(painter);pen(color,width);painter.StrokeLine(x1,y1,x2,y2);}
    void drawPolyline(const std::vector<CanvasPoint> & points,CanvasColor color={},int width=1){
        if(points.empty())return;SavedState saved(painter);pen(color,width);auto path=painter.CreatePath();path.MoveToPoint(points[0].x,points[0].y);for(std::size_t i=1;i<points.size();++i)path.AddLineToPoint(points[i].x,points[i].y);painter.StrokePath(path);
    }
    void drawCubicBezier(CanvasPoint start,CanvasPoint c1,CanvasPoint c2,CanvasPoint end,CanvasColor color={},int width=1){SavedState saved(painter);pen(color,width);auto path=painter.CreatePath();path.MoveToPoint(start.x,start.y);path.AddCurveToPoint(c1.x,c1.y,c2.x,c2.y,end.x,end.y);painter.StrokePath(path);}
    void drawPolygon(const std::vector<CanvasPoint> & points,CanvasColor fill={},CanvasColor stroke={},int width=1){
        if(points.empty())return;SavedState saved(painter);pen(stroke,width);brush(fill);auto path=painter.CreatePath();path.MoveToPoint(points[0].x,points[0].y);for(std::size_t i=1;i<points.size();++i)path.AddLineToPoint(points[i].x,points[i].y);path.CloseSubpath();painter.DrawPath(path);
    }
    void drawRect(double x1,double y1,double x2,double y2,CanvasColor fill={},CanvasColor stroke={},int width=1){SavedState saved(painter);pen(stroke,width);brush(fill);painter.DrawRectangle(std::min(x1,x2),std::min(y1,y2),std::abs(x2-x1),std::abs(y2-y1));}
    void drawEllipse(double x,double y,double rx,double ry,CanvasColor fill={},CanvasColor stroke={},int width=1){SavedState saved(painter);pen(stroke,width);brush(fill);painter.DrawEllipse(x-rx,y-ry,rx*2,ry*2);}
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
private:
    Painter painter;
    std::uint64_t generation=0;
    void paint(wxPaintEvent &){
        wxAutoBufferedPaintDC dc(this);dc.SetBackground(wxBrush(GetBackgroundColour()));dc.Clear();
        auto invoke=painter;if(!invoke)return;auto version=generation;bool again=false;
        {std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));if(!gc)return;
        CanvasPainter canvas(*gc,GetClientSize().x,GetClientSize().y,std::max(1.0,GetContentScaleFactor()),GetFont(),GetForegroundColour());
        try{again=invoke(canvas);}catch(...){reportException("paint");}}
        if(again&&painter&&version==generation)Refresh(false);
    }
public:
    explicit CanvasWidget(wxWindow * parent):wxPanel(parent){SetBackgroundStyle(wxBG_STYLE_PAINT);Bind(wxEVT_PAINT,&CanvasWidget::paint,this);}
    void setPainter(Painter value){painter=std::move(value);++generation;Refresh(false);}
    void clearPainter(){painter={};++generation;}
};
class Canvas:public Widget {
public:
    using Painter=CanvasWidget::Painter;
    Painter painter;
    explicit Canvas(Painter painter):painter(std::move(painter)){expandX=expandY=3;}
    void updateWidget(Widget * prev) override {ui=prev?prev->ui:new CanvasWidget(parentWindow());static_cast<CanvasWidget *>(ui)->setPainter(painter);}
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

#ifndef CPPUI_WX_IMAGE_RESOURCE
#define CPPUI_WX_IMAGE_RESOURCE
#include <wx/image.h>
#include <wx/log.h>
#include <algorithm>
#include <string>
namespace CPPUI {
class CanvasPainter;
class ImageResource {
    wxImage image;
    friend class CanvasPainter;
public:
    ImageResource() = default;
    explicit ImageResource(wxImage value): image(std::move(value)) {}
    static ImageResource load(const std::string & path) {
        // Handlers belong to the wx runtime; it may be torn down and restarted.
        if(!wxImage::FindHandler(wxBITMAP_TYPE_PNG)) wxInitAllImageHandlers();
        wxLogNull quiet;
        return ImageResource(wxImage(wxString::FromUTF8(path)));
    }
    ImageResource crop(int x,int y,int w,int h) const {
        if(isNull() || x<0 || y<0 || w<=0 || h<=0 || x>width()-w || y>height()-h) return {};
        return ImageResource(image.GetSubImage(wxRect(x,y,w,h)));
    }
    ImageResource scale(int w,int h,bool keepAspectRatio=true,int quality=0) const {
        if(isNull() || w<=0 || h<=0) return {};
        if(keepAspectRatio) { const double ratio=std::min(double(w)/width(),double(h)/height());w=std::max(1,int(width()*ratio));h=std::max(1,int(height()*ratio)); }
        return ImageResource(image.Scale(w,h,quality==0?wxIMAGE_QUALITY_NORMAL:wxIMAGE_QUALITY_HIGH));
    }
    bool isNull() const { return !image.IsOk(); }
    int width() const { return isNull()?0:image.GetWidth(); }
    int height() const { return isNull()?0:image.GetHeight(); }
    const wxImage & nativeImage() const { return image; }
};
}
#endif

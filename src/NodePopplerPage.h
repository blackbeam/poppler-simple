#include <unistd.h>
#include <v8.h>
#include <node.h>
#include <nan.h>
#include <poppler/poppler-config.h>
#include <cpp/poppler-version.h>
#include <poppler/Page.h>
#include <poppler/PDFDoc.h>
#include <poppler/TextOutputDev.h>
#include <poppler/Gfx.h>
#include <poppler/SplashOutputDev.h>
#include <splash/SplashBitmap.h>
#include <splash/SplashErrorCodes.h>
#include <goo/ImgWriter.h>
#include <goo/PNGWriter.h>
#include <goo/TiffWriter.h>
#include <goo/JpegWriter.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <tuple>

#include "iconv_string.h"
#include "MemoryStream.h"

namespace node
{
class NodePopplerDocument;
class NodePopplerPage : public Nan::ObjectWrap
{
  public:
    enum Writer
    {
        W_PNG,
        W_JPEG,
        W_TIFF /*, W_PIXBUF*/
    };
    enum Destination
    {
        DEST_BUFFER,
        DEST_FILE
    };

    class RenderWork
    {
      public:
        RenderWork(NodePopplerPage *self, NodePopplerPage::Destination dest)
            : callback(NULL), progressive(false), error(NULL), mstrm_buf(NULL), filename(NULL), compression(NULL), quality(100), slice_x(0), slice_y(0), slice_w(1), slice_h(1), PPI(72), f(NULL), stream(NULL), mstrm_len(0), w(W_JPEG)
        {
            this->self = self;
            this->dest = dest;
            request.data = this;
            format[0] = '\0';
        }
        ~RenderWork()
        {
            if (error)
                delete[] error;
            if (mstrm_buf)
                free(mstrm_buf);
            if (filename)
                delete[] filename;
            if (compression)
                delete[] compression;
            if (callback != NULL)
                delete callback;
            if (f)
                fclose(f);
            if (stream)
                delete stream;
        }
        void setWriter(const v8::Local<v8::Value> method);
        void setWriterOptions(const v8::Local<v8::Value> optsVal);
        void setPPI(const v8::Local<v8::Value> PPI);
        void setPath(const v8::Local<v8::Value> path);
        void setSlice(const v8::Local<v8::Value> sliceVal);
        void openStream();
        void closeStream();
        std::tuple<int, int, int, int> applyScale();

        uv_work_t request;
        Nan::Callback *callback;
        bool progressive;
        char *error;
        char *mstrm_buf;
        char *filename;
        char *compression;
        char format[5];
        int quality;
        double slice_x;
        double slice_y;
        double slice_w;
        double slice_h;
        double PPI;
        FILE *f;
        MemoryStream *stream;
        size_t mstrm_len;
        NodePopplerPage::Writer w;
        NodePopplerPage::Destination dest;
        NodePopplerPage *self;
    };

    NodePopplerPage(NodePopplerDocument *doc, const int32_t pageNum);
    ~NodePopplerPage();

    static NAN_MODULE_INIT(Init);

    bool isOk()
    {
        return pg != NULL && pg->isOk();
    }

    void wrap(v8::Local<v8::Object> o)
    {
        this->Wrap(o);
    }

    double getWidth()
    {
        return ((getRotate() == 90 || getRotate() == 270)
                    ? pg->getCropHeight()
                    : pg->getCropWidth());
    }
    double getHeight()
    {
        return ((getRotate() == 90 || getRotate() == 270)
                    ? pg->getCropWidth()
                    : pg->getCropHeight());
    }
    double getRotate() { return pg->getRotate(); }
    bool isDocClosed() { return docClosed; }

    static void display(RenderWork *work);

  protected:
    static NAN_METHOD(New);
    static NAN_METHOD(findText);
    static NAN_METHOD(getWordList);
    static NAN_METHOD(renderToFile);
    static NAN_METHOD(renderToBuffer);
    static NAN_METHOD(addAnnot);
    static NAN_METHOD(deleteAnnots);

    static void AsyncRenderWork(uv_work_t *req);
    static void AsyncRenderAfter(uv_work_t *req, int status);
    void parseAnnot(const v8::Local<v8::Value> rect,
                    double *x1, double *y1,
                    double *x2, double *y2,
                    double *x3, double *y3,
                    double *x4, double *y4,
                    char **error);

    void evDocumentClosed();

    bool docClosed;

  private:
    static NAN_GETTER(paramsGetter);

    TextPage *getTextPage(bool rawOrder)
    {
        if (text == NULL)
        {
            TextOutputDev *textDev;
            Gfx *gfx;
            textDev = new TextOutputDev(NULL, true, 0, rawOrder, false);
            gfx = pg->createGfx(textDev, 72., 72., 0,
                                false,
                                true,
                                -1, -1, -1, -1,
                                false, NULL, NULL);
            pg->display(gfx);
            textDev->endPage();
            text = textDev->takeText();
            delete gfx;
            delete textDev;
        }
        return text;
    }
    void renderToStream(RenderWork *work);
    void addAnnot(const v8::Local<v8::Array> array, char **error);

    // we doesn't own this
    PDFDoc *doc;

    Page *pg;
    TextPage *text;
    double color_r;
    double color_g;
    double color_b;
    NodePopplerDocument *parent;

    friend class NodePopplerDocument;
};
} // namespace node

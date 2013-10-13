#include <unistd.h>
#include <v8.h>
#include <node.h>
#include <node_object_wrap.h>
#include "helpers.h"
#include <poppler/poppler-config.h>
#include <cpp/poppler-version.h>
#include <poppler/Page.h>
#include <poppler/PDFDoc.h>
#include <poppler/TextOutputDev.h>
#include <poppler/Gfx.h>
#include <poppler/SplashOutputDev.h>
#include <splash/SplashBitmap.h>
#include <splash/SplashErrorCodes.h>
#include <goo/gtypes.h>
#include <goo/ImgWriter.h>
#include <goo/PNGWriter.h>
#include <goo/TiffWriter.h>
#include <goo/JpegWriter.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "iconv_string.h"


namespace node {
    class NodePopplerDocument;
    class NodePopplerPage : public ObjectWrap {
    public:
        enum Writer { W_PNG, W_JPEG, W_TIFF/*, W_PIXBUF*/ };
        enum Destination { DEST_BUFFER, DEST_FILE };

        class RenderWork
        {
        public:
            RenderWork(NodePopplerPage *self, NodePopplerPage::Destination dest)
                : progressive(false)
                , error(NULL)
                , mstrm_buf(NULL)
                , filename(NULL)
                , compression(NULL)
                , quality(100)
                , sx(0)
                , sy(0)
                , sw(1)
                , sh(1)
                , PPI(72)
                , f(NULL)
                , mstrm_len(0)
                , w(W_JPEG)
            {
                this->self = self;
                this->dest = dest;
                request.data = this;
                format[0] = '\0';
            }
            ~RenderWork() {
                if (error) delete [] error;
                if (mstrm_buf) free(mstrm_buf);
                if (filename) delete [] filename;
                if (compression) delete [] compression;
                if (!callback.IsEmpty()) DISPOSE_PERSISTENT(callback);
                if (f) fclose(f);
            }
            void setWriter(const v8::Handle<v8::Value> method);
            void setWriterOptions(const v8::Handle<v8::Value> optsVal);
            void setPPI(const v8::Handle<v8::Value> PPI);
            void setPath(const v8::Handle<v8::Value> path);
            void setSlice(const v8::Handle<v8::Value> sliceVal);
            void openStream();
            void closeStream();

            uv_work_t request;
            v8::Persistent<v8::Function> callback;
            bool progressive;
            char *error;
            char *mstrm_buf;
            char *filename;
            char *compression;
            char format[5];
            int quality;
            int sx;
            int sy;
            int sw;
            int sh;
            double PPI;
            FILE *f;
            size_t mstrm_len;
            NodePopplerPage::Writer w;
            NodePopplerPage::Destination dest;
            NodePopplerPage *self;
        };

        NodePopplerPage(NodePopplerDocument* doc, const int32_t pageNum);
        ~NodePopplerPage();

        static void Init(v8::Handle<v8::Object> exports);

        bool isOk() {
            return pg != NULL && pg->isOk();
        }

        void wrap(v8::Handle<v8::Object> o) {
            this->Wrap(o);
        }

        double getWidth() {
            return ((getRotate() == 90 || getRotate() == 270)
                ? pg->getCropHeight()
                : pg->getCropWidth()
                );
        }
        double getHeight() {
            return ((getRotate() == 90 || getRotate() == 270)
                ? pg->getCropWidth()
                : pg->getCropHeight()
                );
        }
        double getRotate() { return pg->getRotate(); }
        bool isDocClosed() { return docClosed; }

        static void display(RenderWork *work);

    protected:
        V8_METHOD_DECL(New);
        V8_METHOD_DECL(findText);
        V8_METHOD_DECL(getWordList);
        V8_METHOD_DECL(renderToFile);
        V8_METHOD_DECL(renderToBuffer);
#if POPPLER_VERSION_MAJOR == 0 && POPPLER_VERSION_MINOR < 20
#else
        V8_METHOD_DECL(addAnnot);
#endif
        V8_METHOD_DECL(deleteAnnots);

        static void AsyncRenderWork(uv_work_t *req);
        static void AsyncRenderAfter(uv_work_t *req, int status);
        void parseAnnot(const v8::Handle<v8::Value> rect,
            double *x1, double *y1,
            double *x2, double *y2,
            double *x3, double *y3,
            double *x4, double *y4,
            char **error);

        void evDocumentClosed();

        bool docClosed;
    private:
        V8_ACCESSOR_GETTER_DECL(paramsGetter);

        TextPage *getTextPage() {
            if (text == NULL) {
                TextOutputDev *textDev;
                Gfx *gfx;
#if (POPPLER_VERSION_MINOR < 19)
                textDev = new TextOutputDev(NULL, gTrue, gFalse, gFalse);
                gfx = pg->createGfx(textDev, 72., 72., 0,
                    gFalse,
                    gTrue,
                    -1, -1, -1, -1,
                    gFalse,
                    doc->getCatalog(),
                    NULL, NULL, NULL, NULL);
#else
                textDev = new TextOutputDev(NULL, gTrue, 0, gFalse, gFalse);
                gfx = pg->createGfx(textDev, 72., 72., 0,
                    gFalse,
                    gTrue,
                    -1, -1, -1, -1,
                    gFalse, NULL, NULL);
#endif
                pg->display(gfx);
                textDev->endPage();
                text = textDev->takeText();
                delete gfx;
                delete textDev;
            }
            return text;
        }
        void renderToStream(RenderWork *work);
#if POPPLER_VERSION_MAJOR == 0 && POPPLER_VERSION_MINOR < 20
#else
        void addAnnot(const v8::Handle<v8::Array> array, char **error);
#endif

        PDFDoc *doc;
        Page *pg;
        TextPage *text;
        AnnotColor *color;
        NodePopplerDocument *parent;

        friend class NodePopplerDocument;
    };
}

#include <unistd.h>
#include <v8.h>
#include <node.h>
#include <node_object_wrap.h>
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
            RenderWork() {
                error = mstrm_buf = filename = compression = NULL;
                f = NULL;
                mstrm_len = 0;
            }
            ~RenderWork() {
                if (error) delete [] error;
                if (mstrm_buf) free(mstrm_buf);
                if (filename) delete [] filename;
                if (compression) delete [] compression;
                if (!callback.IsEmpty()) callback.Dispose();
                if (f) fclose(f);
            }

            uv_work_t request;
            v8::Persistent<v8::Function> callback;
            bool progressive;
            char *error, *mstrm_buf, *filename, *compression;
            int quality, sx, sy, sw, sh;
            double PPI;
            FILE *f;
            size_t mstrm_len;
            NodePopplerPage::Writer w;
            NodePopplerPage *self;
        };

        NodePopplerPage(NodePopplerDocument* doc, int32_t pageNum);
        ~NodePopplerPage();
        static void Initialize(v8::Handle<v8::Object> target);
        bool isOk() {
            return pg != NULL && pg->isOk();
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

        static void display(
            NodePopplerPage *self,
            FILE *f, double PPI, Writer wr,
            char *compression, int quality, bool progressive,
            int x, int y, int w, int h, char **error);

    protected:
        static v8::Handle<v8::Value> New(const v8::Arguments &args);
        static v8::Handle<v8::Value> findText(const v8::Arguments &args);
        static v8::Handle<v8::Value> getWordList(const v8::Arguments &args);
        static v8::Handle<v8::Value> renderToFile(const v8::Arguments &args);
        static v8::Handle<v8::Value> renderToBuffer(const v8::Arguments &args);
        static v8::Handle<v8::Value> addAnnot(const v8::Arguments &args);
        static v8::Handle<v8::Value> deleteAnnots(const v8::Arguments &args);

        static void parseRenderArguments(
            v8::Handle<v8::Value> argv[], int argc,
            Writer *wr, char **compression, int *quality, bool *progressive, double *PPI,
            double *x, double *y, double *w, double *h,
            char **error);
        static void parseWriterOptions(
            v8::Handle<v8::Value> options,
            Writer w,
            char **compression, int *quality, bool *progressive,
            char **error);
        static Writer parseWriter(v8::Handle<v8::Value> method, char **error);
        static double parsePPI(v8::Handle<v8::Value> PPI, char **error);
        static void parseSlice(
            v8::Handle<v8::Value> sliceValue,
            double *x, double *y, double *w, double *h,
            char **error);
        void parseAnnot(v8::Handle<v8::Value> rect,
            double *x1, double *y1,
            double *x2, double *y2,
            double *x3, double *y3,
            double *x4, double *y4,
            char **error);

        void evDocumentClosed();

        bool docClosed;
    private:
        static v8::Persistent<v8::FunctionTemplate> constructor_template;

        static v8::Handle<v8::Value> paramsGetter(v8::Local<v8::String> property, const v8::AccessorInfo &info);

        TextPage *getTextPage() {
            if (text == NULL) {
                TextOutputDev *textDev;
                Gfx *gfx;

                textDev = new TextOutputDev(NULL, gTrue, 0, gFalse, gFalse);
                gfx = pg->createGfx(textDev, 72., 72., 0,
                    gFalse,
                    gTrue,
                    -1, -1, -1, -1,
                    gFalse, NULL, NULL);
                pg->display(gfx);
                textDev->endPage();
                text = textDev->takeText();
                delete gfx;
                delete textDev;
            }
            return text;
        }
        void renderToStream(int argc, v8::Handle<v8::Value> argv[], RenderWork *work);
        void addAnnot(v8::Handle<v8::Array> array, char **error);

        PDFDoc *doc;
        Page *pg;
        TextPage *text;
        AnnotColor *color;
        NodePopplerDocument *parent;

        friend class NodePopplerDocument;
    };
}

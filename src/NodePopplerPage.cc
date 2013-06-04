#include <v8.h>
#include <node.h>
#include <node_buffer.h>

#include "NodePopplerDocument.h"
#include "NodePopplerPage.h"

#define THROW_SYNC_ASYNC_ERR(work, err) \
    if (work->callback.IsEmpty()) { \
        delete work; \
        return ThrowException(err); \
    } else { \
        Local<Value> argv[] = {err}; \
        TryCatch try_catch; \
        work->callback->Call(Context::GetCurrent()->Global(), 1, argv); \
        if (try_catch.HasCaught()) { \
            node::FatalException(try_catch); \
        } \
        delete work; \
        return scope.Close(Undefined()); \
    }

using namespace v8;
using namespace node;

Persistent<FunctionTemplate> NodePopplerPage::constructor_template;
static Persistent<String> width_sym;
static Persistent<String> height_sym;
static Persistent<String> index_sym;

namespace node {

    void NodePopplerPage::Initialize(v8::Handle<v8::Object> target) {
        HandleScope scope;

        width_sym = Persistent<String>::New(String::NewSymbol("width"));
        height_sym = Persistent<String>::New(String::NewSymbol("height"));
        index_sym = Persistent<String>::New(String::NewSymbol("index"));

        Local<FunctionTemplate> t = FunctionTemplate::New(New);
        constructor_template = Persistent<FunctionTemplate>::New(t);
        constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
        constructor_template->SetClassName(String::NewSymbol("PopplerPage"));

        /** Instance methods
         *  static Handle<Value> funcName(const Arguments &args);
         *  NODE_SET_PROTOTYPE_METHOD(constructor_template, "getPageCount", funcName);
         */
        NODE_SET_PROTOTYPE_METHOD(constructor_template, "renderToFile", NodePopplerPage::renderToFile);
        NODE_SET_PROTOTYPE_METHOD(constructor_template, "renderToBuffer", NodePopplerPage::renderToBuffer);
        NODE_SET_PROTOTYPE_METHOD(constructor_template, "findText", NodePopplerPage::findText);
        NODE_SET_PROTOTYPE_METHOD(constructor_template, "getWordList", NodePopplerPage::getWordList);
        NODE_SET_PROTOTYPE_METHOD(constructor_template, "addAnnot", NodePopplerPage::addAnnot);
        NODE_SET_PROTOTYPE_METHOD(constructor_template, "deleteAnnots", NodePopplerPage::deleteAnnots);

        /** Getters:
         *  static Handle<Value> funcName(Local<String> property, const AccessorInfo& info);
         *  constructor_template->PrototypeTemplate()->SetAccessor(String::NewSymbol("page_count"), funcName);
         */
        constructor_template->InstanceTemplate()->SetAccessor(String::NewSymbol("num"), NodePopplerPage::paramsGetter);
        constructor_template->InstanceTemplate()->SetAccessor(String::NewSymbol("width"), NodePopplerPage::paramsGetter);
        constructor_template->InstanceTemplate()->SetAccessor(String::NewSymbol("height"), NodePopplerPage::paramsGetter);
        constructor_template->InstanceTemplate()->SetAccessor(String::NewSymbol("crop_box"), NodePopplerPage::paramsGetter);
        constructor_template->InstanceTemplate()->SetAccessor(String::NewSymbol("numAnnots"), NodePopplerPage::paramsGetter);
        constructor_template->InstanceTemplate()->SetAccessor(String::NewSymbol("media_box"), NodePopplerPage::paramsGetter);
        constructor_template->InstanceTemplate()->SetAccessor(String::NewSymbol("art_box"), NodePopplerPage::paramsGetter);
        constructor_template->InstanceTemplate()->SetAccessor(String::NewSymbol("trim_box"), NodePopplerPage::paramsGetter);
        constructor_template->InstanceTemplate()->SetAccessor(String::NewSymbol("bleed_box"), NodePopplerPage::paramsGetter);
        constructor_template->InstanceTemplate()->SetAccessor(String::NewSymbol("rotate"), NodePopplerPage::paramsGetter);
        constructor_template->InstanceTemplate()->SetAccessor(String::NewSymbol("isCropped"), NodePopplerPage::paramsGetter);

	    /** Class methods
	     * NODE_SET_METHOD(constructor_template->GetFunction(), "GetPageCount", funcName);
	     */
	    // NODE_SET_METHOD(constructor_template->GetFunction(), "pixbufToImage", NodePopplerPage::pixbufToImage);

	    target->Set(String::NewSymbol("PopplerPage"), constructor_template->GetFunction());
    }

    NodePopplerPage::~NodePopplerPage() {
        if (text != NULL) { text->decRefCnt(); }
        if (color != NULL) { delete color; }
        if (!docClosed) { parent->evPageClosed(this); }
    }

    NodePopplerPage::NodePopplerPage(NodePopplerDocument* doc, int32_t pageNum) : ObjectWrap() {
        text = NULL;
        color = NULL;

        pg = doc->doc->getPage(pageNum);
        if (pg && pg->isOk()) {
            parent = doc;
            parent->evPageOpened(this);
            this->doc = doc->doc;
            color = new AnnotColor(0, 1, 0);
            docClosed = false;
        } else {
            docClosed = true;
        }
    }

    void NodePopplerPage::evDocumentClosed() {
        docClosed = true;
    }

    Handle<Value> NodePopplerPage::New(const Arguments &args) {
        HandleScope scope;
        NodePopplerDocument* doc;
        int32_t pageNum;

        if (args.Length() != 2) {
            return ThrowException(Exception::Error(
                String::New("Two arguments required: (doc: NodePopplerDocument, page: Uint32).")));
        }
        if (!args[1]->IsUint32()) {
            return ThrowException(
                Exception::TypeError(String::New("'page' must be an instance of Uint32.")));
        }
        pageNum = args[1]->ToUint32()->Value();

        if(!args[0]->IsObject()) { // TODO: hasInstance
            return ThrowException(Exception::TypeError(
                String::New("'doc' must be an instance of NodePopplerDocument.")));
        }

        doc = ObjectWrap::Unwrap<NodePopplerDocument>(args[0]->ToObject());
        if (0 >= pageNum || pageNum > doc->doc->getNumPages()) {
            return ThrowException(Exception::Error(String::New(
                "Page number out of bounds.")));
        }

        NodePopplerPage* page = new NodePopplerPage(doc, pageNum);
        if(!page->isOk()) {
            delete page;
            return ThrowException(Exception::Error(String::New("Can't open page.")));;
        }
        page->Wrap(args.This());
        return args.This();
    }

    Handle<Value> NodePopplerPage::paramsGetter(Local<String> property, const AccessorInfo &info) {
        HandleScope scope;

        String::Utf8Value propName(property);
        NodePopplerPage *self = ObjectWrap::Unwrap<NodePopplerPage>(info.This());

        if (strcmp(*propName, "width") == 0) {
            return scope.Close(Number::New(self->getWidth()));

        } else if (strcmp(*propName, "height") == 0) {
            return scope.Close(Number::New(self->getHeight()));

        } else if (strcmp(*propName, "num") == 0) {
            return scope.Close(Uint32::New(self->pg->getNum()));

        } else if (strcmp(*propName, "crop_box") == 0) {
            PDFRectangle *rect = self->pg->getCropBox();
            Local<v8::Object> crop_box = v8::Object::New();

            crop_box->Set(String::NewSymbol("x1"), Number::New(rect->x1));
            crop_box->Set(String::NewSymbol("x2"), Number::New(rect->x2));
            crop_box->Set(String::NewSymbol("y1"), Number::New(rect->y1));
            crop_box->Set(String::NewSymbol("y2"), Number::New(rect->y2));

            return scope.Close(crop_box);

        } else if (strcmp(*propName, "media_box") == 0) {
            PDFRectangle *rect = self->pg->getMediaBox();
            Local<v8::Object> media_box = v8::Object::New();

            media_box->Set(String::NewSymbol("x1"), Number::New(rect->x1));
            media_box->Set(String::NewSymbol("x2"), Number::New(rect->x2));
            media_box->Set(String::NewSymbol("y1"), Number::New(rect->y1));
            media_box->Set(String::NewSymbol("y2"), Number::New(rect->y2));

            return scope.Close(media_box);

        } else if (strcmp(*propName, "bleed_box") == 0) {
            PDFRectangle *rect = self->pg->getBleedBox();
            Local<v8::Object> bleed_box = v8::Object::New();

            bleed_box->Set(String::NewSymbol("x1"), Number::New(rect->x1));
            bleed_box->Set(String::NewSymbol("x2"), Number::New(rect->x2));
            bleed_box->Set(String::NewSymbol("y1"), Number::New(rect->y1));
            bleed_box->Set(String::NewSymbol("y2"), Number::New(rect->y2));

            return scope.Close(bleed_box);

        } else if (strcmp(*propName, "trim_box") == 0) {
            PDFRectangle *rect = self->pg->getTrimBox();
            Local<v8::Object> trim_box = v8::Object::New();

            trim_box->Set(String::NewSymbol("x1"), Number::New(rect->x1));
            trim_box->Set(String::NewSymbol("x2"), Number::New(rect->x2));
            trim_box->Set(String::NewSymbol("y1"), Number::New(rect->y1));
            trim_box->Set(String::NewSymbol("y2"), Number::New(rect->y2));

            return scope.Close(trim_box);

        } else if (strcmp(*propName, "art_box") == 0) {
            PDFRectangle *rect = self->pg->getArtBox();
            Local<v8::Object> art_box = v8::Object::New();

            art_box->Set(String::NewSymbol("x1"), Number::New(rect->x1));
            art_box->Set(String::NewSymbol("x2"), Number::New(rect->x2));
            art_box->Set(String::NewSymbol("y1"), Number::New(rect->y1));
            art_box->Set(String::NewSymbol("y2"), Number::New(rect->y2));

            return scope.Close(art_box);

        } else if (strcmp(*propName, "rotate") == 0) {
            return scope.Close(Int32::New(self->pg->getRotate()));

        } else if (strcmp(*propName, "numAnnots") == 0) {
            Annots *annots = self->pg->getAnnots();
            return scope.Close(Uint32::New(annots->getNumAnnots()));

        } else if (strcmp(*propName, "isCropped") == 0) {
            return scope.Close(Boolean::New(self->pg->isCropped()));

        } else {
            return scope.Close(Null());
        }
    }

    /**
     * \return Object Array of Objects which represents individual words on page
     *                and stores word text and relative coords
     */
    Handle<Value> NodePopplerPage::getWordList(const Arguments &args) {
        HandleScope scope;
        NodePopplerPage* self = ObjectWrap::Unwrap<NodePopplerPage>(args.Holder());
        TextPage *text;
        TextWordList *wordList;

        if (self->isDocClosed()) {
            return ThrowException(Exception::Error(String::New(
                "Document closed. You must delete this page")));
        }

        text = self->getTextPage();
        wordList = text->makeWordList(gTrue);
        int l = wordList->getLength();
        Local<v8::Array> v8results = v8::Array::New(l);
        for (int i = 0; i < l; i++) {
            Local<v8::Object> v8result = v8::Object::New();
            TextWord *word = wordList->get(i);
            GooString *str = word->getText();
            double x1, y1, x2, y2;

            word->getBBox(&x1, &y1, &x2, &y2);
            // Make coords relative
            x1 /= self->getWidth();
            x2 /= self->getWidth();
            y1 /= self->getHeight();
            y2 /= self->getHeight();
            // TextOutputDev is upside down device
            y1 = 1 - y1;
            y2 = 1 - y2;
            y1 = y1 + y2;
            y2 = y1 - y2;
            y1 = y1 - y2;

            v8result->Set(String::NewSymbol("x1", 2), Number::New(x1));
            v8result->Set(String::NewSymbol("x2", 2), Number::New(x2));
            v8result->Set(String::NewSymbol("y1", 2), Number::New(y1));
            v8result->Set(String::NewSymbol("y2", 2), Number::New(y2));
            v8result->Set(String::NewSymbol("text", 4), String::New(str->getCString()));

            v8results->Set(i, v8result);

            delete str;
        }
        delete wordList;

        return scope.Close(v8results);
    }

    /**
     * \return Object Relative coors from lower left corner
     */
    Handle<Value> NodePopplerPage::findText(const Arguments &args) {
        HandleScope scope;
        NodePopplerPage* self = ObjectWrap::Unwrap<NodePopplerPage>(args.Holder());
        TextPage *text;
        char *ucs4 = NULL;
        size_t ucs4_len;
        double xMin = 0, yMin = 0, xMax, yMax;
        PDFRectangle **matches = NULL;
        unsigned int cnt = 0;

        if (self->isDocClosed()) {
            return ThrowException(Exception::Error(String::New(
                "Document closed. You must delete this page")));
        }

        if (args.Length() != 1 && !args[0]->IsString()) {
            return ThrowException(Exception::Error(
                String::New("One argument required: (str: String)")));
        }
        String::Utf8Value str(args[0]);

        iconv_string("UCS-4LE", "UTF-8", *str, *str+strlen(*str)+1, &ucs4, &ucs4_len);
        text = self->getTextPage();

        while (text->findText((unsigned int *)ucs4, ucs4_len/4 - 1,
                 gFalse, gTrue, // startAtTop, stopAtBottom
                 gFalse, gFalse, // startAtLast, stopAtLast
                 gFalse, gFalse, // caseSensitive, backwards
                 gFalse, // wholeWord
                 &xMin, &yMin, &xMax, &yMax)) {
            PDFRectangle **t_matches = matches;
            cnt++;
            matches = (PDFRectangle**) realloc(t_matches, sizeof(PDFRectangle*) * cnt);
            matches[cnt-1] = new PDFRectangle(xMin, self->getHeight() - yMax, xMax, self->getHeight() - yMin);
        }
        Local<v8::Array> v8results = v8::Array::New(cnt);
        for (unsigned int i = 0; i < cnt; i++) {
            PDFRectangle *match = matches[i];
            Local<v8::Object> v8result = v8::Object::New();
            v8result->Set(String::NewSymbol("x1"), Number::New(match->x1 / self->getWidth()));
            v8result->Set(String::NewSymbol("x2"), Number::New(match->x2 / self->getWidth()));
            v8result->Set(String::NewSymbol("y1"), Number::New(match->y1 / self->getHeight()));
            v8result->Set(String::NewSymbol("y2"), Number::New(match->y2 / self->getHeight()));
            v8results->Set(i, v8result);
            delete match;
        }
        if (ucs4 != NULL) {
            free(ucs4);
        }
        if (matches != NULL) {
            free(matches);
        }
        return scope.Close(v8results);
    }

    /**
     * Deletes all annotations
     */
    Handle<Value> NodePopplerPage::deleteAnnots(const Arguments &args) {
        HandleScope scope;
        NodePopplerPage *self = ObjectWrap::Unwrap<NodePopplerPage>(args.Holder());

        Annots *annots = self->pg->getAnnots();

        while (annots->getNumAnnots()) {
            Annot *annot = annots->getAnnot(0);
            annot->invalidateAppearance();
            self->pg->removeAnnot(annot);
        }

        return scope.Close(Null());
    }

    /**
     * Adds annotations to a page
     *
     * Javascript function
     *
     * \param annot Object or Array of annot objects with fields:
     *  x1 - for lower left corner relative x ord
     *  y1 - for lower left corner relative y ord
     *  x2 - for upper right corner relative x ord
     *  y2 - for upper right corner relative y ord
     */
    Handle<Value> NodePopplerPage::addAnnot(const Arguments &args) {
        HandleScope scope;
        NodePopplerPage* self = ObjectWrap::Unwrap<NodePopplerPage>(args.Holder());

        if (self->isDocClosed()) {
            return ThrowException(Exception::Error(String::New(
                "Document closed. You must delete this page")));
        }

        char *error = NULL;

        if (args.Length() < 1) {
            return ThrowException(Exception::Error(String::New(
                "One argument required: (annot: Object | Array).")));
        }

        if (args[0]->IsArray()) {
            if (Handle<v8::Array>::Cast(args[0])->Length() > 0) {
                self->addAnnot(Handle<v8::Array>::Cast(args[0]), &error);
            }
        } else if (args[0]->IsObject()) {
            Handle<v8::Array> annot = v8::Array::New(1);
            annot->Set(0, args[0]);
            self->addAnnot(annot, &error);
        }

        if (error) {
            Handle<Value> e = Exception::Error(String::New(error));
            delete [] error;
            return ThrowException(e);
        } else {
            return scope.Close(Null());
        }
    }

    /**
     * Add annotations to page
     */
    void NodePopplerPage::addAnnot(Handle<v8::Array> array, char **error) {
        HandleScope scope;

        double x1, y1, x2, y2, x3, y3, x4, y4;
        int len = array->Length();
        AnnotQuadrilaterals::AnnotQuadrilateral **quads =
            (AnnotQuadrilaterals::AnnotQuadrilateral**) gmalloc(sizeof(AnnotQuadrilaterals::AnnotQuadrilateral*) * len);
        for (int i = 0; i < len; i++) {
            parseAnnot(array->Get(i), &x1, &y1, &x2, &y2, &x3, &y3, &x4, &y4, error);
            if (*error) {
                for (i--; i >= 0; i--) {
                    delete quads[i];
                }
                delete [] quads;
                return;
            }
            quads[i] = new AnnotQuadrilaterals::AnnotQuadrilateral(x1, y1, x2, y2, x3, y3, x4, y4);
        }

        PDFRectangle *rect = new PDFRectangle(0,0,0,0);
        AnnotQuadrilaterals *aq = new AnnotQuadrilaterals(quads, len);
        AnnotTextMarkup *annot = new AnnotTextMarkup(doc, rect, Annot::typeHighlight, aq);

        annot->setOpacity(.5);
        annot->setColor(color);
        pg->addAnnot(annot);

        delete aq;
        delete rect;
    }

    /**
     * Parse annotation quadrilateral
     */
    void NodePopplerPage::parseAnnot(Handle<Value> rect,
            double *x1, double *y1,
            double *x2, double *y2,
            double *x3, double *y3,
            double *x4, double *y4,
            char **error) {
        HandleScope scope;
        Local<String> x1k = String::NewSymbol("x1");
        Local<String> x2k = String::NewSymbol("x2");
        Local<String> y1k = String::NewSymbol("y1");
        Local<String> y2k = String::NewSymbol("y2");
        if (!rect->IsObject() ||
                !rect->ToObject()->Has(x1k) || !rect->ToObject()->Has(x2k) ||
                !rect->ToObject()->Has(y1k) || !rect->ToObject()->Has(y2k)) {
            char *e = (char*)"Invalid rectangle definition for annotation quadrilateral";
            *error = new char[strlen(e)+1];
            strcpy(*error, e);
        } else {
            Handle<Value> x1v = rect->ToObject()->Get(x1k);
            Handle<Value> x2v = rect->ToObject()->Get(x2k);
            Handle<Value> y1v = rect->ToObject()->Get(y1k);
            Handle<Value> y2v = rect->ToObject()->Get(y2k);
            if (!x1v->IsNumber() || !x2v->IsNumber() || !y1v->IsNumber() || !y2v->IsNumber()) {
                char *e = (char*)"Wrong values for rectangle corners definition";
                *error = new char(strlen(e)+1);
                strcpy(*error, e);
            } else {
                int rotation = getRotate();
                switch (rotation) {
                    case 90:
                        *x1 = *x2 = pg->getCropWidth() * (1-y1v->NumberValue());
                        *x3 = *x4 = pg->getCropWidth() * (1-y2v->NumberValue());
                        *y2 = *y4 = pg->getCropHeight() * x2v->NumberValue();
                        *y1 = *y3 = pg->getCropHeight() * x1v->NumberValue();
                        break;
                    case 180:
                        *x1 = *x2 = pg->getCropWidth() * (1-x2v->NumberValue());
                        *x3 = *x4 = pg->getCropWidth() * (1-x1v->NumberValue());
                        *y2 = *y4 = pg->getCropHeight() * (1-y2v->NumberValue());
                        *y1 = *y3 = pg->getCropHeight() * (1-y1v->NumberValue());
                        break;
                    case 270:
                        *x1 = *x2 = pg->getCropWidth() * (y1v->NumberValue());
                        *x3 = *x4 = pg->getCropWidth() * (y2v->NumberValue());
                        *y2 = *y4 = pg->getCropHeight() * (1-x2v->NumberValue());
                        *y1 = *y3 = pg->getCropHeight() * (1-x1v->NumberValue());
                        break;
                    default:
                        *x1 = *x2 = pg->getCropWidth() * x1v->NumberValue();
                        *x3 = *x4 = pg->getCropWidth() * x2v->NumberValue();
                        *y2 = *y4 = pg->getCropHeight() * y1v->NumberValue();
                        *y1 = *y3 = pg->getCropHeight() * y2v->NumberValue();
                        break;
                }
            }
        }
    }

    /**
     * Displaying page slice to stream work->f
     */
    void NodePopplerPage::display(RenderWork *work) {
        SplashColor paperColor;
        paperColor[0] = 255;
        paperColor[1] = 255;
        paperColor[2] = 255;
        SplashOutputDev *splashOut = new SplashOutputDev(
            splashModeRGB8,
            4, gFalse,
            paperColor);
        splashOut->startDoc(work->self->doc);
        ImgWriter *writer = NULL;
        switch (work->w) {
            case W_PNG:
                writer = new PNGWriter();
                break;
            case W_JPEG:
                writer = new JpegWriter(work->quality, work->progressive);
                break;
            case W_TIFF:
#if (POPPLER_VERSION_MINOR > 20)
                writer = new TiffWriter(TiffWriter::RGB);
#else
                writer = new TiffWriter();
                ((TiffWriter*)writer)->setSplashMode(splashModeRGB8);
#endif
                ((TiffWriter*)writer)->setCompressionString(work->compression);
        }

        work->self->pg->displaySlice(splashOut, work->PPI, work->PPI, 0, gFalse, gTrue, work->sx, work->sy, work->sw, work->sh, gFalse);

        SplashBitmap *bitmap = splashOut->getBitmap();
        SplashError e = bitmap->writeImgFile(writer, work->f, (int)work->PPI, (int)work->PPI);
        delete splashOut;
        if (writer != NULL) delete writer;

        if (e) {
            char err[256];
            sprintf(err, "SplashError %d", e);
            work->error = new char[strlen(err)+1];
            strcpy(work->error, err);
        }
    }

    /**
     * Renders page to a file stream
     *
     * Backend function for \see NodePopplerPage::renderToBuffer and \see NodePopplerPage::renderToFile
     */
    void NodePopplerPage::renderToStream(int argc, Handle<Value> argv[], RenderWork *work) {
        HandleScope scope;
        double x, y, w, h, scale, scaledWidth, scaledHeight;

        parseRenderArguments(argv, argc, work, &x, &y, &w, &h);

        if (work->error) {
            if (!work->callback.IsEmpty()) {
                Local<Value> err = Exception::Error(String::New(work->error));
                Local<Value> argv[] = {err};
                TryCatch try_catch;
                work->callback->Call(Context::GetCurrent()->Global(), 1, argv);
                if (try_catch.HasCaught()) {
                    node::FatalException(try_catch);
                }
                delete work;
                return;
            } else {
                return;
            }
        }

        // cap width and height to fit page size
        if (y + h > 1.0) { h = 1.0 - y; }
        if (x + w > 1.0) { w = 1.0 - x; }

        scale = work->PPI / 72.0;
        scaledWidth = getWidth() * scale;
        scaledHeight = getHeight() * scale;
        work->sw = scaledWidth * w;
        work->sh = scaledHeight * h;
        work->sx = scaledWidth * x;
        work->sy = scaledHeight - scaledHeight * y - work->sh; // convert to bottom related coords

        if ((unsigned long)work->sh * work->sw > 100000000L) {
            if (!work->callback.IsEmpty()) {
                Local<Value> err = Exception::Error(String::New("Result image is too big"));
                Local<Value> argv[] = {err};
                TryCatch try_catch;
                work->callback->Call(Context::GetCurrent()->Global(), 1, argv);
                if (try_catch.HasCaught()) {
                    node::FatalException(try_catch);
                }
                delete work;
                return;
            } else {
                char *e = (char*)"Result image is too big";
                work->error = new char[strlen(e)+1];
                strcpy(work->error, e);
                return;
            }
        }

        if (work->callback.IsEmpty()) {
            display(work);
        } else {
            uv_queue_work(uv_default_loop(), &work->request, AsyncRenderWork, AsyncRenderAfter);
        }
    }

    void NodePopplerPage::AsyncRenderWork(uv_work_t *req) {
        RenderWork *work = static_cast<RenderWork*>(req->data);
        display(work);
    }

    void NodePopplerPage::AsyncRenderAfter(uv_work_t *req, int status) {
        HandleScope scope;
        RenderWork *work = static_cast<RenderWork*>(req->data);

        if (work->error) {
            Local<Value> err = Exception::Error(String::New(work->error));
            Local<Value> argv[] = {err};
            TryCatch try_catch;
            work->callback->Call(Context::GetCurrent()->Global(), 1, argv);
            if (try_catch.HasCaught()) {
                node::FatalException(try_catch);
            }
            if (work->dest == DEST_FILE || work->w == W_TIFF) {
                unlink(work->filename);
            }
        } else {
            switch (work->dest) {
                case DEST_FILE:
                {
                    Local<v8::Object> out = v8::Object::New();
                    out->Set(String::NewSymbol("type"), String::NewSymbol("file"));
                    out->Set(String::NewSymbol("path"), String::New(work->filename));
                    Local<Value> args[] = {Local<Value>::New(Null()), Local<Value>::New(out)};
                    work->callback->Call(Context::GetCurrent()->Global(), 2, args);
                    break;
                }
                case DEST_BUFFER:
                {
                    Buffer *buffer;
                    Local<v8::Object> out = v8::Object::New();

                    if (work->w == W_TIFF) {
                        struct stat s;
                        int filedes;
                        filedes = open(work->filename, O_RDONLY);
                        fstat(filedes, &s);
                        if (s.st_size > 0) {
                            work->mstrm_len = s.st_size;
                            work->mstrm_buf = (char*) malloc(work->mstrm_len);
                            read(filedes, work->mstrm_buf, work->mstrm_len);
                        }
                        close(filedes);
                        remove(work->filename);
                    } else {
                        // must close memstream before read from it's buffer
                        fclose(work->f);
                        work->f = NULL;
                    }

                    buffer = Buffer::New(work->mstrm_len);
                    memcpy(Buffer::Data(buffer), work->mstrm_buf, work->mstrm_len);
                    out->Set(String::NewSymbol("type"), String::NewSymbol("buffer"));
                    out->Set(String::NewSymbol("format"), String::NewSymbol(work->format));
                    out->Set(String::NewSymbol("data"), buffer->handle_);
                    Local<Value> args[] = {Local<Value>::New(Null()), Local<Value>::New(out)};
                    work->callback->Call(Context::GetCurrent()->Global(), 2, args);
                    break;
                }
            }
        }

        delete work;
    }

    /**
     * Renders page to a Buffer
     *
     * Javascript function
     *
     * \param method String \see NodePopplerPage::renderToFile
     * \param PPI Number \see NodePopplerPage::renderToFile
     * \param options Object \see NodePopplerPage::renderToFile
     * \param callback Function \see NodePopplerPage::renderToFile
     */
    Handle<Value> NodePopplerPage::renderToBuffer(const Arguments &args) {
        HandleScope scope;
        NodePopplerPage* self = ObjectWrap::Unwrap<NodePopplerPage>(args.Holder());
        RenderWork *work = new RenderWork(self, DEST_BUFFER);

        if (args.Length() < 2 || !args[0]->IsString()) {
            delete work;
            return ThrowException(Exception::Error(String::New(
                "Arguments: (method: String, PPI: Number[, options: Object]")));
        }

        if (args[args.Length() - 1]->IsFunction()) {
            work->callback = Persistent<v8::Function>::New(Local<v8::Function>::Cast(args[args.Length() - 1]));
        }

        if (self->isDocClosed()) {
            Local<Value> err = Exception::Error(String::New("Document closed. You must delete this page"));
            THROW_SYNC_ASYNC_ERR(work, err);
        }

        work->setWriter(args[0]);
        if (work->error) {
            Local<Value> err = Exception::Error(String::New(work->error));
            THROW_SYNC_ASYNC_ERR(work, err);
        }

        work->setPPI(args[1]);
        if (work->error) {
            Local<Value> err = Exception::Error(String::New(work->error));
            THROW_SYNC_ASYNC_ERR(work, err);
        }

        // Hack. libtiff fail on writing to memstream
        if (work->w == W_TIFF) {
            tmpnam(work->filename);
            work->f = fopen(work->filename, "wb");
        } else {
            work->f = open_memstream(&work->mstrm_buf, &work->mstrm_len);
        }
        
        if (!work->f) {
            Local<Value> err = Exception::Error(String::New("Can't open output stream"));
            THROW_SYNC_ASYNC_ERR(work, err);
        }

        if (!work->callback.IsEmpty()) {
            if (args.Length() > 3) {
                Local<Value> argv[3] = {args[0], args[1], args[2]};
                self->renderToStream(3, argv, work);
            } else {
                Local<Value> argv[2] = {args[0], args[1]};
                self->renderToStream(2, argv, work);
            }
            return scope.Close(Undefined());
        } else {
            if (args.Length() > 2) {
                Handle<Value> argv[3] = {args[0], args[1], args[2]};
                self->renderToStream(3, argv, work);
            } else {
                Handle<Value> argv[2] = {args[0], args[1]};
                self->renderToStream(2, argv, work);
            }

            if (work->w == W_TIFF) {
                struct stat s;
                int filedes;
                filedes = open(work->filename, O_RDONLY);
                fstat(filedes, &s);
                if (s.st_size > 0) {
                    work->mstrm_len = s.st_size;
                    work->mstrm_buf = (char*) malloc(work->mstrm_len);
                    read(filedes, work->mstrm_buf, work->mstrm_len);
                }
                close(filedes);
                remove(work->filename);
            } else {
                // must close memstream before read from it's buffer
                fclose(work->f);
                work->f = NULL;
            }


            if (work->error) {
                Handle<Value> e = Exception::Error(String::New(work->error));
                delete work;
                return ThrowException(e);
            } else {
                Buffer *buffer = Buffer::New(work->mstrm_len);
                Handle<v8::Object> out = v8::Object::New();

                memcpy(Buffer::Data(buffer), work->mstrm_buf, work->mstrm_len);

                out->Set(String::NewSymbol("type"), String::NewSymbol("buffer"));
                out->Set(String::NewSymbol("format"), args[0]);
                out->Set(String::NewSymbol("data"), buffer->handle_);

                delete work;
                return scope.Close(out);
            }
        }
    }

    /**
     * Renders page to a file
     *
     * Javascript function
     *
     * \param path String. Path to output file.
     * \param method String with value 'png', 'jpeg' or 'tiff'. Image compression method.
     * \param PPI Number. Pixel per inch value.
     * \param options Object with optional fields:
     *   quality: Integer - defines jpeg quality value (0 - 100) if
     *              image compression method 'jpeg' (default 100)
     *   compression: String - defines tiff compression string if image compression method
     *              is 'tiff' (default NULL).
     *   progressive: Boolean - defines progressive compression for JPEG (default false)
     *   slice: Object - Slice definition in format of object with fields
     *            x: for relative x coordinate of bottom left corner
     *            y: for relative y coordinate of bottom left corner
     *            w: for relative slice width
     *            h: for relative slice height
     * \param callback Function. If exists, then called asynchronously
     *
     * \return Node::Buffer Buffer with rendered image data.
     */
    Handle<Value> NodePopplerPage::renderToFile(const Arguments &args) {
        HandleScope scope;
        NodePopplerPage* self = ObjectWrap::Unwrap<NodePopplerPage>(args.Holder());
        RenderWork *work = new RenderWork(self, DEST_FILE);

        if (args.Length() < 3) {
            Local<Value> err = Exception::Error(String::New(
                "Arguments: (path: String, method: String, PPI: Number[, options: Object, callback: Function])"));
            THROW_SYNC_ASYNC_ERR(work, err);
        }

        if (args[args.Length() - 1]->IsFunction()) {
            work->callback = Persistent<v8::Function>::New(Local<v8::Function>::Cast(args[args.Length() - 1]));
        }

        if (self->isDocClosed()) {
            Local<Value> err = Exception::Error(String::New("Document closed. You must delete this page"));
            THROW_SYNC_ASYNC_ERR(work, err);
        }

        work->setPath(args[0]);
        if (work->error) {
            Local<Value> err = Exception::Error(String::New(work->error));
            THROW_SYNC_ASYNC_ERR(work, err);
        }

        work->setWriter(args[1]);
        if (work->error) {
            Local<Value> err = Exception::Error(String::New(work->error));
            THROW_SYNC_ASYNC_ERR(work, err);
        }

        work->setPPI(args[2]);
        if (work->error) {
            Local<Value> err = Exception::Error(String::New(work->error));
            THROW_SYNC_ASYNC_ERR(work, err);
        }

        work->f = fopen(work->filename, "wb");
        if (!work->f) {
            Local<Value> err = Exception::Error(String::New("Can't open output file"));
            THROW_SYNC_ASYNC_ERR(work, err);
        }

        if (!work->callback.IsEmpty()) {
            if (args.Length() == 4) {
                Handle<Value> argv[2] = {args[1], args[2]};
                self->renderToStream(2, argv, work);
            } else {
                Handle<Value> argv[3] = {args[1], args[2], args[3]};
                self->renderToStream(3, argv, work);
            }
            return scope.Close(Undefined());
        } else {
            if (args.Length() == 3) {
                Handle<Value> argv[2] = {args[1], args[2]};
                self->renderToStream(2, argv, work);
            } else {
                Handle<Value> argv[3] = {args[1], args[2], args[3]};
                self->renderToStream(3, argv, work);
            }

            if (work->error) {
                Handle<Value> e = Exception::Error(String::New(work->error));
                unlink(work->filename);
                delete work;
                return ThrowException(e);
            } else {
                Handle<v8::Object> out = v8::Object::New();
                out->Set(String::NewSymbol("type"), String::NewSymbol("file"));
                out->Set(String::NewSymbol("path"), String::New(work->filename));
                delete work;
                return scope.Close(out);
            }
        }
    }

    /**
     * Parses render arguments in order [method, PPI, options]
     *
     * Parses method, PPI and image compression method options (quality for jpeg and compression
     * string for tiff)
     */
    void NodePopplerPage::parseRenderArguments(
            Handle<Value> argv[], int argc, RenderWork *work,
            double *x, double *y, double *w, double *h) {
        HandleScope scope;

        if (argc == 3) {
            parseWriterOptions(argv[2], work);
            if (work->error) { return; }
        }

        Handle<String> sk = String::NewSymbol("slice");
        if (argc == 3 && argv[2]->ToObject()->Has(sk)) {
            parseSlice(argv[2]->ToObject()->Get(sk), x, y, w, h, &work->error);
            if (work->error) { return; }
        } else {
            *x = *y = 0;
            *w = *h = 1;
        }
    }

    /**
     * Parse writer options
     */
    void NodePopplerPage::parseWriterOptions(Handle<Value> optionsValue, RenderWork *work) {
        HandleScope scope;

        Local<String> ck = String::NewSymbol("compression");
        Local<String> qk = String::NewSymbol("quality");
        Local<String> pk = String::NewSymbol("progressive");
        Local<v8::Object> options;

        if (!optionsValue->IsObject()) {
            char *e = (char*)"'options' must be an instance of Object";
            work->error = new char[strlen(e)+1];
            strcpy(work->error, e);
            return;
        } else {
            options = optionsValue->ToObject();
        }
        
        switch (work->w) {
            case W_TIFF:
                if (options->Has(ck)) {
                    Handle<Value> cv = options->Get(ck);
                    if (cv->IsString() && cv->ToString()->Utf8Length() > 0) {
                        Handle<String> cmp = cv->ToString();
                        work->compression = new char[cmp->Utf8Length()+1];
                        cmp->WriteUtf8(work->compression);
                    }
                }
                break;
            case W_JPEG:
                if (options->Has(qk)) {
                    Handle<Value> qv = options->Get(qk);
                    if (qv->IsUint32()) {
                        work->quality = qv->Uint32Value();
                        if (0 > work->quality || work->quality > 100) {
                            char *e = (char*)"'quality' not in 0 - 100 interval";
                            work->error = new char[strlen(e)+1];
                            strcpy(work->error, e);
                        }
                    } else {
                        char *e = (char*)"'quality' must be 0 - 100 interval integer";
                        work->error = new char[strlen(e)+1];
                        strcpy(work->error, e);
                    }
                }
                if (options->Has(pk)) {
                    Handle<Value> pv = options->Get(pk);
                    if (pv->IsBoolean()) {
                        work->progressive = pv->BooleanValue();
                    } else {
                        char *e = (char*)"'progressive' must be a boolean value";
                        work->error = new char[strlen(e)+1];
                        strcpy(work->error, e);
                    }
                }
                break;
            case W_PNG:
                break;
        }
    }

    /**
     * Parse slice values
     *
     * \see NodePopplerPage::renderToFile
     */
    void NodePopplerPage::parseSlice(
            Handle<Value> sliceValue,
            double *x, double *y, double *w, double *h,
            char **error) {
        HandleScope scope;
        Local<v8::Object> slice;
        Local<String> xk = String::NewSymbol("x");
        Local<String> yk = String::NewSymbol("y");
        Local<String> wk = String::NewSymbol("w");
        Local<String> hk = String::NewSymbol("h");

        if (!sliceValue->IsObject()) {
            char *e = (char*)"'slice' must be an instance of Object";
            *error = new char[strlen(e)+1];
            strcpy(*error, e);
            return;
        } else {
            slice = sliceValue->ToObject();
        }

        if (slice->Has(xk) && slice->Has(yk) && slice->Has(wk) && slice->Has(hk)) {
            Local<Value> xv = slice->Get(xk);
            Local<Value> yv = slice->Get(yk);
            Local<Value> wv = slice->Get(wk);
            Local<Value> hv = slice->Get(hk);
            if (!xv->IsNumber() || !yv->IsNumber() || !wv->IsNumber() || !hv->IsNumber()) {
                char *e = (char*)"Wrong values for slice";
                *error = new char[strlen(e)+1];
                strcpy(*error, e);
            } else {
                *x = xv->NumberValue();
                *y = yv->NumberValue();
                *w = wv->NumberValue();
                *h = hv->NumberValue();
                if (
                        (0 > *x || *x > 1) ||
                        (0 > *y || *y > 1) ||
                        (0 > *w || *w > 1) ||
                        (0 > *h || *h > 1)) {
                    char *e = (char*)"Slice values not in 0 - 1 interval";
                    *error = new char[strlen(e)+1];
                    strcpy(*error, e);
                }
            }
        } else {
            char *e = (char*)"Not enough values for slice";
            *error = new char[strlen(e)+1];
            strcpy(*error, e);
        }
    }

    void NodePopplerPage::RenderWork::setWriter(Handle<Value> method) {
        HandleScope scope;
        if (method->IsString()) {
            String::Utf8Value m(method);
            if (strncmp(*m, "png", 3) == 0) {
                this->w = W_PNG;
            } else if (strncmp(*m, "jpeg", 4) == 0) {
                this->w = W_JPEG;
            } else if (strncmp(*m, "tiff", 4) == 0) {
                this->w = W_TIFF;
            } else {
                char *e = (char*)"Unsupported compression method";
                this->error = new char[strlen(e)+1];
                strcpy(this->error, e);
            }
            strcpy(this->format, *m);
        } else {
            char *e = (char*)"'method' must be an instance of String";
            this->error = new char[strlen(e)+1];
            strcpy(this->error, e);
        }
    }

    void NodePopplerPage::RenderWork::setPPI(Handle<Value> PPI) {
        HandleScope scope;
        if (PPI->IsNumber()) {
            double ppi;
            ppi = PPI->NumberValue();
            if (0 > ppi) {
                char *e = (char*)"'PPI' value must be greater then 0";
                this->error = new char[strlen(e)+1];
                strcpy(this->error, e);
            } else {
                this->PPI = ppi;
            }
        } else {
            char *e = (char*)"'PPI' must be an instance of number";
            this->error = new char[strlen(e)+1];
            strcpy(this->error, e);
        }
    }

    void NodePopplerPage::RenderWork::setPath(Handle<Value> path) {
        HandleScope scope;
        if (path->IsString()) {
            if (path->ToString()->Utf8Length() > 0) {
                this->filename = new char[path->ToString()->Utf8Length() + 1];
                path->ToString()->WriteUtf8(this->filename);
            } else {
                char *e = (char*) "'path' can't be empty";
                this->error = new char[strlen(e)+1];
                strcpy(this->error, e);
            }
        } else {
            char *e = (char*) "'path' must be an instance of string";
            this->error = new char[strlen(e)+1];
            strcpy(this->error, e);
        }
    }
}

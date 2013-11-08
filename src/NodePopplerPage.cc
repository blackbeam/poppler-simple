#include <v8.h>
#include <node.h>
#include <node_buffer.h>

#include "NodePopplerDocument.h"
#include "NodePopplerPage.h"

#define THROW_SYNC_ASYNC_ERR(work, err) \
    if (work->callback.IsEmpty()) { \
        delete work; \
        V8_THROW(err); \
    } else { \
        Local<Value> argv[] = {err}; \
        TryCatch try_catch; \
        EXTRACT_CALLBACK(callback, work->callback); \
        callback->Call(Context::GetCurrent()->Global(), 1, argv); \
        if (try_catch.HasCaught()) { \
            node::FatalException(try_catch); \
        } \
        delete work; \
        V8_RETURN(scope.Close(Undefined())); \
    }

using namespace v8;
using namespace node;

namespace node {

    void NodePopplerPage::Init(v8::Handle<v8::Object> exports) {
        Local<FunctionTemplate> tpl = FunctionTemplate::New(NodePopplerPage::New);
        tpl->SetClassName(String::NewSymbol("PopplerPage"));
        tpl->InstanceTemplate()->SetInternalFieldCount(1);

        NODE_SET_PROTOTYPE_METHOD(tpl, "renderToFile", NodePopplerPage::renderToFile);
        NODE_SET_PROTOTYPE_METHOD(tpl, "renderToBuffer", NodePopplerPage::renderToBuffer);
        NODE_SET_PROTOTYPE_METHOD(tpl, "findText", NodePopplerPage::findText);
        NODE_SET_PROTOTYPE_METHOD(tpl, "getWordList", NodePopplerPage::getWordList);
#if POPPLER_VERSION_MAJOR == 0 && POPPLER_VERSION_MINOR < 20
#else
        NODE_SET_PROTOTYPE_METHOD(tpl, "addAnnot", NodePopplerPage::addAnnot);
        NODE_SET_PROTOTYPE_METHOD(tpl, "deleteAnnots", NodePopplerPage::deleteAnnots);
#endif

        tpl->InstanceTemplate()->SetAccessor(String::NewSymbol("num"), NodePopplerPage::paramsGetter);
        tpl->InstanceTemplate()->SetAccessor(String::NewSymbol("width"), NodePopplerPage::paramsGetter);
        tpl->InstanceTemplate()->SetAccessor(String::NewSymbol("height"), NodePopplerPage::paramsGetter);
        tpl->InstanceTemplate()->SetAccessor(String::NewSymbol("crop_box"), NodePopplerPage::paramsGetter);
#if POPPLER_VERSION_MAJOR == 0 && POPPLER_VERSION_MINOR < 19
#else
        tpl->InstanceTemplate()->SetAccessor(String::NewSymbol("numAnnots"), NodePopplerPage::paramsGetter);
#endif
        tpl->InstanceTemplate()->SetAccessor(String::NewSymbol("media_box"), NodePopplerPage::paramsGetter);
        tpl->InstanceTemplate()->SetAccessor(String::NewSymbol("art_box"), NodePopplerPage::paramsGetter);
        tpl->InstanceTemplate()->SetAccessor(String::NewSymbol("trim_box"), NodePopplerPage::paramsGetter);
        tpl->InstanceTemplate()->SetAccessor(String::NewSymbol("bleed_box"), NodePopplerPage::paramsGetter);
        tpl->InstanceTemplate()->SetAccessor(String::NewSymbol("rotate"), NodePopplerPage::paramsGetter);
        tpl->InstanceTemplate()->SetAccessor(String::NewSymbol("isCropped"), NodePopplerPage::paramsGetter);
        
        exports->Set(String::NewSymbol("PopplerPage"), tpl->GetFunction());
    }

    NodePopplerPage::~NodePopplerPage() {
        if (text != NULL) { text->decRefCnt(); }
        if (color != NULL) { delete color; }
        if (!docClosed) { parent->evPageClosed(this); }
    }

    NodePopplerPage::NodePopplerPage(NodePopplerDocument* doc, const int32_t pageNum) : ObjectWrap() {
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

    V8_METHOD(NodePopplerPage::New) {
        CREATE_HANDLE_SCOPE;
        NodePopplerDocument* doc;
        int32_t pageNum;

        if (args.Length() != 2) {
            V8_THROW(Exception::Error(
                String::New("Two arguments required: (doc: NodePopplerDocument, page: Uint32).")));
        }
        if (!args[1]->IsUint32()) {
            V8_THROW(
                Exception::TypeError(String::New("'page' must be an instance of Uint32.")));
        }
        pageNum = args[1]->ToUint32()->Value();

        if(!args[0]->IsObject()) { // TODO: hasInstance
            V8_THROW(Exception::TypeError(
                String::New("'doc' must be an instance of NodePopplerDocument.")));
        }

        doc = ObjectWrap::Unwrap<NodePopplerDocument>(args[0]->ToObject());
        if (0 >= pageNum || pageNum > doc->doc->getNumPages()) {
            V8_THROW(Exception::Error(String::New("Page number out of bounds.")));
        }

        NodePopplerPage* page = new NodePopplerPage(doc, pageNum);
        if(!page->isOk()) {
            delete page;
            V8_THROW(Exception::Error(String::New("Can't open page.")));;
        }
        page->wrap(args.This());
        V8_RETURN(args.This());
    }

    V8_ACCESSOR_GETTER(NodePopplerPage::paramsGetter) {
        CREATE_HANDLE_SCOPE;

        String::Utf8Value propName(property);
        NodePopplerPage *self = ObjectWrap::Unwrap<NodePopplerPage>(info.This());

        if (strcmp(*propName, "width") == 0) {
            V8_ACCESSOR_RETURN(scope.Close(Number::New(self->getWidth())));

        } else if (strcmp(*propName, "height") == 0) {
            V8_ACCESSOR_RETURN(scope.Close(Number::New(self->getHeight())));

        } else if (strcmp(*propName, "num") == 0) {
            V8_ACCESSOR_RETURN(scope.Close(Uint32::New(self->pg->getNum())));

        } else if (strcmp(*propName, "crop_box") == 0) {
            PDFRectangle *rect = self->pg->getCropBox();
            Local<v8::Object> crop_box = v8::Object::New();

            crop_box->Set(String::NewSymbol("x1"), Number::New(rect->x1));
            crop_box->Set(String::NewSymbol("x2"), Number::New(rect->x2));
            crop_box->Set(String::NewSymbol("y1"), Number::New(rect->y1));
            crop_box->Set(String::NewSymbol("y2"), Number::New(rect->y2));

            V8_ACCESSOR_RETURN(scope.Close(crop_box));

        } else if (strcmp(*propName, "media_box") == 0) {
            PDFRectangle *rect = self->pg->getMediaBox();
            Local<v8::Object> media_box = v8::Object::New();

            media_box->Set(String::NewSymbol("x1"), Number::New(rect->x1));
            media_box->Set(String::NewSymbol("x2"), Number::New(rect->x2));
            media_box->Set(String::NewSymbol("y1"), Number::New(rect->y1));
            media_box->Set(String::NewSymbol("y2"), Number::New(rect->y2));

            V8_ACCESSOR_RETURN(scope.Close(media_box));

        } else if (strcmp(*propName, "bleed_box") == 0) {
            PDFRectangle *rect = self->pg->getBleedBox();
            Local<v8::Object> bleed_box = v8::Object::New();

            bleed_box->Set(String::NewSymbol("x1"), Number::New(rect->x1));
            bleed_box->Set(String::NewSymbol("x2"), Number::New(rect->x2));
            bleed_box->Set(String::NewSymbol("y1"), Number::New(rect->y1));
            bleed_box->Set(String::NewSymbol("y2"), Number::New(rect->y2));

            V8_ACCESSOR_RETURN(scope.Close(bleed_box));

        } else if (strcmp(*propName, "trim_box") == 0) {
            PDFRectangle *rect = self->pg->getTrimBox();
            Local<v8::Object> trim_box = v8::Object::New();

            trim_box->Set(String::NewSymbol("x1"), Number::New(rect->x1));
            trim_box->Set(String::NewSymbol("x2"), Number::New(rect->x2));
            trim_box->Set(String::NewSymbol("y1"), Number::New(rect->y1));
            trim_box->Set(String::NewSymbol("y2"), Number::New(rect->y2));

            V8_ACCESSOR_RETURN(scope.Close(trim_box));

        } else if (strcmp(*propName, "art_box") == 0) {
            PDFRectangle *rect = self->pg->getArtBox();
            Local<v8::Object> art_box = v8::Object::New();

            art_box->Set(String::NewSymbol("x1"), Number::New(rect->x1));
            art_box->Set(String::NewSymbol("x2"), Number::New(rect->x2));
            art_box->Set(String::NewSymbol("y1"), Number::New(rect->y1));
            art_box->Set(String::NewSymbol("y2"), Number::New(rect->y2));

            V8_ACCESSOR_RETURN(scope.Close(art_box));

        } else if (strcmp(*propName, "rotate") == 0) {
            V8_ACCESSOR_RETURN(scope.Close(Int32::New(self->pg->getRotate())));

        } else if (strcmp(*propName, "numAnnots") == 0) {
#if POPPLER_VERSION_MAJOR == 0 && POPPLER_VERSION_MINOR < 19
            Annots *annots = self->pg->getAnnots(self->doc->getCatalog());
#else
            Annots *annots = self->pg->getAnnots();
#endif
            V8_ACCESSOR_RETURN(scope.Close(Uint32::New(annots->getNumAnnots())));

        } else if (strcmp(*propName, "isCropped") == 0) {
            V8_ACCESSOR_RETURN(scope.Close(Boolean::New(self->pg->isCropped())));

        } else {
            V8_ACCESSOR_RETURN(scope.Close(Null()));
        }
    }

    /**
     * \return Object Array of Objects which represents individual words on page
     *                and stores word text and relative coords
     */
    V8_METHOD(NodePopplerPage::getWordList) {
        CREATE_HANDLE_SCOPE;
        NodePopplerPage* self = ObjectWrap::Unwrap<NodePopplerPage>(args.Holder());
        TextPage *text;
        TextWordList *wordList;

        if (self->isDocClosed()) {
            V8_THROW(Exception::Error(String::New("Document closed. You must delete this page")));
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

        V8_RETURN(scope.Close(v8results));
    }

    /**
     * \return Object Relative coors from lower left corner
     */
    V8_METHOD(NodePopplerPage::findText) {
        CREATE_HANDLE_SCOPE;
        NodePopplerPage* self = ObjectWrap::Unwrap<NodePopplerPage>(args.Holder());
        TextPage *text;
        char *ucs4 = NULL;
        size_t ucs4_len;
        double xMin = 0, yMin = 0, xMax, yMax;
        PDFRectangle **matches = NULL;
        unsigned int cnt = 0;

        if (self->isDocClosed()) {
            V8_THROW(Exception::Error(String::New("Document closed. You must delete this page")));
        }

        if (args.Length() != 1 && !args[0]->IsString()) {
            V8_THROW(Exception::Error(String::New("One argument required: (str: String)")));
        }
        String::Utf8Value str(args[0]);

        iconv_string("UCS-4LE", "UTF-8", *str, *str+strlen(*str)+1, &ucs4, &ucs4_len);
        text = self->getTextPage();

        while (text->findText((unsigned int *)ucs4, ucs4_len/4 - 1,
                 gFalse, gTrue, // startAtTop, stopAtBottom
                 gFalse, gFalse, // startAtLast, stopAtLast
                 gFalse, gFalse, // caseSensitive, backwards
#if POPPLER_VERSION_MAJOR == 0 && POPPLER_VERSION_MINOR < 19
#else
                 gFalse, // wholeWord
#endif
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
        V8_RETURN(scope.Close(v8results));
    }

#if POPPLER_VERSION_MAJOR == 0 && POPPLER_VERSION_MINOR < 20
#else
    /**
     * Deletes typeHighlight annotations from end of an annotations array
     */
    V8_METHOD(NodePopplerPage::deleteAnnots) {
        CREATE_HANDLE_SCOPE;
        NodePopplerPage *self = ObjectWrap::Unwrap<NodePopplerPage>(args.Holder());

        while (true) {
            Annots *annots = self->pg->getAnnots();
            int num_annots = annots->getNumAnnots();
            if (num_annots > 0) {
                Annot *annot = annots->getAnnot(num_annots - 1);
                if (annot->getType() != Annot::typeHighlight) {
                    break;
                }
                self->pg->removeAnnot(annot);
            } else {
                break;
            }
        }

        V8_RETURN(scope.Close(Null()));
    }
#endif

#if POPPLER_VERSION_MAJOR == 0 && POPPLER_VERSION_MINOR < 20
#else
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
    V8_METHOD(NodePopplerPage::addAnnot) {
        CREATE_HANDLE_SCOPE;
        NodePopplerPage* self = ObjectWrap::Unwrap<NodePopplerPage>(args.Holder());

        if (self->isDocClosed()) {
            V8_THROW(Exception::Error(String::New("Document closed. You must delete this page")));
        }

        char *error = NULL;

        if (args.Length() < 1) {
            V8_THROW(Exception::Error(String::New(
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
            V8_THROW(e);
        } else {
            V8_RETURN(scope.Close(Null()));
        }
    }

    /**
     * Add annotations to page
     */
    void NodePopplerPage::addAnnot(const Handle<v8::Array> v8array, char **error) {
        CREATE_HANDLE_SCOPE;

        double x1, y1, x2, y2, x3, y3, x4, y4;
        int len = v8array->Length();
        ::Array *array = new ::Array(doc->getXRef());
        for (int i = 0; i < len; i++) {
            parseAnnot(v8array->Get(i), &x1, &y1, &x2, &y2, &x3, &y3, &x4, &y4, error);
            if (*error) {
                delete array;
                return;
            } else {
                array->add((new ::Object())->initReal(x1));
                array->add((new ::Object())->initReal(y1));
                array->add((new ::Object())->initReal(x2));
                array->add((new ::Object())->initReal(y2));
                array->add((new ::Object())->initReal(x3));
                array->add((new ::Object())->initReal(y3));
                array->add((new ::Object())->initReal(x4));
                array->add((new ::Object())->initReal(y4));
            }
        }

        PDFRectangle *rect = new PDFRectangle(0, 0, 0, 0);
        AnnotQuadrilaterals *aq = new AnnotQuadrilaterals(array, rect);
#if ((POPPLER_VERSION_MINOR == 23) && (POPPLER_VERSION_MICRO >= 3)) || (POPPLER_VERSION_MINOR > 23)
        AnnotTextMarkup *annot = new AnnotTextMarkup(doc, rect, Annot::typeHighlight);
        annot->setQuadrilaterals(aq);
#else
        AnnotTextMarkup *annot = new AnnotTextMarkup(doc, rect, Annot::typeHighlight, aq);
#endif

        annot->setOpacity(.5);
        annot->setColor(color);
        pg->addAnnot(annot);

        delete array;
        delete rect;
        delete aq;
    }
#endif

    /**
     * Parse annotation quadrilateral
     */
    void NodePopplerPage::parseAnnot(const Handle<Value> rect,
            double *x1, double *y1,
            double *x2, double *y2,
            double *x3, double *y3,
            double *x4, double *y4,
            char **error) {
        CREATE_HANDLE_SCOPE;
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
#if POPPLER_VERSION_MAJOR == 0 && POPPLER_VERSION_MINOR < 19
        splashOut->startDoc(work->self->doc->getXRef());
#else
        splashOut->startDoc(work->self->doc);
#endif
        ImgWriter *writer = NULL;
        switch (work->w) {
            case W_PNG:
                writer = new PNGWriter();
                break;
            case W_JPEG:
                writer = new JpegWriter(work->quality, work->progressive);
                break;
            case W_TIFF:
#if POPPLER_VERSION_MAJOR == 0 && POPPLER_VERSION_MINOR < 21
                writer = new TiffWriter();
                ((TiffWriter*)writer)->setSplashMode(splashModeRGB8);
#else
                writer = new TiffWriter(TiffWriter::RGB);
#endif
                ((TiffWriter*)writer)->setCompressionString(work->compression);
        }
#if POPPLER_VERSION_MAJOR == 0 && POPPLER_VERSION_MINOR < 19
        work->self->pg->displaySlice(splashOut, work->PPI, work->PPI,
            0, gFalse, gTrue,
            work->sx, work->sy, work->sw, work->sh,
            gFalse, work->self->doc->getCatalog(),
            NULL, NULL, NULL, NULL);
#else
        work->self->pg->displaySlice(splashOut, work->PPI, work->PPI,
            0, gFalse, gTrue,
            work->sx, work->sy, work->sw, work->sh,
            gFalse);
#endif

        SplashBitmap *bitmap = splashOut->getBitmap();
        SplashError e = bitmap->writeImgFile(writer, work->f, (int)work->PPI, (int)work->PPI);
        delete splashOut;
        if (writer != NULL) delete writer;

        if (e
#if POPPLER_VERSION_MINOR < 20 || (POPPLER_VERSION_MINOR == 20 && POPPLER_VERSION_MICRO < 1)
            // must ignore this due to a bug in poppler
            && e != splashErrGeneric
#endif
            ) {
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
    void NodePopplerPage::renderToStream(RenderWork *work) {
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
        CREATE_HANDLE_SCOPE;
        RenderWork *work = static_cast<RenderWork*>(req->data);

        work->closeStream();

        if (work->error) {
            Local<Value> err = Exception::Error(String::New(work->error));
            Local<Value> argv[] = {err};
            TryCatch try_catch;
            EXTRACT_CALLBACK(callback, work->callback);
            callback->Call(Context::GetCurrent()->Global(), 1, argv);
            if (try_catch.HasCaught()) {
                node::FatalException(try_catch);
            }
        } else {
            switch (work->dest) {
                case DEST_FILE:
                {
                    Local<v8::Object> out = v8::Object::New();
                    out->Set(String::NewSymbol("type"), String::NewSymbol("file"));
                    out->Set(String::NewSymbol("path"), String::New(work->filename));
                    Local<Value> argv[] = {Local<Value>::New(Null()), Local<Value>::New(out)};
                    TryCatch try_catch;
                    EXTRACT_CALLBACK(callback, work->callback);
                    callback->Call(Context::GetCurrent()->Global(), 2, argv);
                    if (try_catch.HasCaught()) {
                        node::FatalException(try_catch);
                    }
                    break;
                }
                case DEST_BUFFER:
                {
#if NODE_VERSION_MAJOR == 0 && NODE_VERSION_MINOR <= 10
                    Buffer* buffer = Buffer::New(work->mstrm_len);
#else
                    Local<v8::Object> buffer = Buffer::New(work->mstrm_len);
#endif
                    Local<v8::Object> out = v8::Object::New();
                    memcpy(Buffer::Data(buffer), work->mstrm_buf, work->mstrm_len);
                    out->Set(String::NewSymbol("type"), String::NewSymbol("buffer"));
                    out->Set(String::NewSymbol("format"), String::NewSymbol(work->format));
#if NODE_VERSION_MAJOR == 0 && NODE_VERSION_MINOR <= 10
                    out->Set(String::NewSymbol("data"), buffer->handle_);
#else
                    out->Set(String::NewSymbol("data"), buffer);
#endif
                    Local<Value> argv[] = {Local<Value>::New(Null()), Local<Value>::New(out)};
                    TryCatch try_catch;
                    EXTRACT_CALLBACK(callback, work->callback);
                    callback->Call(Context::GetCurrent()->Global(), 2, argv);
                    if (try_catch.HasCaught()) {
                        node::FatalException(try_catch);
                    }
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
    V8_METHOD(NodePopplerPage::renderToBuffer) {
        CREATE_HANDLE_SCOPE;
        NodePopplerPage* self = ObjectWrap::Unwrap<NodePopplerPage>(args.Holder());
        RenderWork *work = new RenderWork(self, DEST_BUFFER);

        if (args.Length() < 2 || !args[0]->IsString()) {
            delete work;
            V8_THROW(Exception::Error(String::New(
                "Arguments: (method: String, PPI: Number[, options: Object, callback: Function]")));
        }

        if (args[args.Length() - 1]->IsFunction()) {
            PERSIST_CALLBACK(work->callback, v8::Local<v8::Function>::Cast(args[args.Length() - 1]));
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

        if (args.Length() > 2 && args[2]->IsObject()) {
            work->setWriterOptions(args[2]);
            if (work->error) {
                Local<Value> err = Exception::Error(String::New(work->error));
                THROW_SYNC_ASYNC_ERR(work, err);
            }
        }

        work->openStream();
        if (work->error) {
            Local<Value> err = Exception::Error(String::New(work->error));
            THROW_SYNC_ASYNC_ERR(work, err);
        }

        self->renderToStream(work);
        if (!work->callback.IsEmpty()) {
            V8_RETURN(scope.Close(Undefined()));
        } else {
            work->closeStream();

            if (work->error) {
                Handle<Value> e = Exception::Error(String::New(work->error));
                delete work;
                V8_THROW(e);
            } else {
#if NODE_VERSION_MAJOR == 0 && NODE_VERSION_MINOR <= 10
                Buffer* buffer = Buffer::New(work->mstrm_len);
#else
                Local<v8::Object> buffer = Buffer::New(work->mstrm_len);
#endif
                Handle<v8::Object> out = v8::Object::New();

                memcpy(Buffer::Data(buffer), work->mstrm_buf, work->mstrm_len);

                out->Set(String::NewSymbol("type"), String::NewSymbol("buffer"));
                out->Set(String::NewSymbol("format"), args[0]);
#if NODE_VERSION_MAJOR == 0 && NODE_VERSION_MINOR <= 10
                out->Set(String::NewSymbol("data"), buffer->handle_);
#else
                out->Set(String::NewSymbol("data"), buffer);
#endif

                delete work;
                V8_RETURN(scope.Close(out));
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
    V8_METHOD(NodePopplerPage::renderToFile) {
        CREATE_HANDLE_SCOPE;
        NodePopplerPage* self = ObjectWrap::Unwrap<NodePopplerPage>(args.Holder());
        RenderWork *work = new RenderWork(self, DEST_FILE);

        if (args.Length() < 3) {
            Local<Value> err = Exception::Error(String::New(
                "Arguments: (path: String, method: String, PPI: Number[, options: Object, callback: Function])"));
            THROW_SYNC_ASYNC_ERR(work, err);
        }

        if (args[args.Length() - 1]->IsFunction()) {
            PERSIST_CALLBACK(work->callback, v8::Local<v8::Function>::Cast(args[args.Length() - 1]));
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

        if (args.Length() > 3 && args[3]->IsObject()) {
            work->setWriterOptions(args[3]);
            if (work->error) {
                Local<Value> err = Exception::Error(String::New(work->error));
                THROW_SYNC_ASYNC_ERR(work, err);
            }
        }

        work->openStream();
        if (work->error) {
            Local<Value> err = Exception::Error(String::New(work->error));
            THROW_SYNC_ASYNC_ERR(work, err);
        }

        self->renderToStream(work);
        if (!work->callback.IsEmpty()) {
            V8_RETURN(scope.Close(Undefined()));
        } else {
            work->closeStream();
            if (work->error) {
                Handle<Value> e = Exception::Error(String::New(work->error));
                unlink(work->filename);
                delete work;
                V8_THROW(e);
            } else {
                Handle<v8::Object> out = v8::Object::New();
                out->Set(String::NewSymbol("type"), String::NewSymbol("file"));
                out->Set(String::NewSymbol("path"), String::New(work->filename));
                delete work;
                V8_RETURN(scope.Close(out));
            }
        }
    }

    void NodePopplerPage::RenderWork::setWriter(const Handle<Value> method) {
        CREATE_HANDLE_SCOPE;
        char *e = NULL;
        String::Utf8Value m(method);
        if (m.length() > 0) {
            if (strncmp(*m, "png", 3) == 0) {
                this->w = W_PNG;
            } else if (strncmp(*m, "jpeg", 4) == 0) {
                this->w = W_JPEG;
            } else if (strncmp(*m, "tiff", 4) == 0) {
                this->w = W_TIFF;
            } else {
                e = (char*)"Unsupported compression method";
            }
        } else {
            e = (char*)"'method' must be an instance of String";
        }
        if (e) {
            this->error = new char[strlen(e) + 1];
            strcpy(this->error, e);
        } else {
            strcpy(this->format, *m);
        }
    }

    void NodePopplerPage::RenderWork::setWriterOptions(const Handle<Value> optsVal) {
        CREATE_HANDLE_SCOPE;

        Local<String> ck = String::NewSymbol("compression");
        Local<String> qk = String::NewSymbol("quality");
        Local<String> pk = String::NewSymbol("progressive");
        Local<String> sk = String::NewSymbol("slice");
        Local<v8::Object> options;
        char *e = NULL;

        if (!optsVal->IsObject()) {
            e = (char*) "'options' must be an instance of Object";
        } else {
            options = optsVal->ToObject();
            switch (this->w) {
                case W_TIFF:
                {
                    if (options->Has(ck)) {
                        Handle<Value> cv = options->Get(ck);
                        if (cv->IsString()) {
                            Handle<String> cmp = cv->ToString();
                            if (cmp->Utf8Length() > 0) {
                                this->compression = new char[cmp->Utf8Length()+1];
                                cmp->WriteUtf8(this->compression);
                            } else {
                                e = (char*) "'compression' option value could not be an empty string";
                            }
                        } else {
                            e = (char*) "'compression' option must be an instance of string";
                        }
                    }
                }
                break;
                case W_JPEG:
                {
                    if (options->Has(qk)) {
                        Handle<Value> qv = options->Get(qk);
                        if (qv->IsUint32()) {
                            this->quality = qv->Uint32Value();
                            if (0 > this->quality || this->quality > 100) {
                                e = (char*) "'quality' not in 0 - 100 interval";
                            }
                        } else {
                            e = (char*) "'quality' option value must be 0 - 100 interval integer";
                        }
                    }
                    if (options->Has(pk)) {
                        Handle<Value> pv = options->Get(pk);
                        if (pv->IsBoolean()) {
                            this->progressive = pv->BooleanValue();
                        } else {
                            e = (char*)"'progressive' option value must be a boolean value";
                        }
                    }
                }
                break;
                case W_PNG:
                break;
            }
            if (options->Has(sk)) {\
                this->setSlice(options->Get(sk));
            } else {
                // Injecting fake slice to render whole page
                Local<v8::Object> slice = v8::Object::New();
                slice->Set(String::NewSymbol("x"), Number::New(0));
                slice->Set(String::NewSymbol("y"), Number::New(0));
                slice->Set(String::NewSymbol("w"), Number::New(1));
                slice->Set(String::NewSymbol("h"), Number::New(1));
                this->setSlice(slice);
            }
        }
        if (e) {
            this->error = new char[strlen(e)+1];
            strcpy(this->error, e);
        }
    }

    void NodePopplerPage::RenderWork::setPPI(const Handle<Value> PPI) {
        CREATE_HANDLE_SCOPE;
        char *e = NULL;
        if (PPI->IsNumber()) {
            double ppi;
            ppi = PPI->NumberValue();
            if (0 > ppi) {
                e = (char*)"'PPI' value must be greater then 0";
            } else {
                this->PPI = ppi;
            }
        } else {
            e = (char*)"'PPI' must be an instance of number";
        }
        if (e) {
            this->error = new char[strlen(e) + 1];
            strcpy(this->error, e);
        }
    }

    void NodePopplerPage::RenderWork::setPath(const Handle<Value> path) {
        CREATE_HANDLE_SCOPE;
        char *e = NULL;
        if (path->IsString()) {
            if (path->ToString()->Utf8Length() > 0) {
                this->filename = new char[path->ToString()->Utf8Length() + 1];
                path->ToString()->WriteUtf8(this->filename);
            } else {
                e = (char*) "'path' can't be empty";
            }
        } else {
            e = (char*) "'path' must be an instance of string";
        }
        if (e) {
            this->error = new char[strlen(e) + 1];
            strcpy(this->error, e);
        }
    }

    void NodePopplerPage::RenderWork::setSlice(const Handle<Value> sliceVal) {
        CREATE_HANDLE_SCOPE;
        Local<v8::Object> slice;
        Local<String> xk = String::NewSymbol("x");
        Local<String> yk = String::NewSymbol("y");
        Local<String> wk = String::NewSymbol("w");
        Local<String> hk = String::NewSymbol("h");
        char *e = NULL;
        if (!sliceVal->IsObject()) {
            e = (char*) "'slice' option value must be an instance of Object";
        } else {
            slice = sliceVal->ToObject();
            if (slice->Has(xk) && slice->Has(yk) && slice->Has(wk) && slice->Has(hk)) {
                Local<Value> xv = slice->Get(xk);
                Local<Value> yv = slice->Get(yk);
                Local<Value> wv = slice->Get(wk);
                Local<Value> hv = slice->Get(hk);
                if (xv->IsNumber() && yv->IsNumber() && wv->IsNumber() && hv->IsNumber()) {
                    double x, y, w, h;
                    x = xv->NumberValue();
                    y = yv->NumberValue();
                    w = wv->NumberValue();
                    h = hv->NumberValue();
                    if (
                            (0.0 > x || x > 1.0) ||
                            (0.0 > y || y > 1.0) ||
                            (0.0 > w || w > 1.0) ||
                            (0.0 > h || h > 1.0)) {
                        e = (char*) "Slice values must be 0 - 1 interval numbers";
                    } else {
                        double scale, scaledWidth, scaledHeight;
                        // cap width and height to fit page size
                        if (y + h > 1.0) { h = 1.0 - y; }
                        if (x + w > 1.0) { w = 1.0 - x; }
                        scale = this->PPI / 72.0;
                        scaledWidth = this->self->getWidth() * scale;
                        scaledHeight = this->self->getHeight() * scale;
                        this->sw = scaledWidth * w;
                        this->sh = scaledHeight * h;
                        this->sx = scaledWidth * x;
                        this->sy = scaledHeight - scaledHeight * y - scaledHeight * h; // convert to bottom related coords
                        if ((unsigned long)(this->sh * this->sw) > 100000000L) {
                            e = (char*) "Result image is too big";
                        }
                    }
                } else {
                    e = (char*) "Slice must be an object: {x: Number, y: Number, w: Number, h: Number}";
                }
            } else {
                e = (char*) "Slice must be an object: {x: Number, y: Number, w: Number, h: Number}";
            }
        }
        if (e) {
            this->error = new char[strlen(e) + 1];
            strcpy(this->error, e);
        }
    }

    /**
     * Opens output stream for rendering
     */
    void NodePopplerPage::RenderWork::openStream() {
        char *e = NULL;
        switch(this->dest) {
            case DEST_FILE:
            {
                if (this->filename) {
                    this->f = fopen(this->filename, "wb");
                } else {
                    e = (char*) "Output file name was not set";
                }
            }
            break;
            case DEST_BUFFER:
            {
                if (this->w == W_TIFF) {
                    this->filename = new char[L_tmpnam];
                    this->filename = tmpnam(this->filename);
                    this->f = fopen(this->filename, "wb");
                } else {
                    this->f = open_memstream(&this->mstrm_buf, &this->mstrm_len);
                }
            }
        }
        if (!this->f) {
            e = (char*) "Could not open output stream";
        }
        if (e) {
            this->error = new char[strlen(e) + 1];
            strcpy(this->error, e);
        }
    }

    /**
     * Closes output stream
     */
     void NodePopplerPage::RenderWork::closeStream() {
        fclose(this->f);
        this->f = NULL;
        switch(this->dest) {
            case DEST_FILE:
            break;
            case DEST_BUFFER:
            {
                if (this->w == W_TIFF) {
                    struct stat s;
                    int filedes;
                    filedes = open(this->filename, O_RDONLY);
                    fstat(filedes, &s);
                    if (s.st_size > 0) {
                        this->mstrm_len = s.st_size;
                        this->mstrm_buf = (char*) malloc(this->mstrm_len);
                        ssize_t count = read(filedes, this->mstrm_buf, this->mstrm_len);
                        if (count != (ssize_t) this->mstrm_len && this->error == NULL) {
                            char err[256];
                            sprintf(err, "Can't read temporary file");
                            this->error = new char[strlen(err)+1];
                            strcpy(this->error, err);
                        }
                    }
                    close(filedes);
                    unlink(this->filename);
                }
            }
        }
     }
}

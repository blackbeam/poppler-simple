#include <v8.h>
#include <node.h>
#include <node_buffer.h>

#include "NodePopplerDocument.h"
#include "NodePopplerPage.h"

#define THROW_SYNC_ASYNC_ERR(work, err) \
    if (work->callback == NULL) { \
        delete work; \
        return Nan::ThrowError(err); \
    } else { \
        Local<Value> argv[] = {err}; \
        Nan::TryCatch try_catch; \
        work->callback->Call(1, argv); \
        if (try_catch.HasCaught()) { \
            Nan::FatalException(try_catch); \
        } \
        delete work; \
        return; \
    }

using namespace v8;
using namespace node;

namespace node {

    NAN_MODULE_INIT(NodePopplerPage::Init) {
        Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(NodePopplerPage::New);
        tpl->SetClassName(Nan::New<String>("PopplerPage").ToLocalChecked());
        tpl->InstanceTemplate()->SetInternalFieldCount(1);

        Nan::SetPrototypeMethod(tpl, "renderToFile", NodePopplerPage::renderToFile);
        Nan::SetPrototypeMethod(tpl, "renderToBuffer", NodePopplerPage::renderToBuffer);
        Nan::SetPrototypeMethod(tpl, "findText", NodePopplerPage::findText);
        Nan::SetPrototypeMethod(tpl, "getWordList", NodePopplerPage::getWordList);
#if POPPLER_VERSION_MAJOR == 0 && POPPLER_VERSION_MINOR < 20
#else
        Nan::SetPrototypeMethod(tpl, "addAnnot", NodePopplerPage::addAnnot);
        Nan::SetPrototypeMethod(tpl, "deleteAnnots", NodePopplerPage::deleteAnnots);
#endif

        Nan::SetAccessor(tpl->InstanceTemplate(), Nan::New<String>("num").ToLocalChecked(), NodePopplerPage::paramsGetter);
        Nan::SetAccessor(tpl->InstanceTemplate(), Nan::New<String>("width").ToLocalChecked(), NodePopplerPage::paramsGetter);
        Nan::SetAccessor(tpl->InstanceTemplate(), Nan::New<String>("height").ToLocalChecked(), NodePopplerPage::paramsGetter);
        Nan::SetAccessor(tpl->InstanceTemplate(), Nan::New<String>("crop_box").ToLocalChecked(), NodePopplerPage::paramsGetter);
#if POPPLER_VERSION_MAJOR == 0 && POPPLER_VERSION_MINOR < 19
#else
        Nan::SetAccessor(tpl->InstanceTemplate(), Nan::New<String>("numAnnots").ToLocalChecked(), NodePopplerPage::paramsGetter);
#endif
        Nan::SetAccessor(tpl->InstanceTemplate(), Nan::New<String>("media_box").ToLocalChecked(), NodePopplerPage::paramsGetter);
        Nan::SetAccessor(tpl->InstanceTemplate(), Nan::New<String>("art_box").ToLocalChecked(), NodePopplerPage::paramsGetter);
        Nan::SetAccessor(tpl->InstanceTemplate(), Nan::New<String>("trim_box").ToLocalChecked(), NodePopplerPage::paramsGetter);
        Nan::SetAccessor(tpl->InstanceTemplate(), Nan::New<String>("bleed_box").ToLocalChecked(), NodePopplerPage::paramsGetter);
        Nan::SetAccessor(tpl->InstanceTemplate(), Nan::New<String>("rotate").ToLocalChecked(), NodePopplerPage::paramsGetter);
        Nan::SetAccessor(tpl->InstanceTemplate(), Nan::New<String>("isCropped").ToLocalChecked(), NodePopplerPage::paramsGetter);

        Nan::Set(target,
                 Nan::New<String>("PopplerPage").ToLocalChecked(),
                 Nan::GetFunction(tpl).ToLocalChecked());
    }

    NodePopplerPage::~NodePopplerPage() {
        if (text != NULL) { text->decRefCnt(); }
        if (color != NULL) { delete color; }
        if (!docClosed) { parent->evPageClosed(this); }
    }

    NodePopplerPage::NodePopplerPage(NodePopplerDocument* doc, const int32_t pageNum) {
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

    NAN_METHOD(NodePopplerPage::New) {
        Nan::HandleScope scope;
        NodePopplerDocument* doc;
        int32_t pageNum;

        if (info.Length() != 2) {
            return Nan::ThrowError("Two arguments required: (doc: NodePopplerDocument, page: Uint32).");
        }
        if (!info[1]->IsUint32()) {
            return Nan::ThrowTypeError("'page' must be an instance of Uint32.");
        }
        pageNum = info[1]->ToUint32()->Value();

        if(!info[0]->IsObject()) { // TODO: hasInstance
            return Nan::ThrowTypeError("'doc' must be an instance of NodePopplerDocument.");
        }

        doc = Nan::ObjectWrap::Unwrap<NodePopplerDocument>(info[0]->ToObject());
        if (0 >= pageNum || pageNum > doc->doc->getNumPages()) {
            return Nan::ThrowError("Page number out of bounds.");
        }

        NodePopplerPage* page = new NodePopplerPage(doc, pageNum);
        if(!page->isOk()) {
            delete page;
            return Nan::ThrowError("Can't open page.");
        }
        page->wrap(info.This());
        info.GetReturnValue().Set(info.This());
    }

    NAN_GETTER(NodePopplerPage::paramsGetter) {
        String::Utf8Value propName(property);
        NodePopplerPage *self = Nan::ObjectWrap::Unwrap<NodePopplerPage>(info.This());

        if (strcmp(*propName, "width") == 0) {
            info.GetReturnValue().Set(Nan::New<Number>(self->getWidth()));

        } else if (strcmp(*propName, "height") == 0) {
            info.GetReturnValue().Set(Nan::New<Number>(self->getHeight()));

        } else if (strcmp(*propName, "num") == 0) {
            info.GetReturnValue().Set(Nan::New<Uint32>(self->pg->getNum()));

        } else if (strcmp(*propName, "crop_box") == 0) {
            PDFRectangle *rect = self->pg->getCropBox();
            Local<v8::Object> crop_box = Nan::New<v8::Object>();

            crop_box->Set(Nan::New("x1").ToLocalChecked(), Nan::New<Number>(rect->x1));
            crop_box->Set(Nan::New("x2").ToLocalChecked(), Nan::New<Number>(rect->x2));
            crop_box->Set(Nan::New("y1").ToLocalChecked(), Nan::New<Number>(rect->y1));
            crop_box->Set(Nan::New("y2").ToLocalChecked(), Nan::New<Number>(rect->y2));

            info.GetReturnValue().Set(crop_box);

        } else if (strcmp(*propName, "media_box") == 0) {
            PDFRectangle *rect = self->pg->getMediaBox();
            Local<v8::Object> media_box = Nan::New<v8::Object>();

            media_box->Set(Nan::New("x1").ToLocalChecked(), Nan::New<Number>(rect->x1));
            media_box->Set(Nan::New("x2").ToLocalChecked(), Nan::New<Number>(rect->x2));
            media_box->Set(Nan::New("y1").ToLocalChecked(), Nan::New<Number>(rect->y1));
            media_box->Set(Nan::New("y2").ToLocalChecked(), Nan::New<Number>(rect->y2));

            info.GetReturnValue().Set(media_box);

        } else if (strcmp(*propName, "bleed_box") == 0) {
            PDFRectangle *rect = self->pg->getBleedBox();
            Local<v8::Object> bleed_box = Nan::New<v8::Object>();

            bleed_box->Set(Nan::New("x1").ToLocalChecked(), Nan::New<Number>(rect->x1));
            bleed_box->Set(Nan::New("x2").ToLocalChecked(), Nan::New<Number>(rect->x2));
            bleed_box->Set(Nan::New("y1").ToLocalChecked(), Nan::New<Number>(rect->y1));
            bleed_box->Set(Nan::New("y2").ToLocalChecked(), Nan::New<Number>(rect->y2));

            info.GetReturnValue().Set(bleed_box);

        } else if (strcmp(*propName, "trim_box") == 0) {
            PDFRectangle *rect = self->pg->getTrimBox();
            Local<v8::Object> trim_box = Nan::New<v8::Object>();

            trim_box->Set(Nan::New("x1").ToLocalChecked(), Nan::New<Number>(rect->x1));
            trim_box->Set(Nan::New("x2").ToLocalChecked(), Nan::New<Number>(rect->x2));
            trim_box->Set(Nan::New("y1").ToLocalChecked(), Nan::New<Number>(rect->y1));
            trim_box->Set(Nan::New("y2").ToLocalChecked(), Nan::New<Number>(rect->y2));

            info.GetReturnValue().Set(trim_box);

        } else if (strcmp(*propName, "art_box") == 0) {
            PDFRectangle *rect = self->pg->getArtBox();
            Local<v8::Object> art_box = Nan::New<v8::Object>();

            art_box->Set(Nan::New("x1").ToLocalChecked(), Nan::New<Number>(rect->x1));
            art_box->Set(Nan::New("x2").ToLocalChecked(), Nan::New<Number>(rect->x2));
            art_box->Set(Nan::New("y1").ToLocalChecked(), Nan::New<Number>(rect->y1));
            art_box->Set(Nan::New("y2").ToLocalChecked(), Nan::New<Number>(rect->y2));

            info.GetReturnValue().Set(art_box);

        } else if (strcmp(*propName, "rotate") == 0) {
            info.GetReturnValue().Set(Nan::New<Int32>(self->pg->getRotate()));

        } else if (strcmp(*propName, "numAnnots") == 0) {
#if POPPLER_VERSION_MAJOR == 0 && POPPLER_VERSION_MINOR < 19
            Annots *annots = self->pg->getAnnots(self->doc->getCatalog());
#else
            Annots *annots = self->pg->getAnnots();
#endif
            info.GetReturnValue().Set(Nan::New<Uint32>(annots->getNumAnnots()));

        } else if (strcmp(*propName, "isCropped") == 0) {
            info.GetReturnValue().Set(Nan::New<Boolean>(self->pg->isCropped()));

        } else {
            info.GetReturnValue().Set(Nan::Null());
        }
    }

    /**
     * \return Object Array of Objects which represents individual words on page
     *                and stores word text and relative coords
     */
    NAN_METHOD(NodePopplerPage::getWordList) {
        Nan::HandleScope scope;
        NodePopplerPage* self = Nan::ObjectWrap::Unwrap<NodePopplerPage>(info.Holder());
        TextPage *text;
        TextWordList *wordList;

        GBool rawOrder = info[0]->IsBoolean() ? (Nan::To<bool>(info[0]).FromMaybe(false) ? gTrue : gFalse) : gFalse;

        if (self->isDocClosed()) {
            return Nan::ThrowError("Document closed. You must delete this page");
        }

        text = self->getTextPage(rawOrder);
        wordList = text->makeWordList(gTrue);
        int l = wordList->getLength();
        Local<v8::Array> v8results = Nan::New<v8::Array>(l);
        for (int i = 0; i < l; i++) {
            Local<v8::Object> v8result = Nan::New<v8::Object>();
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

            v8result->Set(Nan::New("x1", 2).ToLocalChecked(), Nan::New<Number>(x1));
            v8result->Set(Nan::New("x2", 2).ToLocalChecked(), Nan::New<Number>(x2));
            v8result->Set(Nan::New("y1", 2).ToLocalChecked(), Nan::New<Number>(y1));
            v8result->Set(Nan::New("y2", 2).ToLocalChecked(), Nan::New<Number>(y2));
            v8result->Set(Nan::New("text", 4).ToLocalChecked(), Nan::New(str->getCString()).ToLocalChecked());

            v8results->Set(i, v8result);

            delete str;
        }
        delete wordList;

        info.GetReturnValue().Set(v8results);
    }

    /**
     * \return Object Relative coors from lower left corner
     */
    NAN_METHOD(NodePopplerPage::findText) {
        Nan::HandleScope scope;
        NodePopplerPage* self = Nan::ObjectWrap::Unwrap<NodePopplerPage>(info.Holder());
        TextPage *text;
        char *ucs4 = NULL;
        size_t ucs4_len;
        double xMin = 0, yMin = 0, xMax, yMax;
        PDFRectangle **matches = NULL;
        unsigned int cnt = 0;

        if (self->isDocClosed()) {
            return Nan::ThrowError("Document closed. You must delete this page");
        }

        if (info.Length() != 1 && !info[0]->IsString()) {
            return Nan::ThrowError("One argument required: (str: String)");
        }
        String::Utf8Value str(info[0]);

        iconv_string("UCS-4LE", "UTF-8", *str, *str+strlen(*str)+1, &ucs4, &ucs4_len);
        text = self->getTextPage(gFalse);

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
        Local<v8::Array> v8results = Nan::New<v8::Array>(cnt);
        for (unsigned int i = 0; i < cnt; i++) {
            PDFRectangle *match = matches[i];
            Local<v8::Object> v8result = Nan::New<v8::Object>();
            v8result->Set(Nan::New("x1").ToLocalChecked(), Nan::New<Number>(match->x1 / self->getWidth()));
            v8result->Set(Nan::New("x2").ToLocalChecked(), Nan::New<Number>(match->x2 / self->getWidth()));
            v8result->Set(Nan::New("y1").ToLocalChecked(), Nan::New<Number>(match->y1 / self->getHeight()));
            v8result->Set(Nan::New("y2").ToLocalChecked(), Nan::New<Number>(match->y2 / self->getHeight()));
            v8results->Set(i, v8result);
            delete match;
        }
        if (ucs4 != NULL) {
            free(ucs4);
        }
        if (matches != NULL) {
            free(matches);
        }
        info.GetReturnValue().Set(v8results);
    }

#if POPPLER_VERSION_MAJOR == 0 && POPPLER_VERSION_MINOR < 20
#else
    /**
     * Deletes typeHighlight annotations from end of an annotations array
     */
    NAN_METHOD(NodePopplerPage::deleteAnnots) {
        Nan::HandleScope scope;
        NodePopplerPage *self = Nan::ObjectWrap::Unwrap<NodePopplerPage>(info.Holder());

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

        info.GetReturnValue().Set(Nan::Null());
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
    NAN_METHOD(NodePopplerPage::addAnnot) {
        Nan::HandleScope scope;
        NodePopplerPage* self = Nan::ObjectWrap::Unwrap<NodePopplerPage>(info.Holder());

        if (self->isDocClosed()) {
            return Nan::ThrowError("Document closed. You must delete this page");
        }

        char *error = NULL;

        if (info.Length() < 1) {
            return Nan::ThrowError("One argument required: (annot: Object | Array).");
        }

        if (info[0]->IsArray()) {
            if (Local<v8::Array>::Cast(info[0])->Length() > 0) {
                self->addAnnot(Local<v8::Array>::Cast(info[0]), &error);
            }
        } else if (info[0]->IsObject()) {
            Local<v8::Array> annot = Nan::New<v8::Array>(1);
            annot->Set(0, info[0]);
            self->addAnnot(annot, &error);
        }

        if (error) {
            Local<Value> e = Nan::Error(error);
            delete [] error;
            return Nan::ThrowError(e);
        } else {
            info.GetReturnValue().Set(Nan::Null());
        }
    }

    /**
     * Add annotations to page
     */
    void NodePopplerPage::addAnnot(const Local<v8::Array> v8array, char **error) {
        Nan::HandleScope scope;

        double x1, y1, x2, y2, x3, y3, x4, y4;
        int len = v8array->Length();
        ::Array *array = new ::Array(doc->getXRef());
        for (int i = 0; i < len; i++) {
            parseAnnot(v8array->Get(i), &x1, &y1, &x2, &y2, &x3, &y3, &x4, &y4, error);
            if (*error) {
                delete array;
                return;
            } else {
#if ((POPPLER_VERSION_MAJOR == 0) && (POPPLER_VERSION_MINOR < 55))
                array->add((new ::Object())->initReal(x1));
                array->add((new ::Object())->initReal(y1));
                array->add((new ::Object())->initReal(x2));
                array->add((new ::Object())->initReal(y2));
                array->add((new ::Object())->initReal(x3));
                array->add((new ::Object())->initReal(y3));
                array->add((new ::Object())->initReal(x4));
                array->add((new ::Object())->initReal(y4));
#else
                array->add(::Object(x1));
                array->add(::Object(y1));
                array->add(::Object(x2));
                array->add(::Object(y2));
                array->add(::Object(x3));
                array->add(::Object(y3));
                array->add(::Object(x4));
                array->add(::Object(y4));
#endif
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
    void NodePopplerPage::parseAnnot(const Local<Value> rect,
            double *x1, double *y1,
            double *x2, double *y2,
            double *x3, double *y3,
            double *x4, double *y4,
            char **error) {
        Nan::HandleScope scope;
        Local<String> x1k = Nan::New("x1").ToLocalChecked();
        Local<String> x2k = Nan::New("x2").ToLocalChecked();
        Local<String> y1k = Nan::New("y1").ToLocalChecked();
        Local<String> y2k = Nan::New("y2").ToLocalChecked();
        if (!rect->IsObject() ||
                !rect->ToObject()->Has(x1k) || !rect->ToObject()->Has(x2k) ||
                !rect->ToObject()->Has(y1k) || !rect->ToObject()->Has(y2k)) {
            char *e = (char*)"Invalid rectangle definition for annotation quadrilateral";
            *error = new char[strlen(e)+1];
            strcpy(*error, e);
        } else {
            Local<Value> x1v = rect->ToObject()->Get(x1k);
            Local<Value> x2v = rect->ToObject()->Get(x2k);
            Local<Value> y1v = rect->ToObject()->Get(y1k);
            Local<Value> y2v = rect->ToObject()->Get(y2k);
            if (!x1v->IsNumber() || !x2v->IsNumber() || !y1v->IsNumber() || !y2v->IsNumber()) {
                char *e = (char*)"Wrong values for rectangle corners definition";
                *error = new char[strlen(e)+1];
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
        int sx, sy, sw, sh;
        std::tie(sx, sy, sw, sh) = work->applyScale();
        if (work->error) return;
#if POPPLER_VERSION_MAJOR == 0 && POPPLER_VERSION_MINOR < 19
        work->self->pg->displaySlice(splashOut, work->PPI, work->PPI,
            0, gFalse, gTrue,
            sx, sy, sw, sh,
            gFalse, work->self->doc->getCatalog(),
            NULL, NULL, NULL, NULL);
#else
        work->self->pg->displaySlice(splashOut, work->PPI, work->PPI,
            0, gFalse, gTrue,
            sx, sy, sw, sh,
            gFalse);
#endif

        SplashBitmap *bitmap = splashOut->getBitmap();
#if POPPLER_VERSION_MAJOR > 0 || (POPPLER_VERSION_MAJOR == 0 && POPPLER_VERSION_MINOR > 49)
	SplashError e = bitmap->writeImgFile(writer, work->f, (int)work->PPI, (int)work->PPI, splashModeRGB8);
#else
        SplashError e = bitmap->writeImgFile(writer, work->f, (int)work->PPI, (int)work->PPI);
#endif
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
        if (work->callback == NULL) {
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
        Nan::HandleScope scope;
        RenderWork *work = static_cast<RenderWork*>(req->data);

        work->closeStream();

        if (work->error) {
            Local<Value> err = Nan::Error(work->error);
            Local<Value> argv[] = {err};
            Nan::TryCatch try_catch;
            work->callback->Call(1, argv);
            if (try_catch.HasCaught()) {
                Nan::FatalException(try_catch);
            }
        } else {
            switch (work->dest) {
                case DEST_FILE:
                {
                    Local<v8::Object> out = Nan::New<v8::Object>();
                    out->Set(Nan::New("type").ToLocalChecked(), Nan::New("file").ToLocalChecked());
                    out->Set(Nan::New("path").ToLocalChecked(), Nan::New(work->filename).ToLocalChecked());
                    Local<Value> argv[] = {Nan::Null(), out};
                    Nan::TryCatch try_catch;
                    work->callback->Call(2, argv);
                    if (try_catch.HasCaught()) {
                        Nan::FatalException(try_catch);
                    }
                    break;
                }
                case DEST_BUFFER:
                {
                    Local<v8::Object> buffer = Nan::NewBuffer(work->mstrm_len)
                                                   .ToLocalChecked();
                    Local<v8::Object> out = Nan::New<v8::Object>();
                    memcpy(Buffer::Data(buffer), work->mstrm_buf, work->mstrm_len);
                    out->Set(Nan::New("type").ToLocalChecked(), Nan::New("buffer").ToLocalChecked());
                    out->Set(Nan::New("format").ToLocalChecked(), Nan::New(work->format).ToLocalChecked());
                    out->Set(Nan::New("data").ToLocalChecked(), buffer);
                    Local<Value> argv[] = {Nan::Null(), out};
                    Nan::TryCatch try_catch;
                    work->callback->Call(2, argv);
                    if (try_catch.HasCaught()) {
                        Nan::FatalException(try_catch);
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
    NAN_METHOD(NodePopplerPage::renderToBuffer) {
        Nan::HandleScope scope;
        NodePopplerPage* self = Nan::ObjectWrap::Unwrap<NodePopplerPage>(info.Holder());
        RenderWork *work = new RenderWork(self, DEST_BUFFER);

        if (info.Length() < 2 || !info[0]->IsString()) {
            delete work;
            return Nan::ThrowError("Arguments: (method: String, PPI: Number[, options: Object, callback: Function]");
        }

        if (info[info.Length() - 1]->IsFunction()) {
            Local<v8::Function> callbackHandle = info[info.Length() - 1].As<v8::Function>();
            work->callback = new Nan::Callback(callbackHandle);
        }

        if (self->isDocClosed()) {
            Local<Value> err = Nan::Error("Document closed. You must delete this page");
            THROW_SYNC_ASYNC_ERR(work, err);
        }

        work->setWriter(info[0]);
        if (work->error) {
            Local<Value> err = Nan::Error(work->error);
            THROW_SYNC_ASYNC_ERR(work, err);
        }

        work->setPPI(info[1]);
        if (work->error) {
            Local<Value> err = Nan::Error(work->error);
            THROW_SYNC_ASYNC_ERR(work, err);
        }

        if (info.Length() > 2 && info[2]->IsObject()) {
            work->setWriterOptions(info[2]);
            if (work->error) {
                Local<Value> err = Nan::Error(work->error);
                THROW_SYNC_ASYNC_ERR(work, err);
            }
        }

        work->openStream();
        if (work->error) {
            Local<Value> err = Nan::Error(work->error);
            THROW_SYNC_ASYNC_ERR(work, err);
        }

        self->renderToStream(work);
        if (work->callback != NULL) {
            return;
        } else {
            work->closeStream();

            if (work->error) {
                Local<Value> e = Nan::Error(work->error);
                delete work;
                return Nan::ThrowError(e);
            } else {
                Local<v8::Object> buffer = Nan::NewBuffer(work->mstrm_len).ToLocalChecked();
                Local<v8::Object> out = Nan::New<v8::Object>();

                memcpy(Buffer::Data(buffer), work->mstrm_buf, work->mstrm_len);

                out->Set(Nan::New("type").ToLocalChecked(), Nan::New("buffer").ToLocalChecked());
                out->Set(Nan::New("format").ToLocalChecked(), info[0]);
                out->Set(Nan::New("data").ToLocalChecked(), buffer);

                delete work;
                info.GetReturnValue().Set(out);
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
    NAN_METHOD(NodePopplerPage::renderToFile) {
        Nan::HandleScope scope;
        NodePopplerPage* self = Nan::ObjectWrap::Unwrap<NodePopplerPage>(info.Holder());
        RenderWork *work = new RenderWork(self, DEST_FILE);

        if (info.Length() < 3) {
            Local<Value> err = Nan::Error(
                "Arguments: (path: String, method: String, PPI: Number[, options: Object, callback: Function])");
            THROW_SYNC_ASYNC_ERR(work, err);
        }

        if (info[info.Length() - 1]->IsFunction()) {
            Local<v8::Function> callbackHandle = info[info.Length() - 1].As<v8::Function>();
            work->callback = new Nan::Callback(callbackHandle);
        }

        if (self->isDocClosed()) {
            Local<Value> err = Nan::Error("Document closed. You must delete this page");
            THROW_SYNC_ASYNC_ERR(work, err);
        }

        work->setPath(info[0]);
        if (work->error) {
            Local<Value> err = Nan::Error(work->error);
            THROW_SYNC_ASYNC_ERR(work, err);
        }

        work->setWriter(info[1]);
        if (work->error) {
            Local<Value> err = Nan::Error(work->error);
            THROW_SYNC_ASYNC_ERR(work, err);
        }

        work->setPPI(info[2]);
        if (work->error) {
            Local<Value> err = Nan::Error(work->error);
            THROW_SYNC_ASYNC_ERR(work, err);
        }

        if (info.Length() > 3 && info[3]->IsObject()) {
            work->setWriterOptions(info[3]);
            if (work->error) {
                Local<Value> err = Nan::Error(work->error);
                THROW_SYNC_ASYNC_ERR(work, err);
            }
        }

        work->openStream();
        if (work->error) {
            Local<Value> err = Nan::Error(work->error);
            THROW_SYNC_ASYNC_ERR(work, err);
        }

        self->renderToStream(work);
        if (work->callback != NULL) {
            return;
        } else {
            work->closeStream();
            if (work->error) {
                Local<Value> e = Nan::Error(work->error);
                unlink(work->filename);
                delete work;
                return Nan::ThrowError(e);
            } else {
                Local<v8::Object> out = Nan::New<v8::Object>();
                out->Set(Nan::New("type").ToLocalChecked(), Nan::New("file").ToLocalChecked());
                out->Set(Nan::New("path").ToLocalChecked(), Nan::New(work->filename).ToLocalChecked());
                delete work;
                info.GetReturnValue().Set(out);
            }
        }
    }

    void NodePopplerPage::RenderWork::setWriter(const Local<Value> method) {
        Nan::HandleScope scope;
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

    void NodePopplerPage::RenderWork::setWriterOptions(const Local<Value> optsVal) {
        Nan::HandleScope scope;

        Local<String> ck = Nan::New("compression").ToLocalChecked();
        Local<String> qk = Nan::New("quality").ToLocalChecked();
        Local<String> pk = Nan::New("progressive").ToLocalChecked();
        Local<String> sk = Nan::New("slice").ToLocalChecked();
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
                        Local<Value> cv = options->Get(ck);
                        if (cv->IsString()) {
                            Local<String> cmp = cv->ToString();
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
                        Local<Value> qv = options->Get(qk);
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
                        Local<Value> pv = options->Get(pk);
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
                Local<v8::Object> slice = Nan::New<v8::Object>();
                slice->Set(Nan::New("x").ToLocalChecked(), Nan::New<Number>(0));
                slice->Set(Nan::New("y").ToLocalChecked(), Nan::New<Number>(0));
                slice->Set(Nan::New("w").ToLocalChecked(), Nan::New<Number>(1));
                slice->Set(Nan::New("h").ToLocalChecked(), Nan::New<Number>(1));
                this->setSlice(slice);
            }
        }
        if (e) {
            this->error = new char[strlen(e)+1];
            strcpy(this->error, e);
        }
    }

    void NodePopplerPage::RenderWork::setPPI(const Local<Value> PPI) {
        Nan::HandleScope scope;
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

    void NodePopplerPage::RenderWork::setPath(const Local<Value> path) {
        Nan::HandleScope scope;
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

    std::tuple<int, int, int, int> NodePopplerPage::RenderWork::applyScale() {
        char *e = NULL;
        double scale, scaledWidth, scaledHeight;
        int scaled_x, scaled_y, scaled_w, scaled_h;
        scale = PPI / 72.0;
        scaledWidth = self->getWidth() * scale;
        scaledHeight = self->getHeight() * scale;
        scaled_w = scaledWidth * slice_w;
        scaled_h = scaledHeight * slice_h;
        scaled_x = scaledWidth * slice_x;
        scaled_y = scaledHeight - scaledHeight * slice_y - scaledHeight * slice_h;
        if ((unsigned long)(scaled_w * scaled_h) > 100000000L)
        {
            e = (char*) "Result image is too big";
        }
        if (e) {
            this->error = new char[strlen(e) + 1];
            strcpy(this->error, e);
        }
        return std::make_tuple(scaled_x, scaled_y, scaled_w, scaled_h);
    }

    void NodePopplerPage::RenderWork::setSlice(const Local<Value> sliceVal) {
        Nan::HandleScope scope;
        Local<v8::Object> slice;
        Local<String> xk = Nan::New("x").ToLocalChecked();
        Local<String> yk = Nan::New("y").ToLocalChecked();
        Local<String> wk = Nan::New("w").ToLocalChecked();
        Local<String> hk = Nan::New("h").ToLocalChecked();
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
                        // cap width and height to fit page size
                        if (y + h > 1.0) { h = 1.0 - y; }
                        if (x + w > 1.0) { w = 1.0 - x; }
                        this->slice_x = x;
                        this->slice_y = y;
                        this->slice_h = h;
                        this->slice_w = w;
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
                if (this->w != W_TIFF) {
                    this->stream = new MemoryStream();
                    this->f = this->stream->open();
                } else {
                    this->filename = new char[17];
                    strcpy(this->filename, "/tmp/psmplXXXXXX");
                    int fd = mkstemp(this->filename);
                    if (fd != -1) {
                        this->f = fdopen(fd, "wb");
                    }
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
        switch(this->dest) {
            case DEST_FILE:
                fclose(this->f);
                this->f = NULL;
            break;
            case DEST_BUFFER:
            {
                if (this->w != W_TIFF) {
                    fclose(this->f);
                    this->f = NULL;
                    this->mstrm_len = this->stream->getBufferLen();
                    this->mstrm_buf = this->stream->giveBuffer();
                } else {
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
                    fclose(this->f);
                    this->f = NULL;
                }
            }
        }
     }
}

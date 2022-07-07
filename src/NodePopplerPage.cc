#include <v8.h>
#include <memory>
#include <node.h>
#include <node_buffer.h>

#include "NodePopplerDocument.h"
#include "NodePopplerPage.h"

int getNumAnnotsHelper(Annots &annots) {
#if ((POPPLER_VERSION_MAJOR == 22) && (POPPLER_VERSION_MINOR >= 3)) || POPPLER_VERSION_MAJOR > 22
    return annots.getAnnots().size();
#else
    return annots.getNumAnnots();
#endif
}

Annot *getAnnotHelper(Annots &annots, int i) {
#if ((POPPLER_VERSION_MAJOR == 22) && (POPPLER_VERSION_MINOR >= 3)) || POPPLER_VERSION_MAJOR > 22
    return annots.getAnnots()[i];
#else
    return annots.getAnnot(i);
#endif
}

#define THROW_SYNC_ASYNC_ERR(work, err)      \
    if (work->callback == NULL)              \
    {                                        \
        delete work;                         \
        return Nan::ThrowError(err);         \
    }                                        \
    else                                     \
    {                                        \
        Local<Value> argv[] = {err};         \
        Nan::TryCatch try_catch;             \
        Nan::Call(*work->callback, 1, argv); \
        if (try_catch.HasCaught())           \
        {                                    \
            Nan::FatalException(try_catch);  \
        }                                    \
        delete work;                         \
        return;                              \
    }

using namespace v8;
using namespace node;
using Nan::To;

namespace node
{

NAN_MODULE_INIT(NodePopplerPage::Init)
{
    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(NodePopplerPage::New);
    tpl->SetClassName(Nan::New<String>("PopplerPage").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "renderToFile", NodePopplerPage::renderToFile);
    Nan::SetPrototypeMethod(tpl, "renderToBuffer", NodePopplerPage::renderToBuffer);
    Nan::SetPrototypeMethod(tpl, "findText", NodePopplerPage::findText);
    Nan::SetPrototypeMethod(tpl, "getWordList", NodePopplerPage::getWordList);
    Nan::SetPrototypeMethod(tpl, "addAnnot", NodePopplerPage::addAnnot);
    Nan::SetPrototypeMethod(tpl, "deleteAnnots", NodePopplerPage::deleteAnnots);

    Nan::SetAccessor(tpl->InstanceTemplate(), Nan::New<String>("num").ToLocalChecked(), NodePopplerPage::paramsGetter);
    Nan::SetAccessor(tpl->InstanceTemplate(), Nan::New<String>("width").ToLocalChecked(), NodePopplerPage::paramsGetter);
    Nan::SetAccessor(tpl->InstanceTemplate(), Nan::New<String>("height").ToLocalChecked(), NodePopplerPage::paramsGetter);
    Nan::SetAccessor(tpl->InstanceTemplate(), Nan::New<String>("crop_box").ToLocalChecked(), NodePopplerPage::paramsGetter);
    Nan::SetAccessor(tpl->InstanceTemplate(), Nan::New<String>("numAnnots").ToLocalChecked(), NodePopplerPage::paramsGetter);
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

NodePopplerPage::~NodePopplerPage()
{
    if (text != NULL)
    {
        text->decRefCnt();
    }
    if (!docClosed)
    {
        parent->evPageClosed(this);
    }
}

NodePopplerPage::NodePopplerPage(NodePopplerDocument *doc, const int32_t pageNum)
    : text(NULL), color_r(0), color_g(1), color_b(0)
{
    pg = doc->doc->getPage(pageNum);
    if (pg && pg->isOk())
    {
        parent = doc;
        parent->evPageOpened(this);
        this->doc = doc->getDoc();
        docClosed = false;
    }
    else
    {
        docClosed = true;
    }
}

void NodePopplerPage::evDocumentClosed()
{
    docClosed = true;
}

NAN_METHOD(NodePopplerPage::New)
{
    Nan::HandleScope scope;
    NodePopplerDocument *doc;
    int32_t pageNum;

    if (info.Length() != 2)
    {
        return Nan::ThrowError("Two arguments required: (doc: NodePopplerDocument, page: Uint32).");
    }
    if (!info[1]->IsUint32())
    {
        return Nan::ThrowTypeError("'page' must be an instance of Uint32.");
    }
    pageNum = To<int32_t>(info[1]).FromJust();

    if (!info[0]->IsObject())
    { // TODO: hasInstance
        return Nan::ThrowTypeError("'doc' must be an instance of NodePopplerDocument.");
    }

    doc = Nan::ObjectWrap::Unwrap<NodePopplerDocument>(To<v8::Object>(info[0]).ToLocalChecked());
    if (0 >= pageNum || pageNum > doc->doc->getNumPages())
    {
        return Nan::ThrowError("Page number out of bounds.");
    }

    NodePopplerPage *page = new NodePopplerPage(doc, pageNum);
    if (!page->isOk())
    {
        delete page;
        return Nan::ThrowError("Can't open page.");
    }
    page->wrap(info.This());
    info.GetReturnValue().Set(info.This());
}

NAN_GETTER(NodePopplerPage::paramsGetter)
{
    Nan::Utf8String propName(property);
    NodePopplerPage *self = Nan::ObjectWrap::Unwrap<NodePopplerPage>(info.This());

    if (strcmp(*propName, "width") == 0)
    {
        info.GetReturnValue().Set(Nan::New<Number>(self->getWidth()));
    }
    else if (strcmp(*propName, "height") == 0)
    {
        info.GetReturnValue().Set(Nan::New<Number>(self->getHeight()));
    }
    else if (strcmp(*propName, "num") == 0)
    {
        info.GetReturnValue().Set(Nan::New<Uint32>(self->pg->getNum()));
    }
    else if (strcmp(*propName, "crop_box") == 0)
    {
        auto rect = self->pg->getCropBox();
        Local<v8::Object> crop_box = Nan::New<v8::Object>();

        Nan::Set(crop_box, Nan::New("x1").ToLocalChecked(), Nan::New<Number>(rect->x1));
        Nan::Set(crop_box, Nan::New("x2").ToLocalChecked(), Nan::New<Number>(rect->x2));
        Nan::Set(crop_box, Nan::New("y1").ToLocalChecked(), Nan::New<Number>(rect->y1));
        Nan::Set(crop_box, Nan::New("y2").ToLocalChecked(), Nan::New<Number>(rect->y2));

        info.GetReturnValue().Set(crop_box);
    }
    else if (strcmp(*propName, "media_box") == 0)
    {
        auto rect = self->pg->getMediaBox();
        Local<v8::Object> media_box = Nan::New<v8::Object>();

        Nan::Set(media_box, Nan::New("x1").ToLocalChecked(), Nan::New<Number>(rect->x1));
        Nan::Set(media_box, Nan::New("x2").ToLocalChecked(), Nan::New<Number>(rect->x2));
        Nan::Set(media_box, Nan::New("y1").ToLocalChecked(), Nan::New<Number>(rect->y1));
        Nan::Set(media_box, Nan::New("y2").ToLocalChecked(), Nan::New<Number>(rect->y2));

        info.GetReturnValue().Set(media_box);
    }
    else if (strcmp(*propName, "bleed_box") == 0)
    {
        auto rect = self->pg->getBleedBox();
        Local<v8::Object> bleed_box = Nan::New<v8::Object>();

        Nan::Set(bleed_box, Nan::New("x1").ToLocalChecked(), Nan::New<Number>(rect->x1));
        Nan::Set(bleed_box, Nan::New("x2").ToLocalChecked(), Nan::New<Number>(rect->x2));
        Nan::Set(bleed_box, Nan::New("y1").ToLocalChecked(), Nan::New<Number>(rect->y1));
        Nan::Set(bleed_box, Nan::New("y2").ToLocalChecked(), Nan::New<Number>(rect->y2));

        info.GetReturnValue().Set(bleed_box);
    }
    else if (strcmp(*propName, "trim_box") == 0)
    {
        auto rect = self->pg->getTrimBox();
        Local<v8::Object> trim_box = Nan::New<v8::Object>();

        Nan::Set(trim_box, Nan::New("x1").ToLocalChecked(), Nan::New<Number>(rect->x1));
        Nan::Set(trim_box, Nan::New("x2").ToLocalChecked(), Nan::New<Number>(rect->x2));
        Nan::Set(trim_box, Nan::New("y1").ToLocalChecked(), Nan::New<Number>(rect->y1));
        Nan::Set(trim_box, Nan::New("y2").ToLocalChecked(), Nan::New<Number>(rect->y2));

        info.GetReturnValue().Set(trim_box);
    }
    else if (strcmp(*propName, "art_box") == 0)
    {
        auto rect = self->pg->getArtBox();
        Local<v8::Object> art_box = Nan::New<v8::Object>();

        Nan::Set(art_box, Nan::New("x1").ToLocalChecked(), Nan::New<Number>(rect->x1));
        Nan::Set(art_box, Nan::New("x2").ToLocalChecked(), Nan::New<Number>(rect->x2));
        Nan::Set(art_box, Nan::New("y1").ToLocalChecked(), Nan::New<Number>(rect->y1));
        Nan::Set(art_box, Nan::New("y2").ToLocalChecked(), Nan::New<Number>(rect->y2));

        info.GetReturnValue().Set(art_box);
    }
    else if (strcmp(*propName, "rotate") == 0)
    {
        info.GetReturnValue().Set(Nan::New<Int32>(self->pg->getRotate()));
    }
    else if (strcmp(*propName, "numAnnots") == 0)
    {
        Annots *annots = self->pg->getAnnots();
        info.GetReturnValue().Set(Nan::New<Uint32>(getNumAnnotsHelper(*annots)));
    }
    else if (strcmp(*propName, "isCropped") == 0)
    {
        info.GetReturnValue().Set(Nan::New<Boolean>(self->pg->isCropped()));
    }
    else
    {
        info.GetReturnValue().Set(Nan::Null());
    }
}

/**
     * \return Object Array of Objects which represents individual words on page
     *                and stores word text and relative coords
     */
NAN_METHOD(NodePopplerPage::getWordList)
{
    Nan::HandleScope scope;
    NodePopplerPage *self = Nan::ObjectWrap::Unwrap<NodePopplerPage>(info.Holder());
    TextPage *text;

    bool rawOrder = info[0]->IsBoolean() ? (To<bool>(info[0]).FromMaybe(false) ? true : false) : false;

    if (self->isDocClosed())
    {
        return Nan::ThrowError("Document closed. You must delete this page");
    }

    text = self->getTextPage(rawOrder);
    auto wordList = text->makeWordList(true);
    int l = wordList->getLength();
    Local<v8::Array> v8results = Nan::New<v8::Array>(l);
    for (int i = 0; i < l; i++)
    {
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

        Nan::Set(v8result, Nan::New("x1", 2).ToLocalChecked(), Nan::New<Number>(x1));
        Nan::Set(v8result, Nan::New("x2", 2).ToLocalChecked(), Nan::New<Number>(x2));
        Nan::Set(v8result, Nan::New("y1", 2).ToLocalChecked(), Nan::New<Number>(y1));
        Nan::Set(v8result, Nan::New("y2", 2).ToLocalChecked(), Nan::New<Number>(y2));
#if POPPLER_VERSION_MAJOR == 0 && POPPLER_VERSION_MINOR < 72
        auto c_str = str->getCString();
#else
        auto c_str = str->c_str();
#endif
        Nan::Set(v8result, Nan::New("text", 4).ToLocalChecked(), Nan::New(c_str).ToLocalChecked());
        Nan::Set(v8results, i, v8result);

        delete str;
    }

#if (POPPLER_VERSION_MAJOR == 21 && POPPLER_VERSION_MINOR < 11) || POPPLER_VERSION_MAJOR < 21
    delete wordList;
#endif

    info.GetReturnValue().Set(v8results);
}

/**
     * \return Object Relative coors from lower left corner
     */
NAN_METHOD(NodePopplerPage::findText)
{
    Nan::HandleScope scope;
    NodePopplerPage *self = Nan::ObjectWrap::Unwrap<NodePopplerPage>(info.Holder());
    TextPage *text;
    char *ucs4 = NULL;
    size_t ucs4_len;
    double xMin = 0, yMin = 0, xMax, yMax;
    PDFRectangle **matches = NULL;
    unsigned int cnt = 0;

    if (self->isDocClosed())
    {
        return Nan::ThrowError("Document closed. You must delete this page");
    }

    if (info.Length() != 1 && !info[0]->IsString())
    {
        return Nan::ThrowError("One argument required: (str: String)");
    }
    Nan::Utf8String str(info[0]);

    iconv_string("UCS-4LE", "UTF-8", *str, *str + strlen(*str) + 1, &ucs4, &ucs4_len);
    text = self->getTextPage(false);

    while (text->findText((unsigned int *)ucs4, ucs4_len / 4 - 1,
                          false, true,  // startAtTop, stopAtBottom
                          false, false, // startAtLast, stopAtLast
                          false, false, // caseSensitive, backwards
                          false, // wholeWord
                          &xMin, &yMin, &xMax, &yMax))
    {
        PDFRectangle **t_matches = matches;
        cnt++;
        matches = (PDFRectangle **)realloc(t_matches, sizeof(PDFRectangle *) * cnt);
        matches[cnt - 1] = new PDFRectangle(xMin, self->getHeight() - yMax, xMax, self->getHeight() - yMin);
    }
    Local<v8::Array> v8results = Nan::New<v8::Array>(cnt);
    for (unsigned int i = 0; i < cnt; i++)
    {
        PDFRectangle *match = matches[i];
        Local<v8::Object> v8result = Nan::New<v8::Object>();
        Nan::Set(v8result, Nan::New("x1").ToLocalChecked(), Nan::New<Number>(match->x1 / self->getWidth()));
        Nan::Set(v8result, Nan::New("x2").ToLocalChecked(), Nan::New<Number>(match->x2 / self->getWidth()));
        Nan::Set(v8result, Nan::New("y1").ToLocalChecked(), Nan::New<Number>(match->y1 / self->getHeight()));
        Nan::Set(v8result, Nan::New("y2").ToLocalChecked(), Nan::New<Number>(match->y2 / self->getHeight()));
        Nan::Set(v8results, i, v8result);
        delete match;
    }
    if (ucs4 != NULL)
    {
        free(ucs4);
    }
    if (matches != NULL)
    {
        free(matches);
    }
    info.GetReturnValue().Set(v8results);
}

/**
* Deletes typeHighlight annotations from end of an annotations array
*/
NAN_METHOD(NodePopplerPage::deleteAnnots)
{
    Nan::HandleScope scope;
    NodePopplerPage *self = Nan::ObjectWrap::Unwrap<NodePopplerPage>(info.Holder());

    while (true)
    {
        Annots *annots = self->pg->getAnnots();
        int num_annots = getNumAnnotsHelper(*annots);
        if (num_annots > 0)
        {
            Annot *annot = getAnnotHelper(*annots, num_annots - 1);
            if (annot->getType() != Annot::typeHighlight)
            {
                break;
            }
            self->pg->removeAnnot(annot);
        }
        else
        {
            break;
        }
    }

    info.GetReturnValue().Set(Nan::Null());
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
NAN_METHOD(NodePopplerPage::addAnnot)
{
    Nan::HandleScope scope;
    NodePopplerPage *self = Nan::ObjectWrap::Unwrap<NodePopplerPage>(info.Holder());

    if (self->isDocClosed())
    {
        return Nan::ThrowError("Document closed. You must delete this page");
    }

    char *error = NULL;

    if (info.Length() < 1)
    {
        return Nan::ThrowError("One argument required: (annot: Object | Array).");
    }

    if (info[0]->IsArray())
    {
        if (Local<v8::Array>::Cast(info[0])->Length() > 0)
        {
            self->addAnnot(Local<v8::Array>::Cast(info[0]), &error);
        }
    }
    else if (info[0]->IsObject())
    {
        Local<v8::Array> annot = Nan::New<v8::Array>(1);
        Nan::Set(annot, 0, info[0]);
        self->addAnnot(annot, &error);
    }

    if (error)
    {
        Local<Value> e = Nan::Error(error);
        delete[] error;
        return Nan::ThrowError(e);
    }
    else
    {
        info.GetReturnValue().Set(Nan::Null());
    }
}

/**
     * Add annotations to page
     */
void NodePopplerPage::addAnnot(const Local<v8::Array> v8array, char **error)
{
    Nan::HandleScope scope;

    double x1, y1, x2, y2, x3, y3, x4, y4;
    int len = v8array->Length();
    ::Array *array = new ::Array(doc->getXRef());
    for (int i = 0; i < len; i++)
    {
        parseAnnot(Nan::Get(v8array, i).ToLocalChecked(), &x1, &y1, &x2, &y2, &x3, &y3, &x4, &y4, error);
        if (*error)
        {
            delete array;
            return;
        }
        else
        {
#if ((POPPLER_VERSION_MAJOR == 0) && (POPPLER_VERSION_MINOR <= 57))
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
#if POPPLER_VERSION_MAJOR == 0 && (POPPLER_VERSION_MINOR < 23 || (POPPLER_VERSION_MINOR == 23 && POPPLER_VERSION_MICRO < 3))
    AnnotTextMarkup *annot = new AnnotTextMarkup(doc, rect, Annot::typeHighlight, aq);
#else
    AnnotTextMarkup *annot = new AnnotTextMarkup(doc, rect, Annot::typeHighlight);
    annot->setQuadrilaterals(aq);
#endif

    annot->setOpacity(.5);
#if POPPLER_VERSION_MAJOR == 0 && POPPLER_VERSION_MINOR < 70
    annot->setColor(new AnnotColor(color_r, color_g, color_b));
#else
    auto new_color = std::unique_ptr<AnnotColor>(new AnnotColor(color_r, color_g, color_b));
    annot->setColor(std::move(new_color));
#endif
    pg->addAnnot(annot);

    delete array;
    delete rect;
    delete aq;
}

/**
     * Parse annotation quadrilateral
     */
void NodePopplerPage::parseAnnot(const Local<Value> rect,
                                 double *x1, double *y1,
                                 double *x2, double *y2,
                                 double *x3, double *y3,
                                 double *x4, double *y4,
                                 char **error)
{
    Nan::HandleScope scope;

    Local<v8::Object> rect_obj;
    Local<Value> x1v;
    Local<Value> x2v;
    Local<Value> y1v;
    Local<Value> y2v;

    if (!To<v8::Object>(rect).ToLocal(&rect_obj)
          || !Nan::Get(rect_obj, Nan::New("x1").ToLocalChecked()).ToLocal(&x1v)
          || !Nan::Get(rect_obj, Nan::New("x2").ToLocalChecked()).ToLocal(&x2v)
          || !Nan::Get(rect_obj, Nan::New("y1").ToLocalChecked()).ToLocal(&y1v)
          || !Nan::Get(rect_obj, Nan::New("y2").ToLocalChecked()).ToLocal(&y2v)) {
        char *e = (char *)"Invalid rectangle definition for annotation quadrilateral";
        *error = new char[strlen(e) + 1];
        strcpy(*error, e);
        return;
    }

    if (!x1v->IsNumber() || !x2v->IsNumber() || !y1v->IsNumber() || !y2v->IsNumber())
    {
        char *e = (char *)"Wrong values for rectangle corners definition";
        *error = new char[strlen(e) + 1];
        strcpy(*error, e);
        return;
    }

    int rotation = getRotate();

    switch (rotation)
    {
    case 90:
        *x1 = *x2 = pg->getCropWidth() * (1 - To<double>(y1v).FromJust());
        *x3 = *x4 = pg->getCropWidth() * (1 - To<double>(y2v).FromJust());
        *y2 = *y4 = pg->getCropHeight() * To<double>(x2v).FromJust();
        *y1 = *y3 = pg->getCropHeight() * To<double>(x1v).FromJust();
        break;
    case 180:
        *x1 = *x2 = pg->getCropWidth() * (1 - To<double>(x2v).FromJust());
        *x3 = *x4 = pg->getCropWidth() * (1 - To<double>(x1v).FromJust());
        *y2 = *y4 = pg->getCropHeight() * (1 - To<double>(y2v).FromJust());
        *y1 = *y3 = pg->getCropHeight() * (1 - To<double>(y1v).FromJust());
        break;
    case 270:
        *x1 = *x2 = pg->getCropWidth() * (To<double>(y1v).FromJust());
        *x3 = *x4 = pg->getCropWidth() * (To<double>(y2v).FromJust());
        *y2 = *y4 = pg->getCropHeight() * (1 - To<double>(x2v).FromJust());
        *y1 = *y3 = pg->getCropHeight() * (1 - To<double>(x1v).FromJust());
        break;
    default:
        *x1 = *x2 = pg->getCropWidth() * To<double>(x1v).FromJust();
        *x3 = *x4 = pg->getCropWidth() * To<double>(x2v).FromJust();
        *y2 = *y4 = pg->getCropHeight() * To<double>(y1v).FromJust();
        *y1 = *y3 = pg->getCropHeight() * To<double>(y2v).FromJust();
        break;
    }
}

/**
     * Displaying page slice to stream work->f
     */
void NodePopplerPage::display(RenderWork *work)
{
    SplashColor paperColor;
    paperColor[0] = 255;
    paperColor[1] = 255;
    paperColor[2] = 255;
    SplashOutputDev *splashOut = new SplashOutputDev(
        splashModeRGB8,
        4, false,
        paperColor);
    splashOut->startDoc(work->self->doc);
    ImgWriter *writer = NULL;
    switch (work->w)
    {
    case W_PNG:
        writer = new PNGWriter();
        break;
    case W_JPEG:
        writer = new JpegWriter(work->quality, work->progressive);
        break;
    case W_TIFF:
        writer = new TiffWriter(TiffWriter::RGB);
        if (work->compression != NULL)
        {
            ((TiffWriter *)writer)->setCompressionString(work->compression);
        }
    }
    int sx, sy, sw, sh;
    std::tie(sx, sy, sw, sh) = work->applyScale();
    if (work->error)
        return;
    work->self->pg->displaySlice(splashOut, work->PPI, work->PPI,
                                 0, false, true,
                                 sx, sy, sw, sh,
                                 false);

    SplashBitmap *bitmap = splashOut->getBitmap();
#if POPPLER_VERSION_MAJOR > 0 || (POPPLER_VERSION_MAJOR == 0 && POPPLER_VERSION_MINOR > 49)
    SplashError e = bitmap->writeImgFile(writer, work->f, (int)work->PPI, (int)work->PPI, splashModeRGB8);
#else
    SplashError e = bitmap->writeImgFile(writer, work->f, (int)work->PPI, (int)work->PPI);
#endif
    delete splashOut;
    if (writer != NULL)
        delete writer;

    if (e
    )
    {
        char err[256];
        sprintf(err, "SplashError %d", e);
        work->error = new char[strlen(err) + 1];
        strcpy(work->error, err);
    }
}

/**
     * Renders page to a file stream
     *
     * Backend function for \see NodePopplerPage::renderToBuffer and \see NodePopplerPage::renderToFile
     */
void NodePopplerPage::renderToStream(RenderWork *work)
{
    if (work->callback == NULL)
    {
        display(work);
    }
    else
    {
        uv_queue_work(uv_default_loop(), &work->request, AsyncRenderWork, AsyncRenderAfter);
    }
}

void NodePopplerPage::AsyncRenderWork(uv_work_t *req)
{
    RenderWork *work = static_cast<RenderWork *>(req->data);
    display(work);
}

void NodePopplerPage::AsyncRenderAfter(uv_work_t *req, int status)
{
    Nan::HandleScope scope;
    RenderWork *work = static_cast<RenderWork *>(req->data);

    work->closeStream();

    if (work->error)
    {
        Local<Value> err = Nan::Error(work->error);
        Local<Value> argv[] = {err};
        Nan::TryCatch try_catch;
        Nan::Call(*work->callback, 1, argv);
        if (try_catch.HasCaught())
        {
            Nan::FatalException(try_catch);
        }
    }
    else
    {
        switch (work->dest)
        {
        case DEST_FILE:
        {
            Local<v8::Object> out = Nan::New<v8::Object>();
            Nan::Set(out, Nan::New("type").ToLocalChecked(), Nan::New("file").ToLocalChecked());
            Nan::Set(out, Nan::New("path").ToLocalChecked(), Nan::New(work->filename).ToLocalChecked());
            Local<Value> argv[] = {Nan::Null(), out};
            Nan::TryCatch try_catch;
            Nan::AsyncResource res(Nan::New("poppler-simple::render-to-file").ToLocalChecked());
            work->callback->Call(2, argv, &res);
            if (try_catch.HasCaught())
            {
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
            Nan::Set(out, Nan::New("type").ToLocalChecked(), Nan::New("buffer").ToLocalChecked());
            Nan::Set(out, Nan::New("format").ToLocalChecked(), Nan::New(work->format).ToLocalChecked());
            Nan::Set(out, Nan::New("data").ToLocalChecked(), buffer);
            Local<Value> argv[] = {Nan::Null(), out};
            Nan::TryCatch try_catch;
            Nan::AsyncResource res(Nan::New("poppler-simple::render-to-buffer").ToLocalChecked());
            work->callback->Call(2, argv, &res);
            if (try_catch.HasCaught())
            {
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
NAN_METHOD(NodePopplerPage::renderToBuffer)
{
    Nan::HandleScope scope;
    NodePopplerPage *self = Nan::ObjectWrap::Unwrap<NodePopplerPage>(info.Holder());
    RenderWork *work = new RenderWork(self, DEST_BUFFER);

    if (info.Length() < 2 || !info[0]->IsString())
    {
        delete work;
        return Nan::ThrowError("Arguments: (method: String, PPI: Number[, options: Object, callback: Function]");
    }

    if (info[info.Length() - 1]->IsFunction())
    {
        Local<v8::Function> callbackHandle = info[info.Length() - 1].As<v8::Function>();
        work->callback = new Nan::Callback(callbackHandle);
    }

    if (self->isDocClosed())
    {
        Local<Value> err = Nan::Error("Document closed. You must delete this page");
        THROW_SYNC_ASYNC_ERR(work, err);
    }

    work->setWriter(info[0]);
    if (work->error)
    {
        Local<Value> err = Nan::Error(work->error);
        THROW_SYNC_ASYNC_ERR(work, err);
    }

    work->setPPI(info[1]);
    if (work->error)
    {
        Local<Value> err = Nan::Error(work->error);
        THROW_SYNC_ASYNC_ERR(work, err);
    }

    if (info.Length() > 2 && info[2]->IsObject())
    {
        work->setWriterOptions(info[2]);
        if (work->error)
        {
            Local<Value> err = Nan::Error(work->error);
            THROW_SYNC_ASYNC_ERR(work, err);
        }
    }

    work->openStream();
    if (work->error)
    {
        Local<Value> err = Nan::Error(work->error);
        THROW_SYNC_ASYNC_ERR(work, err);
    }

    self->renderToStream(work);
    if (work->callback != NULL)
    {
        return;
    }
    else
    {
        work->closeStream();

        if (work->error)
        {
            Local<Value> e = Nan::Error(work->error);
            delete work;
            return Nan::ThrowError(e);
        }
        else
        {
            Local<v8::Object> buffer = Nan::NewBuffer(work->mstrm_len).ToLocalChecked();
            Local<v8::Object> out = Nan::New<v8::Object>();

            memcpy(Buffer::Data(buffer), work->mstrm_buf, work->mstrm_len);

            Nan::Set(out, Nan::New("type").ToLocalChecked(), Nan::New("buffer").ToLocalChecked());
            Nan::Set(out, Nan::New("format").ToLocalChecked(), info[0]);
            Nan::Set(out, Nan::New("data").ToLocalChecked(), buffer);

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
NAN_METHOD(NodePopplerPage::renderToFile)
{
    Nan::HandleScope scope;
    NodePopplerPage *self = Nan::ObjectWrap::Unwrap<NodePopplerPage>(info.Holder());
    RenderWork *work = new RenderWork(self, DEST_FILE);

    if (info.Length() < 3)
    {
        Local<Value> err = Nan::Error(
            "Arguments: (path: String, method: String, PPI: Number[, options: Object, callback: Function])");
        THROW_SYNC_ASYNC_ERR(work, err);
    }

    if (info[info.Length() - 1]->IsFunction())
    {
        Local<v8::Function> callbackHandle = info[info.Length() - 1].As<v8::Function>();
        work->callback = new Nan::Callback(callbackHandle);
    }

    if (self->isDocClosed())
    {
        Local<Value> err = Nan::Error("Document closed. You must delete this page");
        THROW_SYNC_ASYNC_ERR(work, err);
    }

    work->setPath(info[0]);
    if (work->error)
    {
        Local<Value> err = Nan::Error(work->error);
        THROW_SYNC_ASYNC_ERR(work, err);
    }

    work->setWriter(info[1]);
    if (work->error)
    {
        Local<Value> err = Nan::Error(work->error);
        THROW_SYNC_ASYNC_ERR(work, err);
    }

    work->setPPI(info[2]);
    if (work->error)
    {
        Local<Value> err = Nan::Error(work->error);
        THROW_SYNC_ASYNC_ERR(work, err);
    }

    if (info.Length() > 3 && info[3]->IsObject())
    {
        work->setWriterOptions(info[3]);
        if (work->error)
        {
            Local<Value> err = Nan::Error(work->error);
            THROW_SYNC_ASYNC_ERR(work, err);
        }
    }

    work->openStream();
    if (work->error)
    {
        Local<Value> err = Nan::Error(work->error);
        THROW_SYNC_ASYNC_ERR(work, err);
    }

    self->renderToStream(work);
    if (work->callback != NULL)
    {
        return;
    }
    else
    {
        work->closeStream();
        if (work->error)
        {
            Local<Value> e = Nan::Error(work->error);
            unlink(work->filename);
            delete work;
            return Nan::ThrowError(e);
        }
        else
        {
            Local<v8::Object> out = Nan::New<v8::Object>();
            Nan::Set(out, Nan::New("type").ToLocalChecked(), Nan::New("file").ToLocalChecked());
            Nan::Set(out, Nan::New("path").ToLocalChecked(), Nan::New(work->filename).ToLocalChecked());
            delete work;
            info.GetReturnValue().Set(out);
        }
    }
}

void NodePopplerPage::RenderWork::setWriter(const Local<Value> method)
{
    Nan::HandleScope scope;
    char *e = NULL;
    Nan::Utf8String m(To<String>(method).ToLocalChecked());
    if (m.length() > 0)
    {
        if (strncmp(*m, "png", 3) == 0)
        {
            this->w = W_PNG;
        }
        else if (strncmp(*m, "jpeg", 4) == 0)
        {
            this->w = W_JPEG;
        }
        else if (strncmp(*m, "tiff", 4) == 0)
        {
            this->w = W_TIFF;
        }
        else
        {
            e = (char *)"Unsupported compression method";
        }
    }
    else
    {
        e = (char *)"'method' must be an instance of String";
    }
    if (e)
    {
        this->error = new char[strlen(e) + 1];
        strcpy(this->error, e);
    }
    else
    {
        strcpy(this->format, *m);
    }
}

void NodePopplerPage::RenderWork::setWriterOptions(const Local<Value> optsVal)
{
    Nan::HandleScope scope;

    Local<String> ck = Nan::New("compression").ToLocalChecked();
    Local<String> qk = Nan::New("quality").ToLocalChecked();
    Local<String> pk = Nan::New("progressive").ToLocalChecked();
    Local<String> sk = Nan::New("slice").ToLocalChecked();
    Local<v8::Object> options;
    char *e = NULL;

    if (!optsVal->IsObject())
    {
        e = (char *)"'options' must be an instance of Object";
    }
    else
    {
        options = To<v8::Object>(optsVal).ToLocalChecked();
        switch (this->w)
        {
        case W_TIFF:
        {
            if (Nan::Has(options, ck).FromMaybe(false))
            {
                Local<Value> cv = Nan::Get(options, ck).ToLocalChecked();
                if (cv->IsString())
                {
                    Local<String> cmp = To<String>(cv).ToLocalChecked();
                    Nan::Utf8String cmp_utf8(cmp);
                    int32_t cmp_utf8_len = cmp_utf8.length();
                    if (cmp_utf8_len > 0)
                    {
                        this->compression = new char[cmp_utf8_len + 1];
                        memcpy(this->compression, *cmp_utf8, cmp_utf8_len);
                        this->compression[cmp_utf8_len] = 0;
                    }
                    else
                    {
                        e = (char *)"'compression' option value could not be an empty string";
                    }
                }
                else
                {
                    e = (char *)"'compression' option must be an instance of string";
                }
            }
        }
        break;
        case W_JPEG:
        {
            if (Nan::Has(options, qk).FromMaybe(false))
            {
                Local<Value> qv = Nan::Get(options, qk).ToLocalChecked();
                if (qv->IsUint32())
                {
                    this->quality = To<int32_t>(qv).FromJust();
                    if (0 > this->quality || this->quality > 100)
                    {
                        e = (char *)"'quality' not in 0 - 100 interval";
                    }
                }
                else
                {
                    e = (char *)"'quality' option value must be 0 - 100 interval integer";
                }
            }
            if (Nan::Has(options, pk).FromMaybe(false))
            {
                Local<Value> pv = Nan::Get(options, pk).ToLocalChecked();
                if (pv->IsBoolean())
                {
                    this->progressive = To<bool>(pv).FromJust();
                }
                else
                {
                    e = (char *)"'progressive' option value must be a boolean value";
                }
            }
        }
        break;
        case W_PNG:
            break;
        }
        if (Nan::Has(options, sk).FromMaybe(false))
        {
            this->setSlice(Nan::Get(options, sk).ToLocalChecked());
        }
        else
        {
            // Injecting fake slice to render whole page
            Local<v8::Object> slice = Nan::New<v8::Object>();
            Nan::Set(slice, Nan::New("x").ToLocalChecked(), Nan::New<Number>(0));
            Nan::Set(slice, Nan::New("y").ToLocalChecked(), Nan::New<Number>(0));
            Nan::Set(slice, Nan::New("w").ToLocalChecked(), Nan::New<Number>(1));
            Nan::Set(slice, Nan::New("h").ToLocalChecked(), Nan::New<Number>(1));
            this->setSlice(slice);
        }
    }
    if (e)
    {
        this->error = new char[strlen(e) + 1];
        strcpy(this->error, e);
    }
}

void NodePopplerPage::RenderWork::setPPI(const Local<Value> PPI)
{
    Nan::HandleScope scope;
    char *e = NULL;
    if (PPI->IsNumber())
    {
        double ppi;
        ppi = To<double>(PPI).FromJust();
        if (0 > ppi)
        {
            e = (char *)"'PPI' value must be greater then 0";
        }
        else
        {
            this->PPI = ppi;
        }
    }
    else
    {
        e = (char *)"'PPI' must be an instance of number";
    }
    if (e)
    {
        this->error = new char[strlen(e) + 1];
        strcpy(this->error, e);
    }
}

void NodePopplerPage::RenderWork::setPath(const Local<Value> path)
{
    Nan::HandleScope scope;
    char *e = NULL;
    if (path->IsString())
    {
        Local<String> path_str = To<String>(path).ToLocalChecked();
        Nan::Utf8String path_str_utf8(path_str);
        int32_t path_len = path_str_utf8.length();
        if (path_len > 0)
        {
            this->filename = new char[path_len + 1];
            memcpy(this->filename, *path_str_utf8, path_len);
            this->filename[path_len] = 0;
        }
        else
        {
            e = (char *)"'path' can't be empty";
        }
    }
    else
    {
        e = (char *)"'path' must be an instance of string";
    }
    if (e)
    {
        this->error = new char[strlen(e) + 1];
        strcpy(this->error, e);
    }
}

std::tuple<int, int, int, int> NodePopplerPage::RenderWork::applyScale()
{
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
        e = (char *)"Result image is too big";
    }
    if (e)
    {
        this->error = new char[strlen(e) + 1];
        strcpy(this->error, e);
    }
    return std::make_tuple(scaled_x, scaled_y, scaled_w, scaled_h);
}

void NodePopplerPage::RenderWork::setSlice(const Local<Value> sliceVal)
{
    Nan::HandleScope scope;
    char *e = NULL;

    Local<v8::Object> slice;

    Local<String> xk = Nan::New("x").ToLocalChecked();
    Local<String> yk = Nan::New("y").ToLocalChecked();
    Local<String> wk = Nan::New("w").ToLocalChecked();
    Local<String> hk = Nan::New("h").ToLocalChecked();

    if (!To<v8::Object>(sliceVal).ToLocal(&slice)) {
        e = (char *)"'slice' option value must be an instance of Object";
    } else {
        Local<Value> xv, yv, hv, wv;

        if (!Nan::Get(slice, xk).ToLocal(&xv)
              || !Nan::Get(slice, yk).ToLocal(&yv)
              || !Nan::Get(slice, wk).ToLocal(&wv)
              || !Nan::Get(slice, hk).ToLocal(&hv)) {
            e = (char *)"Slice must be an object: {x: Number, y: Number, w: Number, h: Number}";
        } else {
            if (xv->IsNumber() && yv->IsNumber() && wv->IsNumber() && hv->IsNumber())
            {
                double x, y, w, h;
                x = To<double>(xv).FromJust();
                y = To<double>(yv).FromJust();
                w = To<double>(wv).FromJust();
                h = To<double>(hv).FromJust();
                if (
                    (0.0 > x || x > 1.0) ||
                    (0.0 > y || y > 1.0) ||
                    (0.0 > w || w > 1.0) ||
                    (0.0 > h || h > 1.0))
                {
                    e = (char *)"Slice values must be 0 - 1 interval numbers";
                }
                else
                {
                    // cap width and height to fit page size
                    if (y + h > 1.0)
                    {
                        h = 1.0 - y;
                    }
                    if (x + w > 1.0)
                    {
                        w = 1.0 - x;
                    }
                    this->slice_x = x;
                    this->slice_y = y;
                    this->slice_h = h;
                    this->slice_w = w;
                }
            }
            else
            {
                e = (char *)"Slice must be an object: {x: Number, y: Number, w: Number, h: Number}";
            }
        }
    }
    if (e)
    {
        this->error = new char[strlen(e) + 1];
        strcpy(this->error, e);
    }
}

/**
     * Opens output stream for rendering
     */
void NodePopplerPage::RenderWork::openStream()
{
    char *e = NULL;
    switch (this->dest)
    {
    case DEST_FILE:
    {
        if (this->filename)
        {
            this->f = fopen(this->filename, "wb");
        }
        else
        {
            e = (char *)"Output file name was not set";
        }
    }
    break;
    case DEST_BUFFER:
    {
        if (this->w != W_TIFF)
        {
            this->stream = new MemoryStream();
            this->f = this->stream->open();
        }
        else
        {
            this->filename = new char[17];
            strcpy(this->filename, "/tmp/psmplXXXXXX");
            int fd = mkstemp(this->filename);
            if (fd != -1)
            {
                this->f = fdopen(fd, "wb");
            }
        }
    }
    }
    if (!this->f)
    {
        e = (char *)"Could not open output stream";
    }
    if (e)
    {
        this->error = new char[strlen(e) + 1];
        strcpy(this->error, e);
    }
}

/**
     * Closes output stream
     */
void NodePopplerPage::RenderWork::closeStream()
{
    switch (this->dest)
    {
    case DEST_FILE:
        fclose(this->f);
        this->f = NULL;
        break;
    case DEST_BUFFER:
    {
        if (this->w != W_TIFF)
        {
            fclose(this->f);
            this->f = NULL;
            this->mstrm_len = this->stream->getBufferLen();
            this->mstrm_buf = this->stream->giveBuffer();
        }
        else
        {
            struct stat s;
            int filedes;
            filedes = open(this->filename, O_RDONLY);
            fstat(filedes, &s);
            if (s.st_size > 0)
            {
                this->mstrm_len = s.st_size;
                this->mstrm_buf = (char *)malloc(this->mstrm_len);
                ssize_t count = read(filedes, this->mstrm_buf, this->mstrm_len);
                if (count != (ssize_t)this->mstrm_len && this->error == NULL)
                {
                    char err[256];
                    sprintf(err, "Can't read temporary file");
                    this->error = new char[strlen(err) + 1];
                    strcpy(this->error, err);
                }
            }
            close(filedes);
            fclose(this->f);
            unlink(this->filename);
            this->f = NULL;
        }
    }
    }
}
} // namespace node

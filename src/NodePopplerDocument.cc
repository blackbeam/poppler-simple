#include <v8.h>
#include <node.h>
#include <node_buffer.h>

#include "NodePopplerDocument.h"
#include "NodePopplerPage.h"

PDFDoc *createMemPDFDoc(
    char *buffer,
    size_t length,
    GooString* ownerPassword = nullptr,
    GooString* userPassword = nullptr)
{
    Object obj;

#if ((POPPLER_VERSION_MAJOR == 0) && (POPPLER_VERSION_MINOR <= 57))
    obj.initNull();
    return new PDFDoc(new MemStream(buffer, 0, length, &obj), ownerPassword, userPassword);
#else
    return new PDFDoc(new MemStream(buffer, 0, length, std::move(obj)), ownerPassword, userPassword);
#endif
}

using namespace v8;
using namespace node;
using Nan::To;

namespace node
{
void NodePopplerDocument::evPageOpened(NodePopplerPage *p)
{
    for (NodePopplerPage* kp : pages) {
        if (p == kp) return;
    }
    pages.push_back(p);
}

void NodePopplerDocument::evPageClosed(NodePopplerPage *p)
{
    for (unsigned int i = 0; i < pages.size(); i++)
    {
        if (p == pages[i])
        {
            pages.erase(pages.begin() + i);
            break;
        }
    }
}

NodePopplerDocument::NodePopplerDocument(
    const char *cFileName,
    GooString* ownerPassword,
    GooString* userPassword)
{
    doc = NULL;
    buffer = NULL;

    GooString *fileNameA = new GooString(cFileName);

    doc = PDFDocFactory().createPDFDoc(*fileNameA, ownerPassword, userPassword);

    pages = std::vector<NodePopplerPage*>();
}

NodePopplerDocument::NodePopplerDocument(
    char *buffer,
    size_t length,
    GooString* ownerPassword,
    GooString* userPassword)
{
    doc = NULL;
    this->buffer = NULL;
    this->buffer = new char[length];
    std::memcpy(this->buffer, buffer, length);
    doc = createMemPDFDoc(this->buffer, length, ownerPassword, userPassword);
    pages = std::vector<NodePopplerPage*>();
}

NodePopplerDocument::~NodePopplerDocument()
{
    for (NodePopplerPage* p : pages) {
        p->evDocumentClosed();
    }

    if (doc)
        delete doc;
    if (buffer)
        delete buffer;
}

NAN_MODULE_INIT(NodePopplerDocument::Init)
{
    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(NodePopplerDocument::New);
    tpl->SetClassName(Nan::New<String>("PopplerDocument").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    NODE_DEFINE_CONSTANT(target, POPPLER_VERSION_MAJOR);
    NODE_DEFINE_CONSTANT(target, POPPLER_VERSION_MINOR);
    NODE_DEFINE_CONSTANT(target, POPPLER_VERSION_MICRO);

    Nan::SetAccessor(tpl->InstanceTemplate(),
                     Nan::New<String>("pageCount").ToLocalChecked(),
                     NodePopplerDocument::paramsGetter);
    Nan::SetAccessor(tpl->InstanceTemplate(),
                     Nan::New<String>("PDFMajorVersion").ToLocalChecked(),
                     NodePopplerDocument::paramsGetter);
    Nan::SetAccessor(tpl->InstanceTemplate(),
                     Nan::New<String>("PDFMinorVersion").ToLocalChecked(),
                     NodePopplerDocument::paramsGetter);
    Nan::SetAccessor(tpl->InstanceTemplate(),
                     Nan::New<String>("pdfVersion").ToLocalChecked(),
                     NodePopplerDocument::paramsGetter);
    Nan::SetAccessor(tpl->InstanceTemplate(),
                     Nan::New<String>("isEncrypted").ToLocalChecked(),
                     NodePopplerDocument::paramsGetter);
    Nan::SetAccessor(tpl->InstanceTemplate(),
                     Nan::New<String>("isLinearized").ToLocalChecked(),
                     NodePopplerDocument::paramsGetter);
    Nan::SetAccessor(tpl->InstanceTemplate(),
                     Nan::New<String>("fileName").ToLocalChecked(),
                     NodePopplerDocument::paramsGetter);

    Nan::Set(target,
        Nan::New<String>("PopplerDocument").ToLocalChecked(),
        Nan::GetFunction(tpl).ToLocalChecked());
}

NAN_GETTER(NodePopplerDocument::paramsGetter)
{
    Nan::Utf8String propName(property);
    NodePopplerDocument *self = Nan::ObjectWrap::Unwrap<NodePopplerDocument>(info.This());

    if (strcmp(*propName, "pageCount") == 0)
    {
        info.GetReturnValue().Set(Nan::New<Uint32>(self->doc->getNumPages()));
    }
    else if (strcmp(*propName, "PDFMajorVersion") == 0)
    {
        info.GetReturnValue().Set(Nan::New<Uint32>(self->doc->getPDFMajorVersion()));
    }
    else if (strcmp(*propName, "PDFMinorVersion") == 0)
    {
        info.GetReturnValue().Set(Nan::New<Uint32>(self->doc->getPDFMinorVersion()));
    }
    else if (strcmp(*propName, "pdfVersion") == 0)
    {
        char versionString[16];
        sprintf(versionString, "PDF-%d.%d",
                self->doc->getPDFMajorVersion(),
                self->doc->getPDFMinorVersion());
        info.GetReturnValue().Set(Nan::New<String>(versionString).ToLocalChecked());
    }
    else if (strcmp(*propName, "isEncrypted") == 0)
    {
        info.GetReturnValue().Set(Nan::New<Boolean>(self->doc->isEncrypted()));
    }
    else if (strcmp(*propName, "isLinearized") == 0)
    {
        info.GetReturnValue().Set(Nan::New<Boolean>(self->doc->isLinearized()));
    }
    else if (strcmp(*propName, "fileName") == 0)
    {
        auto fileName = self->doc->getFileName();
        if (fileName != NULL)
        {
#if POPPLER_VERSION_MAJOR == 0 && POPPLER_VERSION_MINOR < 72
            auto c_str = fileName->getCString();
#else
            auto c_str = fileName->c_str();
#endif
            info.GetReturnValue().Set(Nan::New<String>(c_str, fileName->getLength())
                                          .ToLocalChecked());
        }
        else
        {
            info.GetReturnValue().Set(Nan::Null());
        }
    }
    else
    {
        info.GetReturnValue().Set(Nan::Null());
    }
}

NAN_METHOD(NodePopplerDocument::New)
{
    Nan::HandleScope scope;

    if (
        !(0 < info.Length() && info.Length() <= 3)
        || !(info[0]->IsString() || Buffer::HasInstance(info[0]))
        || !(info[1]->IsUndefined() || info[1]->IsNull() || info[1]->IsString())
        || !(info[2]->IsUndefined() || info[2]->IsNull() || info[2]->IsString()))
    {
        return Nan::ThrowError("Supported arguments: (fileName: string | Buffer, userPassword?: string, ownerPassword?: string).");
    }

    NodePopplerDocument *doc;

    GooString* userPassword = nullptr;
    GooString* ownerPassword = nullptr;

    if (info[1]->IsString()) {
        Nan::Utf8String jsUserPassword(To<String>(info[1]).ToLocalChecked());
        userPassword = new GooString(*jsUserPassword);
    }

    if (info[2]->IsString()) {
        Nan::Utf8String jsOwnerPassword(To<String>(info[2]).ToLocalChecked());
        ownerPassword = new GooString(*jsOwnerPassword);
    }

    if (info[0]->IsString())
    {
        Nan::Utf8String str(To<String>(info[0]).ToLocalChecked());
        doc = new NodePopplerDocument(*str, ownerPassword, userPassword);
    }
    else if (Buffer::HasInstance(info[0]))
    {
        doc = new NodePopplerDocument(
            Buffer::Data(info[0]),
            Buffer::Length(info[0]),
            userPassword,
            ownerPassword);
    }
    else
    {
        return Nan::ThrowTypeError("'filename' must be an instance of String or Buffer.");
    }

    if (!doc->isOk())
    {
        int errorCode = doc->getDoc()->getErrorCode();
        char errorName[128];
        char errorDescription[256];
        switch (errorCode)
        {
        case errOpenFile:
            sprintf(errorName, "fopen error. Errno: %d", doc->getDoc()->getFopenErrno());
            break;
        case errBadCatalog:
            sprintf(errorName, "bad catalog");
            break;
        case errDamaged:
            sprintf(errorName, "damaged");
            break;
        case errEncrypted:
            sprintf(errorName, "encrypted");
            break;
        case errHighlightFile:
            sprintf(errorName, "highlight file");
            break;
        case errBadPrinter:
            sprintf(errorName, "bad printer");
            break;
        case errPrinting:
            sprintf(errorName, "printing error");
            break;
        case errPermission:
            sprintf(errorName, "permission error");
            break;
        case errBadPageNum:
            sprintf(errorName, "bad page num");
            break;
        case errFileIO:
            sprintf(errorName, "file IO error");
            break;
        default:
            sprintf(errorName, "other error");
        }
        sprintf(errorDescription, "Couldn't open file - %s.", errorName);
        delete doc;
        return Nan::ThrowError(Exception::Error(Nan::New<String>(errorDescription, strlen(errorDescription)).ToLocalChecked()));
    }
    doc->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
}

} // namespace node

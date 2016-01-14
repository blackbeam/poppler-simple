#include <v8.h>
#include <node.h>
#include <node_buffer.h>

#include "NodePopplerDocument.h"
#include "NodePopplerPage.h"


PDFDoc *createMemPDFDoc( char* buffer, size_t length ){
    Object obj;
    obj.initNull();
    return new PDFDoc(new MemStream(buffer, 0, length, &obj), NULL, NULL);
}

using namespace v8;
using namespace node;

namespace node {
    void NodePopplerDocument::evPageOpened(const NodePopplerPage *p) {
        for (int i = 0; i < pages->getLength(); i++) {
            if (p == (NodePopplerPage*) pages->get(i)) {
                return;
            }
        }
        pages->append((void*)p);
    }

    void NodePopplerDocument::evPageClosed(const NodePopplerPage *p) {
        for (int i = 0; i < pages->getLength(); i++) {
            if (p == (NodePopplerPage*) pages->get(i)) {
                pages->del(i);
            }
        }
    }

    NodePopplerDocument::NodePopplerDocument(const char* cFileName) {
        doc = NULL;
        GooString *fileNameA = new GooString(cFileName);
        doc = PDFDocFactory().createPDFDoc(*fileNameA, NULL, NULL);
        pages = new GooList();
    }

    NodePopplerDocument::NodePopplerDocument(char* buffer, size_t length) {
        doc = NULL;
        doc = createMemPDFDoc(buffer, length);
        pages = new GooList();
    }

    NodePopplerDocument::~NodePopplerDocument() {
        for (int i = 0; i < pages->getLength(); i++) {
            ((NodePopplerPage*)pages->get(i))->evDocumentClosed();
        }
        if (doc) delete doc;
        delete pages;
    }

    NAN_MODULE_INIT(NodePopplerDocument::Init) {
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
                         Nan::New<String>("isLinearized").ToLocalChecked(),
                         NodePopplerDocument::paramsGetter);
        Nan::SetAccessor(tpl->InstanceTemplate(),
                         Nan::New<String>("fileName").ToLocalChecked(),
                         NodePopplerDocument::paramsGetter);

        Nan::Set(target,
                 Nan::New<String>("PopplerDocument").ToLocalChecked(),
                 tpl->GetFunction());
    }

    NAN_GETTER(NodePopplerDocument::paramsGetter) {
        String::Utf8Value propName(property);
        NodePopplerDocument *self = Nan::ObjectWrap::Unwrap<NodePopplerDocument>(info.This());

        if (strcmp(*propName, "pageCount") == 0) {
            info.GetReturnValue().Set(Nan::New<Uint32>(self->doc->getNumPages()));

        } else if (strcmp(*propName, "PDFMajorVersion") == 0) {
            info.GetReturnValue().Set(Nan::New<Uint32>(self->doc->getPDFMajorVersion()));

        } else if (strcmp(*propName, "PDFMinorVersion") == 0) {
            info.GetReturnValue().Set(Nan::New<Uint32>(self->doc->getPDFMinorVersion()));

        } else if (strcmp(*propName, "pdfVersion") == 0) {
            char versionString[16];
            sprintf(versionString, "PDF-%d.%d",
                    self->doc->getPDFMajorVersion(),
                    self->doc->getPDFMinorVersion());
            info.GetReturnValue().Set(Nan::New<String>(versionString).ToLocalChecked());

        } else if (strcmp(*propName, "isLinearized") == 0) {
            info.GetReturnValue().Set(Nan::New<Boolean>(self->doc->isLinearized()));

        } else if (strcmp(*propName, "fileName") == 0) {
            GooString *fileName = self->doc->getFileName();
            if (fileName != NULL) {
                info.GetReturnValue().Set(
                    Nan::New<String>(fileName->getCString(),
                    fileName->getLength()).ToLocalChecked()
                );
            } else {
                info.GetReturnValue().Set(Nan::Null());
            }

        } else {
            info.GetReturnValue().Set(Nan::Null());
        }
    }

    NAN_METHOD(NodePopplerDocument::New) {
        Nan::HandleScope scope;

        if (info.Length() != 1) {
            return Nan::ThrowError("One argument required: (filename: String).");
        }

        NodePopplerDocument *doc;

        if (info[0]->IsString()) {
            String::Utf8Value str(info[0]);
            doc = new NodePopplerDocument(*str);
        } else if (Buffer::HasInstance(info[0])) {
            doc = new NodePopplerDocument(Buffer::Data(info[0]), Buffer::Length(info[0]));
        } else {
            return Nan::ThrowTypeError("'filename' must be an instance of String or Buffer.");
        }

        if (!doc->isOk()) {
            int errorCode = doc->getDoc()->getErrorCode();
            char errorName[128];
            char errorDescription[256];
            switch (errorCode) {
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

}

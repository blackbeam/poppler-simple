#include <v8.h>
#include <node.h>
#include <node_buffer.h>

#include "NodePopplerDocument.h"
#include "NodePopplerPage.h"

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

    NodePopplerDocument::~NodePopplerDocument() {
        for (int i = 0; i < pages->getLength(); i++) {
            ((NodePopplerPage*)pages->get(i))->evDocumentClosed();
        }
        if (doc) delete doc;
        delete pages;
    }

    void NodePopplerDocument::Init(v8::Local<v8::Object> exports) {
        Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(NodePopplerDocument::New);
        tpl->SetClassName(Nan::New<String>("PopplerDocument").ToLocalChecked());
        tpl->InstanceTemplate()->SetInternalFieldCount(1);

        NODE_DEFINE_CONSTANT(exports, POPPLER_VERSION_MAJOR);
        NODE_DEFINE_CONSTANT(exports, POPPLER_VERSION_MINOR);
        NODE_DEFINE_CONSTANT(exports, POPPLER_VERSION_MICRO);

        tpl->InstanceTemplate()->SetAccessor(Nan::New<String>("pageCount").ToLocalChecked(),
                                             NodePopplerDocument::paramsGetter);
        tpl->InstanceTemplate()->SetAccessor(Nan::New<String>("PDFMajorVersion").ToLocalChecked(),
                                             NodePopplerDocument::paramsGetter);
        tpl->InstanceTemplate()->SetAccessor(Nan::New<String>("PDFMinorVersion").ToLocalChecked(),
                                             NodePopplerDocument::paramsGetter);
        tpl->InstanceTemplate()->SetAccessor(Nan::New<String>("pdfVersion").ToLocalChecked(),
                                             NodePopplerDocument::paramsGetter);
        tpl->InstanceTemplate()->SetAccessor(Nan::New<String>("isLinearized").ToLocalChecked(),
                                             NodePopplerDocument::paramsGetter);
        tpl->InstanceTemplate()->SetAccessor(Nan::New<String>("fileName").ToLocalChecked(),
                                             NodePopplerDocument::paramsGetter);

        exports->Set(Nan::New("PopplerDocument").ToLocalChecked(), tpl->GetFunction());
    }

    V8_ACCESSOR_GETTER(NodePopplerDocument::paramsGetter) {
        Nan::HandleScope scope;
        String::Utf8Value propName(property);
        NodePopplerDocument *self = Nan::ObjectWrap::Unwrap<NodePopplerDocument>(info.This());

        if (strcmp(*propName, "pageCount") == 0) {
            V8_ACCESSOR_RETURN(Nan::New<Uint32>(self->doc->getNumPages()));

        } else if (strcmp(*propName, "PDFMajorVersion") == 0) {
            V8_ACCESSOR_RETURN(Nan::New<Uint32>(self->doc->getPDFMajorVersion()));

        } else if (strcmp(*propName, "PDFMinorVersion") == 0) {
            V8_ACCESSOR_RETURN(Nan::New<Uint32>(self->doc->getPDFMinorVersion()));

        } else if (strcmp(*propName, "pdfVersion") == 0) {
            char versionString[16];
            sprintf(versionString, "PDF-%d.%d", self->doc->getPDFMajorVersion(), self->doc->getPDFMinorVersion());
            V8_ACCESSOR_RETURN(Nan::New<String>(versionString, strlen(versionString)).ToLocalChecked());

        } else if (strcmp(*propName, "isLinearized") == 0) {
            V8_ACCESSOR_RETURN(Nan::New<Boolean>(self->doc->isLinearized()));

        } else if (strcmp(*propName, "fileName") == 0) {
            GooString *fileName = self->doc->getFileName();
            V8_ACCESSOR_RETURN(Nan::New<String>(fileName->getCString(), fileName->getLength()).ToLocalChecked());

        } else {
            V8_ACCESSOR_RETURN(Nan::Null());
        }
    }

    NAN_METHOD(NodePopplerDocument::New) {
        Nan::HandleScope scope;

        if(info.Length() != 1) {
            return Nan::ThrowError("One argument required: (filename: String).");
        }
        if(!info[0]->IsString()) {
            return Nan::ThrowTypeError("'filename' must be an instance of String.");
        }

        String::Utf8Value str(info[0]);
        NodePopplerDocument *doc = new NodePopplerDocument(*str);

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

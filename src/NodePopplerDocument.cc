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

    void NodePopplerDocument::Init(v8::Handle<v8::Object> exports) {
        Local<FunctionTemplate> tpl = FunctionTemplate::New(NodePopplerDocument::New);
        tpl->SetClassName(String::NewSymbol("PopplerDocument"));
        tpl->InstanceTemplate()->SetInternalFieldCount(1);

        NODE_DEFINE_CONSTANT(tpl, POPPLER_VERSION_MAJOR);
        NODE_DEFINE_CONSTANT(tpl, POPPLER_VERSION_MINOR);
        NODE_DEFINE_CONSTANT(tpl, POPPLER_VERSION_MICRO);
        
        NODE_DEFINE_CONSTANT(exports, POPPLER_VERSION_MAJOR);
        NODE_DEFINE_CONSTANT(exports, POPPLER_VERSION_MINOR);
        NODE_DEFINE_CONSTANT(exports, POPPLER_VERSION_MICRO);

        tpl->InstanceTemplate()->SetAccessor(String::NewSymbol("pageCount"),
                                             NodePopplerDocument::paramsGetter);
        tpl->InstanceTemplate()->SetAccessor(String::NewSymbol("PDFMajorVersion"),
                                             NodePopplerDocument::paramsGetter);
        tpl->InstanceTemplate()->SetAccessor(String::NewSymbol("PDFMinorVersion"),
                                             NodePopplerDocument::paramsGetter);
        tpl->InstanceTemplate()->SetAccessor(String::NewSymbol("pdfVersion"),
                                             NodePopplerDocument::paramsGetter);
        tpl->InstanceTemplate()->SetAccessor(String::NewSymbol("isLinearized"),
                                             NodePopplerDocument::paramsGetter);
        tpl->InstanceTemplate()->SetAccessor(String::NewSymbol("fileName"),
                                             NodePopplerDocument::paramsGetter);
        
        exports->Set(String::NewSymbol("PopplerDocument"), tpl->GetFunction());
    }

    V8_ACCESSOR_GETTER(NodePopplerDocument::paramsGetter) {
        CREATE_HANDLE_SCOPE;
        String::Utf8Value propName(property);
        NodePopplerDocument *self = ObjectWrap::Unwrap<NodePopplerDocument>(info.This());

        if (strcmp(*propName, "pageCount") == 0) {
            V8_ACCESSOR_RETURN(scope.Close(Uint32::New(self->doc->getNumPages())));

        } else if (strcmp(*propName, "PDFMajorVersion") == 0) {
            V8_ACCESSOR_RETURN(scope.Close(Uint32::New(self->doc->getPDFMajorVersion())));

        } else if (strcmp(*propName, "PDFMinorVersion") == 0) {
            V8_ACCESSOR_RETURN(scope.Close(Uint32::New(self->doc->getPDFMinorVersion())));

        } else if (strcmp(*propName, "pdfVersion") == 0) {
            char versionString[16];
            sprintf(versionString, "PDF-%d.%d", self->doc->getPDFMajorVersion(), self->doc->getPDFMinorVersion());
            V8_ACCESSOR_RETURN(scope.Close(String::New(versionString, strlen(versionString))));

        } else if (strcmp(*propName, "isLinearized") == 0) {
            V8_ACCESSOR_RETURN(scope.Close(Boolean::New(self->doc->isLinearized())));

        } else if (strcmp(*propName, "fileName") == 0) {
            GooString *fileName = self->doc->getFileName();
            V8_ACCESSOR_RETURN(scope.Close(String::New(fileName->getCString(), fileName->getLength())));

        } else {
            V8_ACCESSOR_RETURN(scope.Close(Null()));
        }
    }

    V8_METHOD(NodePopplerDocument::New) {
        CREATE_HANDLE_SCOPE;

        if(args.Length() != 1) {
            V8_THROW(Exception::Error(String::New("One argument required: (filename: String).")));
        }
        if(!args[0]->IsString()) {
            V8_THROW(Exception::TypeError(String::New("'filename' must be an instance of String.")));
        }

        String::Utf8Value str(args[0]);
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
            V8_THROW(Exception::Error(String::New(errorDescription, strlen(errorDescription))));
        }
        doc->wrap(args.This());
        V8_RETURN(args.This());
    }

}

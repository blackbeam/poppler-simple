#include <v8.h>
#include <node.h>
#include <node_buffer.h>

#include "NodePopplerDocument.h"
#include "NodePopplerPage.h"

using namespace v8;
using namespace node;

namespace node {
    Persistent<FunctionTemplate> NodePopplerDocument::constructor_template;

    void NodePopplerDocument::evPageOpened(NodePopplerPage *p) {
        for (int i = 0; i < pages->getLength(); i++) {
            if (p == (NodePopplerPage*) pages->get(i)) {
                return;
            }
        }
        pages->append((void*)p);
    }

    void NodePopplerDocument::evPageClosed(NodePopplerPage *p) {
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

    void NodePopplerDocument::Initialize(v8::Handle<v8::Object> target) {
        HandleScope scope;

        Local<FunctionTemplate> t = FunctionTemplate::New(New);
        constructor_template = Persistent<FunctionTemplate>::New(t);
        constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
        constructor_template->SetClassName(String::NewSymbol("PopplerDocument"));

        /** Instance methods
         * Prototype looks like this:
         *  static Handle<Value> funcName(const Arguments &args);
         * to access the object do:
         *  ClassName* self = ObjectWrap::Unwrap<ClassName>(args.Holder());
         * 
         * Then do:
         *  NODE_SET_PROTOTYPE_METHOD(constructor_template, "getPageCount", funcName);
         */

        /** Class methods
         * NODE_SET_METHOD(constructor_template->GetFunction(), "GetPageCount", funcName);
         */

        /** Getters:
         * Prototype looks like this:
         *  static Handle<Value> funcName(Local<String> property, const AccessorInfo& info);
         * to access the object do:
         *  ClassName* self = ObjectWrap::Unwrap<ClassName>(info.This());
         * 
         * Then do:
         *  constructor_template->PrototypeTemplate()->SetAccessor(String::NewSymbol("page_count"), funcName);
         */
        constructor_template->InstanceTemplate()->SetAccessor(String::NewSymbol("pageCount"), NodePopplerDocument::paramsGetter);
        constructor_template->InstanceTemplate()->SetAccessor(String::NewSymbol("PDFMajorVersion"), NodePopplerDocument::paramsGetter);
        constructor_template->InstanceTemplate()->SetAccessor(String::NewSymbol("PDFMinorVersion"), NodePopplerDocument::paramsGetter);
        constructor_template->InstanceTemplate()->SetAccessor(String::NewSymbol("pdfVersion"), NodePopplerDocument::paramsGetter);
        constructor_template->InstanceTemplate()->SetAccessor(String::NewSymbol("isLinearized"), NodePopplerDocument::paramsGetter);
        constructor_template->InstanceTemplate()->SetAccessor(String::NewSymbol("fileName"), NodePopplerDocument::paramsGetter);
        target->Set(String::NewSymbol("PopplerDocument"), constructor_template->GetFunction());
    }

    Handle<Value> NodePopplerDocument::paramsGetter(Local< String > property, const AccessorInfo &info) {
        HandleScope scope;
        String::Utf8Value propName(property);
        NodePopplerDocument *self = ObjectWrap::Unwrap<NodePopplerDocument>(info.This());

        if (strcmp(*propName, "pageCount") == 0) {
            return scope.Close(Uint32::New(self->doc->getNumPages()));

        } else if (strcmp(*propName, "PDFMajorVersion") == 0) {
            return scope.Close(Uint32::New(self->doc->getPDFMajorVersion()));

        } else if (strcmp(*propName, "PDFMinorVersion") == 0) {
            return scope.Close(Uint32::New(self->doc->getPDFMinorVersion()));

        } else if (strcmp(*propName, "pdfVersion") == 0) {
            char versionString[16];
            sprintf(versionString, "PDF-%d.%d", self->doc->getPDFMajorVersion(), self->doc->getPDFMinorVersion());
            return scope.Close(String::New(versionString, strlen(versionString)));

        } else if (strcmp(*propName, "isLinearized") == 0) {
            return scope.Close(Boolean::New(self->doc->isLinearized()));

        } else if (strcmp(*propName, "fileName") == 0) {
            GooString *fileName = self->doc->getFileName();
            return scope.Close(String::New(fileName->getCString(), fileName->getLength()));

        } else {
            return scope.Close(Null());
        }
    }

    Handle<Value> NodePopplerDocument::New(const Arguments &args) {
        HandleScope scope;
        if(args.Length() != 1) {
            return ThrowException(
                Exception::Error(String::New("One argument required: (filename: String).")));
        }
        if(!args[0]->IsString()) {
            return ThrowException(
                Exception::TypeError(String::New("'filename' must be an instance of String.")));
        }

        String::Utf8Value str(args[0]);
        NodePopplerDocument *doc = new NodePopplerDocument(*str);

        if (!doc->isOk()) {
            int errorCode = doc->doc->getErrorCode();
            char errorName[128];
            char errorDescription[256];
            switch (errorCode) {
                case errOpenFile:
                    sprintf(errorName, "fopen error. Errno: %d", doc->doc->getFopenErrno());
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
            return ThrowException(
                Exception::Error(String::New(errorDescription, strlen(errorDescription))));
        }
        doc->Wrap(args.This());
        return args.This();
    }

}

// Exporting function
extern "C" void
init (v8::Handle<v8::Object> target)
{
    HandleScope scope;

    // Require for poppler
    globalParams = new GlobalParams();

    NodePopplerDocument::Initialize(target);
    NodePopplerPage::Initialize(target);
}

NODE_MODULE(poppler, init);
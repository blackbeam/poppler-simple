#include <v8.h>
#include <node.h>
#include <nan.h>
#include "helpers.h"
#include <GlobalParams.h>
#include <cpp/poppler-version.h>
#include <poppler/PDFDoc.h>
#include <poppler/ErrorCodes.h>
#include <poppler/PDFDocFactory.h>
#include <goo/GooString.h>
#include <goo/GooList.h>

namespace node {
    class NodePopplerPage;
    class NodePopplerDocument : public Nan::ObjectWrap {
    public:
        NodePopplerDocument(const char* cFileName);
        ~NodePopplerDocument();

        inline bool isOk() {
            return doc->isOk();
        }
        inline PDFDoc *getDoc() {
            return doc;
        }
        static void Init(v8::Local<v8::Object> exports);

    protected:
        static NAN_METHOD(New);
        void evPageOpened(const NodePopplerPage *p);
        void evPageClosed(const NodePopplerPage *p);
        GooList *pages;

    private:
        V8_ACCESSOR_GETTER_DECL(paramsGetter);

        friend class NodePopplerPage;
        PDFDoc *doc;
    };
}

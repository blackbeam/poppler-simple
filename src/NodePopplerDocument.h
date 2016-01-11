#include <v8.h>
#include <node.h>
#include <nan.h>
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
	NodePopplerDocument(char* buffer, size_t length);
        ~NodePopplerDocument();

        inline bool isOk() {
            return doc->isOk();
        }
        inline PDFDoc *getDoc() {
            return doc;
        }
        static NAN_MODULE_INIT(Init);

    protected:
        static NAN_METHOD(New);
        void evPageOpened(const NodePopplerPage *p);
        void evPageClosed(const NodePopplerPage *p);
        GooList *pages;

    private:
        static NAN_GETTER(paramsGetter);

        friend class NodePopplerPage;
        PDFDoc *doc;
    };
}

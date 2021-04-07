#include <v8.h>
#include <node.h>
#include <nan.h>
#include <cpp/poppler-version.h>
#include <poppler/PDFDoc.h>
#include <poppler/ErrorCodes.h>
#include <poppler/PDFDocFactory.h>
#include <goo/GooString.h>

namespace node {
    class NodePopplerPage;
    class NodePopplerDocument : public Nan::ObjectWrap {
    public:
        NodePopplerDocument(
            const char* cFileName,
            GooString* ownerPassword = nullptr,
            GooString* userPassword = nullptr);
        NodePopplerDocument(
            char* buffer,
            size_t length,
            GooString* ownerPassword = nullptr,
            GooString* userPassword = nullptr);
        ~NodePopplerDocument();

        inline bool isOk() {
            return doc->isOk();
        }
        inline PDFDoc *getDoc() {
            return doc.get();
        }
        static NAN_MODULE_INIT(Init);

    protected:
        static NAN_METHOD(New);
        void evPageOpened(NodePopplerPage *p);
        void evPageClosed(NodePopplerPage *p);
        std::vector<NodePopplerPage*> pages;

    private:
        static NAN_GETTER(paramsGetter);

        friend class NodePopplerPage;
        std::unique_ptr<PDFDoc> doc;
        char *buffer;
    };
}

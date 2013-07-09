#include <v8.h>
#include <node.h>
#include <GlobalParams.h>
#include <poppler/PDFDoc.h>
#include <poppler/ErrorCodes.h>
#include <poppler/PDFDocFactory.h>
#include <goo/GooString.h>
#include <goo/GooList.h>

namespace node {
    class NodePopplerPage;
    class NodePopplerDocument : public ObjectWrap {
    public:
        NodePopplerDocument(const char* cFileName);
        ~NodePopplerDocument();
        inline bool isOk() {
            return doc->isOk();
        }
        static void Initialize(v8::Handle<v8::Object> target);

    protected:
        static v8::Handle<v8::Value> New(const v8::Arguments &args);
        void evPageOpened(const NodePopplerPage *p);
        void evPageClosed(const NodePopplerPage *p);
        GooList *pages;

    private:
        static v8::Persistent<v8::FunctionTemplate> constructor_template;

        static v8::Handle<v8::Value> paramsGetter(v8::Local<v8::String> property, const v8::AccessorInfo &info);

        friend class NodePopplerPage;
        PDFDoc *doc;
    };
}

#include <v8.h>
#include <node.h>
#include <GlobalParams.h>
#include <poppler/PDFDoc.h>
#include <poppler/ErrorCodes.h>
#include <poppler/PDFDocFactory.h>
#include <goo/GooString.h>

namespace node {

    class NodePopplerDocument : public ObjectWrap {
    public:
        NodePopplerDocument(const char* cFileName);
        ~NodePopplerDocument();
        inline bool isOk() {
            return doc->isOk();
        }
        static void Initialize(v8::Handle<v8::Object> target);
        static v8::Handle<v8::Value> getPageCount(v8::Local<v8::String> property, const v8::AccessorInfo& info);
    protected:
        static v8::Handle<v8::Value> New(const v8::Arguments &args);
    private:
        static v8::Persistent<v8::FunctionTemplate> constructor_template;

        static v8::Handle<v8::Value> paramsGetter(v8::Local<v8::String> property, const v8::AccessorInfo &info);

        friend class NodePopplerPage;
        PDFDoc *doc;
    };

}

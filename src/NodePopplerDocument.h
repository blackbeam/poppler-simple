#include <v8.h>
#include <node.h>
#include <PDFDoc.h>
#include <poppler/glib/poppler.h>

namespace node {

    class NodePopplerDocument : public ObjectWrap {
	public:
	    NodePopplerDocument(const char* cFileName);
	    ~NodePopplerDocument();
	    static void Initialize(v8::Handle<v8::Object> target);
	    static v8::Handle<v8::Value> getPageCount(v8::Local<v8::String> property, const v8::AccessorInfo& info);
	protected:
	    static v8::Handle<v8::Value> New(const v8::Arguments &args);
	private:
            static v8::Persistent<v8::FunctionTemplate> constructor_template;

            static v8::Handle<v8::Value> paramsGetter(v8::Local<v8::String> property, const v8::AccessorInfo &info);

	    PopplerDocument *document;
	    GError *err;
	    friend class NodePopplerPage;
    };

}

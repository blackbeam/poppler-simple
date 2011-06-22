#include <v8.h>
#include <node.h>
#include <node_events.h>
#include <PDFDoc.h>
#include <goo/GooString.h>

using namespace v8;
using namespace node;

static Handle<Value> VException(const char *msg) {
    HandleScope scope;
    return ThrowException(Exception::Error(String::New(msg)));
};

class PopplerDocument : public EventEmitter {
    public:
	PopplerDocument(const char* cFileName) : EventEmitter() {
	    doc = new PDFDoc((wchar_t) cFileName, strlen(cFileName));
	    //if(doc) {
		//page_count = doc->pages();
	//	pdf_version = doc->get_pdf_version();
	 //   }
	};
	~PopplerDocument() {};
	static void Initialize(Handle<Object> target) {
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
	    constructor_template->PrototypeTemplate()->SetAccessor(String::NewSymbol("page_count"), PageCountGetter);
	    constructor_template->PrototypeTemplate()->SetAccessor(String::NewSymbol("pdf_version"), PdfVersionGetter);

	    target->Set(String::NewSymbol("PopplerDocument"), constructor_template->GetFunction());

	}
	static Handle<Value> PageCountGetter(Local<String> property, const AccessorInfo& info) {
	    PopplerDocument *self = ObjectWrap::Unwrap<PopplerDocument>(info.This());
	    HandleScope scope;

	    return scope.Close(Integer::New(self->doc->getNumPages()));
	};
	static Handle<Value> PdfVersionGetter(Local<String> property, const AccessorInfo& info) {
	    PopplerDocument *self = ObjectWrap::Unwrap<PopplerDocument>(info.This());
	    HandleScope scope;

	    /*
	     * Creating v*::Object
	     */
	    Local<Object> version = Object::New();
	    version->Set(String::NewSymbol("major"), Integer::New(self->doc->getPDFMajorVersion()), ReadOnly);
	    version->Set(String::NewSymbol("minor"), Integer::New(self->doc->getPDFMajorVersion()), ReadOnly);

	    return version;
	}
    protected:
	static Handle<Value> New(const Arguments &args) {
	    HandleScope scope;
	    if(args.Length() != 1) return VException("One argument required: (filename: String).");
	    if(!args[0]->IsString()) return VException("'filename' must be an instance of String.");

	    String::Utf8Value str(args[0]);
	    if (*str == NULL) return VException("Couldn't convert a filename to string.");
	    PopplerDocument *p = new PopplerDocument(*str);
	    if (p->doc) {
		p->Wrap(args.This());
		return args.This();
	    }
	};
//	int GetPageCount() {
//	    return doc;
//	}
    private:
	PDFDoc* doc;
};

// Exporting function
    extern "C" void
init (Handle<Object> target)
{
    HandleScope scope;
    PopplerDocument::Initialize(target);
}

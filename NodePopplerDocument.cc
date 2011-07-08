#include <v8.h>
#include <node.h>
#include <node_events.h>
#include <node_buffer.h>
#include <poppler/glib/poppler.h>

#include "NodePopplerDocument.h"
#include "NodePopplerPage.h"

using namespace v8;
using namespace node;

namespace node {

    NodePopplerDocument::NodePopplerDocument(const char* cFileName) : EventEmitter() {
	err = NULL;
	document = NULL;
	document = poppler_document_new_from_file(cFileName, NULL, &err);
    }

    NodePopplerDocument::~NodePopplerDocument() {
	if (document)
	    g_object_unref(G_OBJECT(document));
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
        constructor_template->InstanceTemplate()->SetAccessor(String::NewSymbol("pdfVersion"), NodePopplerDocument::paramsGetter);
        constructor_template->InstanceTemplate()->SetAccessor(String::NewSymbol("isLinearized"), NodePopplerDocument::paramsGetter);
	target->Set(String::NewSymbol("PopplerDocument"), constructor_template->GetFunction());

    }

    Handle<Value> NodePopplerDocument::paramsGetter(Local< String > property, const AccessorInfo &info) {
      HandleScope scope;
      String::Utf8Value propName(property);
      NodePopplerDocument *self = ObjectWrap::Unwrap<NodePopplerDocument>(info.This());

      if (strcmp(*propName, "pageCount") == 0) {
        return scope.Close( Uint32::New( poppler_document_get_n_pages( self->document ) ) );
      }
      if (strcmp(*propName, "pdfVersion") == 0) {
        gchar* pdfVersion = poppler_document_get_pdf_version_string( self->document );
	Local<String> v8pdfVersion = String::New( pdfVersion );
        g_free( pdfVersion );
        return scope.Close( v8pdfVersion );
      }
      if (strcmp(*propName, "isLinearized") == 0) {
        return scope.Close( Boolean::New( poppler_document_is_linearized( self->document ) ) );
      }
    }

    Handle<Value> NodePopplerDocument::getPageCount(Local<String> property, const AccessorInfo& info) {
	NodePopplerDocument *self = ObjectWrap::Unwrap<NodePopplerDocument>(info.This());
	HandleScope scope;

	return scope.Close(Integer::New(poppler_document_get_n_pages(self->document)));
    }

    Handle<Value> NodePopplerDocument::New(const Arguments &args) {
	HandleScope scope;
	if(args.Length() != 1) return ThrowException(Exception::Error(String::New("One argument required: (filename: String).")));
	if(!args[0]->IsString()) return ThrowException(Exception::TypeError(String::New("'filename' must be an instance of String.")));

	String::Utf8Value str(args[0]);
	if (*str == NULL) return ThrowException(Exception::Error(String::New("Couldn't convert a filename to cstring.")));
	NodePopplerDocument *doc = new NodePopplerDocument(*str);
	if (doc->err) {
	    Local<String> error = String::New(doc->err->message);
	    g_error_free(doc->err);
	    delete doc;
	    return ThrowException(Exception::Error(error));
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
    // globalParams = new GlobalParams();
    // globalParams->setProfileCommands (true);
    //globalParams->setPrintCommands (true);

    //Required for gdk
    g_type_init();

    NodePopplerDocument::Initialize(target);
    NodePopplerPage::Initialize(target);
}

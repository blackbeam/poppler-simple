#include <v8.h>
#include <node.h>
#include <node_events.h>
#include <node_buffer.h>

#include "NodePopplerDocument.h"
#include "NodePopplerPage.h"

using namespace v8;
using namespace node;

Persistent<FunctionTemplate> NodePopplerPage::constructor_template;
static Persistent<String> width_sym;
static Persistent<String> height_sym;
static Persistent<String> index_sym;

namespace node {

    void NodePopplerPage::Initialize(v8::Handle<v8::Object> target) {
	HandleScope scope;

	width_sym = Persistent<String>::New(String::NewSymbol("width"));
	height_sym = Persistent<String>::New(String::NewSymbol("height"));
	index_sym = Persistent<String>::New(String::NewSymbol("index"));

	Local<FunctionTemplate> t = FunctionTemplate::New(New);
	constructor_template = Persistent<FunctionTemplate>::New(t);
	constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
	constructor_template->SetClassName(String::NewSymbol("PopplerPage"));

	/** Instance methods
	 *  static Handle<Value> funcName(const Arguments &args);
	 *  NODE_SET_PROTOTYPE_METHOD(constructor_template, "getPageCount", funcName);
	 */
	NODE_SET_PROTOTYPE_METHOD(constructor_template, "render", NodePopplerPage::render);
	NODE_SET_PROTOTYPE_METHOD(constructor_template, "findText", NodePopplerPage::findText);
	NODE_SET_PROTOTYPE_METHOD(constructor_template, "test", NodePopplerPage::test);

	/** Getters:
	 *  static Handle<Value> funcName(Local<String> property, const AccessorInfo& info);
	 *  constructor_template->PrototypeTemplate()->SetAccessor(String::NewSymbol("page_count"), funcName);
	 */
	constructor_template->InstanceTemplate()->SetAccessor(String::NewSymbol("index"), NodePopplerPage::paramsGetter);
	constructor_template->InstanceTemplate()->SetAccessor(String::NewSymbol("width"), NodePopplerPage::paramsGetter);
	constructor_template->InstanceTemplate()->SetAccessor(String::NewSymbol("height"), NodePopplerPage::paramsGetter);
	constructor_template->InstanceTemplate()->SetAccessor(String::NewSymbol("crop_box"), NodePopplerPage::paramsGetter);
	constructor_template->InstanceTemplate()->SetAccessor(String::NewSymbol("images"), NodePopplerPage::paramsGetter);
	/** Class methods
	 * NODE_SET_METHOD(constructor_template->GetFunction(), "GetPageCount", funcName);
	 */
	// NODE_SET_METHOD(constructor_template->GetFunction(), "pixbufToImage", NodePopplerPage::pixbufToImage);

	target->Set(String::NewSymbol("PopplerPage"), constructor_template->GetFunction());
    }

    NodePopplerPage::~NodePopplerPage() {
	if ( page )
	    g_object_unref(G_OBJECT(page));
	if ( mapping != NULL )
	    poppler_page_free_image_mapping( mapping );
    }

    NodePopplerPage::NodePopplerPage(NodePopplerDocument* doc, int32_t index) : ObjectWrap() {
	mapping = NULL;
	page = poppler_document_get_page(doc->document, index - 1);
	if ( page ) {
	    document = doc->document;
	    poppler_page_get_size(page ,&width, &height);
	    mapping = poppler_page_get_image_mapping( page );
	}
    }

    Handle<Value> NodePopplerPage::New(const Arguments &args) {
	HandleScope scope;
	NodePopplerDocument* doc;
	int32_t index;

	if (args.Length() != 2) return ThrowException(Exception::Error(String::New("Two arguments required: (doc: NodePopplerDocument, index: Integer).")));
	if (!args[1]->IsNumber()) return ThrowException(Exception::TypeError(String::New("'index' must be an instance of Integer.")));
	index = args[1]->ToInt32()->Value();

	if(!args[0]->IsObject()) {
	    return ThrowException(Exception::TypeError(String::New("'doc' must be an instance of NodePopplerDocument.")));
	}
	doc = ObjectWrap::Unwrap<NodePopplerDocument>(args[0]->ToObject());

	NodePopplerPage* page = new NodePopplerPage(doc, index);
	if(! page->isOk() ) {
	    delete page;
	    return ThrowException(Exception::Error(String::New("Can't open page.")));;
	}
	page->Wrap(args.This());
	return args.This();
    }

    Handle<Value> NodePopplerPage::paramsGetter(Local< String > property, const AccessorInfo &info) {
	HandleScope scope;

	String::Utf8Value propName(property);
	NodePopplerPage *self = ObjectWrap::Unwrap<NodePopplerPage>(info.This());

	if (strcmp(*propName, "width") == 0) {
	    return scope.Close(Number::New(self->width));
	}
	if (strcmp(*propName, "height") == 0) {
	    return scope.Close(Number::New(self->height));
	}
	if (strcmp(*propName, "index") == 0) {
	    return scope.Close(Uint32::New(poppler_page_get_index(self->page) + 1));
	}
	if (strcmp(*propName, "crop_box") == 0) {
	    Local<v8::Object> crop_box = v8::Object::New();
	    PopplerRectangle rect;

	    poppler_page_get_crop_box ( self->page, &rect );

	    crop_box->Set(String::NewSymbol("x1"), Number::New(rect.x1));
	    crop_box->Set(String::NewSymbol("x2"), Number::New(rect.x2));
	    crop_box->Set(String::NewSymbol("y1"), Number::New(rect.y1));
	    crop_box->Set(String::NewSymbol("y2"), Number::New(rect.y2));

	    return scope.Close(crop_box);
	}
	if (strcmp(*propName, "images") == 0) {
	    Local<v8::Object> images = v8::Object::New();
	    GList *l;
	    uint32_t i = 0, len;

	    if ( ! self->mapping ) {
		return scope.Close(Handle<Value>());
	    }

	    len = g_list_length (self->mapping);

	    if ( len > 0 ) {
		Local<v8::Array> images = v8::Array::New(len);
		for ( l = self->mapping; l; l = g_list_next (l), i++) {
		    PopplerImageMapping *imapping;
		    imapping = (PopplerImageMapping *)l->data;
		    Local<v8::Object> img = v8::Object::New();
		    img->Set(String::NewSymbol("x1"), Number::New(imapping->area.x1));
		    img->Set(String::NewSymbol("x2"), Number::New(imapping->area.x2));
		    img->Set(String::NewSymbol("y1"), Number::New(imapping->area.y1));
		    img->Set(String::NewSymbol("y2"), Number::New(imapping->area.y2));
		    images->Set( i, img );
		}
		return scope.Close(images);
	    }
	    return scope.Close(Handle<Value>());
	}
    }

    // Возвращает относительные координаты от левого нижнего угла страницы
    Handle<Value> NodePopplerPage::findText(const Arguments &args) {
	HandleScope scope;
	NodePopplerPage* self = ObjectWrap::Unwrap<NodePopplerPage>(args.Holder());
	GList *results = NULL;
	int res_len = 0;

	if (args.Length() != 1 && !args[0]->IsString()) return ThrowException(Exception::Error(String::New("One argument required: (text: String)")));
	String::Utf8Value text(args[0]);

	results = poppler_page_find_text(self->page, *text);
	res_len = g_list_length(results);

	Local<v8::Array> v8results = v8::Array::New(res_len);
	for (int i = 0;results != NULL; results = g_list_next(results), i++) {
	    PopplerRectangle *result = (PopplerRectangle*) results->data;
	    Local<v8::Object> v8result = v8::Object::New();
	    v8result->Set(String::NewSymbol("x1"), Number::New(result->x1 / self->width));
	    v8result->Set(String::NewSymbol("x2"), Number::New(result->x2 / self->width));
	    v8result->Set(String::NewSymbol("y1"), Number::New(result->y1 / self->height));
	    v8result->Set(String::NewSymbol("y2"), Number::New(result->y2 / self->height));
	    v8results->Set(i, v8result);
	    //delete result;
	}
	g_list_free(results);
	return scope.Close(v8results);
    }

    Handle<Value> NodePopplerPage::render(const Arguments &args) {
	HandleScope scope;
	NodePopplerPage* self = ObjectWrap::Unwrap<NodePopplerPage>(args.Holder());
	gdouble PPI;
	bool forcePixbuf;
	GdkPixbuf *pixbuf;
	int scalledWidth, scalledHeight, pixbufSize;

	if (args.Length() != 2) return ThrowException(Exception::Error(String::New("Two arguments required: (PPI: Number, forcePixbuf: Boolean).")));
	if (!args[0]->IsNumber()) return ThrowException(Exception::TypeError(String::New("`PPI' must be an instance of Number.")));
	if (!args[1]->IsBoolean()) return ThrowException(Exception::TypeError(String::New("`forcePixbuf' must be an instance of Boolean.")));

	PPI = args[0]->ToNumber()->Value();
	forcePixbuf = args[1]->ToBoolean()->Value();

	scalledHeight = static_cast<int>( (self->height / 72.0) * PPI );
	scalledWidth = static_cast<int>( (self->width / 72.0) * PPI );

	if ( self->mapping && ! forcePixbuf && g_list_length (self->mapping) == 1 ) {
	    // render to output dev
	    ImageOutputDev *imgOut;
	    char path[L_tmpnam + 4];
	    tmpnam( path );
	    int page_num;
	    bool is_saved;

	    imgOut = new ImageOutputDev( self->width / 72.0, (PopplerImageMapping *)self->mapping->data, path, &is_saved );
	    if (imgOut->isOk()) {
		page_num = poppler_page_get_index(self->page) + 1;
		self->page->document->doc->displayPage(imgOut, page_num, 72, 72, 0,
			gTrue, gFalse, gFalse);
	    }
	    delete imgOut;
	    if ( is_saved ) {
		Local<v8::Object> out = v8::Object::New();

		out->Set(String::NewSymbol("type"), String::New("file"));
		out->Set(String::NewSymbol("path"), String::New(path));

		return scope.Close(out);
	    }

	}
	// render to pixbuf
to_pixbuf:
	Buffer *bp;

	pixbuf = gdk_pixbuf_new( GDK_COLORSPACE_RGB, FALSE, 8, scalledWidth, scalledHeight );
	poppler_page_render_to_pixbuf(self->page, 0, 0, scalledWidth, scalledHeight, PPI / 72.0, 0, pixbuf);

	pixbufSize = ( gdk_pixbuf_get_height( pixbuf ) * gdk_pixbuf_get_rowstride( pixbuf ) );

	bp = Buffer::New(pixbufSize);
	memcpy(Buffer::Data( bp ), gdk_pixbuf_get_pixels(pixbuf), pixbufSize);

	Local<v8::Object> out = v8::Object::New();
	Local<v8::Object> v8pixbuf = v8::Object::New();
	v8pixbuf->Set(String::NewSymbol("width"), Integer::New(gdk_pixbuf_get_width(pixbuf)));
	v8pixbuf->Set(String::NewSymbol("height"), Integer::New(gdk_pixbuf_get_height(pixbuf)));
	v8pixbuf->Set(String::NewSymbol("pixels"), bp->handle_);
	v8pixbuf->Set(String::NewSymbol("has_alpha"), Boolean::New(gdk_pixbuf_get_has_alpha(pixbuf)));

	out->Set(String::NewSymbol("type"), String::New("pixbuf"));
	out->Set(String::NewSymbol("data"), v8pixbuf);

	g_object_unref(pixbuf);

	return scope.Close(out);
    }

    Handle<Value> NodePopplerPage::test(const Arguments &args) {
	HandleScope scope;
	NodePopplerPage* self = ObjectWrap::Unwrap<NodePopplerPage>(args.Holder());
	Local<v8::Object> out = v8::Object::New();

	if ( !self->mapping ) {
	    out->Set(v8::String::NewSymbol("quality"), v8::String::New("DAMAGED"));
	    return scope.Close( out );
	}

	ImageQualityDev *imgQDev = new ImageQualityDev( self->width / 72.0, (PopplerImageMapping *)self->mapping->data, out );
	if (imgQDev->isOk()) {
	    int page_num = poppler_page_get_index(self->page) + 1;
	    self->page->document->doc->displayPage(imgQDev, page_num, 72, 72, 0,
			gTrue, gFalse, gFalse);
	}
	delete imgQDev;
	return scope.Close( out );
    }

}

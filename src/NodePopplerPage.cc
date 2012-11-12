#include <v8.h>
#include <node.h>
#include <node_buffer.h>

#include "NodePopplerDocument.h"
#include "NodePopplerPage.h"

using namespace v8;
using namespace node;

Persistent<FunctionTemplate> NodePopplerPage::constructor_template;
static Persistent<String> width_sym;
static Persistent<String> height_sym;
static Persistent<String> index_sym;

struct render_work_t
{
    gboolean isOk;
    NodePopplerPage *src;
    uv_work_t request;
    gdouble ppi;
    GdkPixbuf *dst;
    int pixbufSize;
    Persistent<v8::Function> cb;
};

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
        g_object_unref(G_OBJECT(document));
        if ( page )
            g_object_unref(G_OBJECT(page));
        if ( mapping != NULL )
            poppler_page_free_image_mapping(mapping);
    }

    NodePopplerPage::NodePopplerPage(NodePopplerDocument* doc, int32_t index) : ObjectWrap() {
        mapping = NULL;
        page = poppler_document_get_page(doc->document, index - 1);
        if ( page ) {
            document = doc->document;
            g_object_ref(document);
            poppler_page_get_size(page ,&width, &height);
            mapping = poppler_page_get_image_mapping(page);
        }
    }

    Handle<Value> NodePopplerPage::New(const Arguments &args) {HandleScope scope;
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

    Handle<Value> NodePopplerPage::paramsGetter(Local<String> property, const AccessorInfo &info) {
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

            poppler_page_get_crop_box(self->page, &rect);

            crop_box->Set(String::NewSymbol("x1"), Number::New(rect.x1));
            crop_box->Set(String::NewSymbol("x2"), Number::New(rect.x2));
            crop_box->Set(String::NewSymbol("y1"), Number::New(rect.y1));
            crop_box->Set(String::NewSymbol("y2"), Number::New(rect.y2));

            return scope.Close(crop_box);
        }
        if (strcmp(*propName, "images") == 0) {
            Local<v8::Object> images = v8::Object::New();
            uint32_t i = 0, len;

            if (!self->mapping) {
                return scope.Close(Handle<Value>());
            }

            len = g_list_length(self->mapping);

            if (len > 0) {
                Local<v8::Array> images = v8::Array::New(len);
                GList *l;
                for (l = self->mapping; l; l = g_list_next(l), i++) {
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

    GdkPixbuf *NodePopplerPage::render_to_pixbuf(int width, int height, double scale) {
        GdkPixbuf *pixbuf;
        cairo_surface_t *surface;
        cairo_t *cr;

        surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
        cr = cairo_create(surface);
        if (scale != 1.0) {
            cairo_scale(cr, scale, scale);
        }
        poppler_page_render(this->page, cr);
        cairo_set_operator(cr, CAIRO_OPERATOR_DEST_OVER);
        cairo_set_source_rgb(cr, 1., 1., 1.);
        cairo_paint(cr);
        cairo_destroy(cr);

        pixbuf = this->cairo_to_pixbuf(surface);
        
        cairo_surface_destroy(surface);

        return pixbuf;
    }

    GdkPixbuf *NodePopplerPage::cairo_to_pixbuf(cairo_surface_t *surface) {
        int cairo_width, cairo_height, cairo_rowstride;
        unsigned char *pixbuf_data, *dst, *cairo_data;
        int pixbuf_rowstride, pixbuf_n_channels;
        unsigned int *src;
        int x, y;
        GdkPixbuf *pixbuf;

        cairo_width = cairo_image_surface_get_width(surface);
        cairo_height = cairo_image_surface_get_height(surface);
        cairo_rowstride = cairo_image_surface_get_stride (surface);
        cairo_data = cairo_image_surface_get_data (surface);

        pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, cairo_width, cairo_height);
        pixbuf_data = gdk_pixbuf_get_pixels (pixbuf);
        pixbuf_rowstride = gdk_pixbuf_get_rowstride (pixbuf);
        pixbuf_n_channels = gdk_pixbuf_get_n_channels (pixbuf);

        for (y = 0; y < cairo_height; y++) {
            src = (unsigned int *) (cairo_data + y * cairo_rowstride);
            dst = pixbuf_data + y * pixbuf_rowstride;
            for (x = 0; x < cairo_width; x++) {
                dst[0] = (*src >> 16) & 0xff;
                dst[1] = (*src >> 8) & 0xff;
                dst[2] = (*src >> 0) & 0xff;
                if (pixbuf_n_channels == 4)
                    dst[3] = (*src >> 24) & 0xff;
                dst += pixbuf_n_channels;
                src++;
            }
        }

        return pixbuf;
    }

    Handle<Value> NodePopplerPage::render(const Arguments &args) {
        HandleScope scope;
        NodePopplerPage* self = ObjectWrap::Unwrap<NodePopplerPage>(args.Holder());
        struct render_work_t *work;

        if (args.Length() != 1) return ThrowException(Exception::Error(String::New("One argument required: (PPI: Number).")));
        if (!args[0]->IsNumber()) return ThrowException(Exception::TypeError(String::New("`PPI' must be an instance of Number.")));

        work = new render_work_t();
        work->request.data = work;
        work->src = self;
        work->ppi = args[0]->ToNumber()->Value();
        work->isOk = false;

        if (args[args.Length() - 1]->IsFunction()) {
            work->cb = Persistent<v8::Function>::New(Handle<v8::Function>::Cast(args[args.Length()-1]->ToObject()));
            uv_queue_work(uv_default_loop(), &work->request, NodePopplerPage::_render, NodePopplerPage::afterRender);
        } else {
            NodePopplerPage::_render(&(work->request));
            if (work->isOk) {
                Buffer *pb;
                Local<v8::Object> out = v8::Object::New();
                Local<v8::Object> v8pixbuf = v8::Object::New();

                pb = Buffer::New(work->pixbufSize);
                memcpy(Buffer::Data(pb), gdk_pixbuf_get_pixels(work->dst), work->pixbufSize);

                v8pixbuf->Set(String::NewSymbol("width"), Integer::New(gdk_pixbuf_get_width(work->dst)));
                v8pixbuf->Set(String::NewSymbol("height"), Integer::New(gdk_pixbuf_get_height(work->dst)));
                v8pixbuf->Set(String::NewSymbol("pixels"), pb->handle_);
                v8pixbuf->Set(String::NewSymbol("has_alpha"), Boolean::New(gdk_pixbuf_get_has_alpha(work->dst)));
                out->Set(String::NewSymbol("type"), String::New("pixbuf"));
                out->Set(String::NewSymbol("data"), v8pixbuf);

                g_object_unref(work->dst);
                delete work;
                return scope.Close(out);
            } else {
                delete work;
                return scope.Close(Null());
            }
        }
        return scope.Close(args.This());
    }

    void NodePopplerPage::_render(uv_work_t *work_req) {
        render_work_t *work = static_cast<render_work_t *>(work_req->data);
        int scalledWidth, scalledHeight;
        double scale;

        scale = work->ppi / 72.0;
        scalledHeight = static_cast<int>(work->src->height * scale);
        scalledWidth = static_cast<int>(work->src->width * scale);
        work->dst = work->src->render_to_pixbuf(scalledWidth, scalledHeight, scale);
        work->pixbufSize = (gdk_pixbuf_get_height(work->dst) * gdk_pixbuf_get_rowstride(work->dst));
        work->isOk = true;
    }

    void NodePopplerPage::afterRender(uv_work_t *work_req) {
        HandleScope scope;
        render_work_t *work = static_cast<render_work_t *>(work_req->data);
        if (work->isOk == true) {
            Buffer *pb;
            Local<v8::Object> out = v8::Object::New();
            Local<v8::Object> v8pixbuf = v8::Object::New();

            pb = Buffer::New(work->pixbufSize);
            memcpy(Buffer::Data(pb), gdk_pixbuf_get_pixels(work->dst), work->pixbufSize);

            v8pixbuf->Set(String::NewSymbol("width"), Integer::New(gdk_pixbuf_get_width(work->dst)));
            v8pixbuf->Set(String::NewSymbol("height"), Integer::New(gdk_pixbuf_get_height(work->dst)));
            v8pixbuf->Set(String::NewSymbol("pixels"), pb->handle_);
            v8pixbuf->Set(String::NewSymbol("has_alpha"), Boolean::New(gdk_pixbuf_get_has_alpha(work->dst)));
            out->Set(String::NewSymbol("type"), String::New("pixbuf"));
            out->Set(String::NewSymbol("data"), v8pixbuf);

            g_object_unref(work->dst);

            Local<Value> argv[2] = {Local<Value>::New(Null()), out};
            work->cb->Call(Context::GetCurrent()->Global(), 2, argv);
        } else {
            Local<Value> argv[1] = {Exception::Error(String::New("Can not render page to pixbuf."))};
            work->cb->Call(Context::GetCurrent()->Global(), 1, argv);
        }
    }
}

#ifndef HELPERS_H_
#define HELPERS_H_

#if NODE_VERSION_MAJOR == 0 && NODE_VERSION_MINOR <= 10
#define V8_METHOD(method) v8::Handle<v8::Value> method(const v8::Arguments& args)
#define V8_ACCESSOR_GETTER(method) v8::Handle<v8::Value> method(v8::Local<v8::String> property, \
                                                                const v8::AccessorInfo &info)
#define V8_ACCESSOR_RETURN(result) return result
#define V8_RETURN(result) return result
#define CREATE_HANDLE_SCOPE \
    v8::HandleScope scope
#define PERSIST_CALLBACK(container, callback) \
    (container) = v8::Persistent<v8::Function>::New(v8::Local<v8::Function>::Cast(callback))
#define EXTRACT_CALLBACK(var, container) \
    v8::Local<v8::Function> (var) = v8::Local<v8::Function>::New(container)
#define DISPOSE_PERSISTENT(container) \
    (container).Dispose()

#else
#define V8_METHOD(method) void method(const v8::FunctionCallbackInfo<v8::Value>& args)
#define V8_ACCESSOR_GETTER(method) void method(v8::Local<v8::String> property, \
                                               const v8::PropertyCallbackInfo<v8::Value> &info)
#define V8_ACCESSOR_RETURN(result) do { info.GetReturnValue().Set(result);return; } while(0)
#define V8_RETURN(result) do { args.GetReturnValue().Set(result);return; } while(0)
#define CREATE_HANDLE_SCOPE \
    v8::HandleScope scope(v8::Isolate::GetCurrent())
#define PERSIST_CALLBACK(container, callback) \
    (container).Reset(v8::Isolate::GetCurrent(), callback)
#define EXTRACT_CALLBACK(var, container) \
    v8::Local<v8::Function> (var) = v8::Local<v8::Function>::New(v8::Isolate::GetCurrent(), \
                                                                 container)
#define DISPOSE_PERSISTENT(container) \
    (container).Dispose(v8::Isolate::GetCurrent())
#endif

#define V8_METHOD_DECL(method) static V8_METHOD(method)
#define V8_ACCESSOR_GETTER_DECL(method) static V8_ACCESSOR_GETTER(method)
#define V8_FUNCTION_DECL(method) V8_METHOD(method)
#define V8_FUNCTION(method) static V8_METHOD(method)
#define V8_THROW(exception) V8_RETURN(ThrowException(exception))

#endif

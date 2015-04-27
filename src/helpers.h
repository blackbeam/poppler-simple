#ifndef HELPERS_H_
#define HELPERS_H_

#if NODE_VERSION_MAJOR == 0 && NODE_VERSION_MINOR <= 10

#define V8_ACCESSOR_GETTER(method) v8::Handle<v8::Value> method(v8::Local<v8::String> property, \
                                                                const v8::AccessorInfo &info)
#define V8_ACCESSOR_RETURN(result) return result

#else

#define V8_ACCESSOR_GETTER(method) void method(v8::Local<v8::String> property, \
                                               const v8::PropertyCallbackInfo<v8::Value> &info)
#define V8_ACCESSOR_RETURN(result) do { info.GetReturnValue().Set(result);return; } while(0)

#endif

#define V8_ACCESSOR_GETTER_DECL(method) static V8_ACCESSOR_GETTER(method)

#endif

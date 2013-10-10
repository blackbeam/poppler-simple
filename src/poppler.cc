#include <node.h>
#include "NodePopplerDocument.h"
#include "NodePopplerPage.h"

using namespace v8;
using namespace node;

void InitAll(Handle<v8::Object> exports) {
    globalParams = new GlobalParams();
    NodePopplerDocument::Init(exports);
    NodePopplerPage::Init(exports);
}

NODE_MODULE(poppler, InitAll)

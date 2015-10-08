#include <nan.h>
#include <node.h>
#include "NodePopplerDocument.h"
#include "NodePopplerPage.h"

using namespace v8;
using namespace node;

NAN_MODULE_INIT(InitAll) {
    globalParams = new GlobalParams();
    NodePopplerPage::Init(target);
    NodePopplerDocument::Init(target);
}

NODE_MODULE(poppler, InitAll)

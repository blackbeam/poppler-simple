#include <nan.h>
#include <node.h>
#include "NodePopplerDocument.h"
#include "NodePopplerPage.h"

using namespace v8;
using namespace node;

NAN_MODULE_INIT(InitAll) {
#if POPPLER_VERSION_MAJOR == 0 && POPPLER_VERSION_MINOR < 83
    globalParams = new GlobalParams();
#else
    globalParams = std::unique_ptr<GlobalParams>(new GlobalParams);
#endif
    NodePopplerPage::Init(target);
    NodePopplerDocument::Init(target);
}

NODE_MODULE(poppler, InitAll)
